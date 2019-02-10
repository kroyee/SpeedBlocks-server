#include "Connections.h"
#include <fstream>
#include <string>
#include "AsyncTask.h"
#include "GameSignals.h"
#include "JSONWrap.h"
#include "NetworkPackets.hpp"
#include "Tournament.h"

using std::cout;
using std::endl;

Connections::Connections(bool use_database) : use_database(use_database), tcpPort(21512), sender(nullptr), clientVersion(114), clientCount(0), lobby(*this) {
    udpSock.bind(21514);
    selector.add(udpSock);

    PM::init();

    PM::handle_packet([&](const NP_LoginRequest& p) { validateClient(p); });
    PM::handle_packet([&](const NP_GameOver& p) { sender->getRoundData(p); });
    PM::handle_packet([&](const NP_ChatMsg& p) { sendChatMsg(p); });
    PM::handle_packet([&](const NP_CreateRoom& p) { lobby.addRoom(p.name, p.maxPlayers, 3, 3); });
    PM::handle_packet([&](const NP_ConfirmUdp& p) { validateUDP(p); });
    PM::handle_packet([&](const NP_Gamestate& p) { getGamestate(p); });
    PM::handle_packet([&](const NP_Ping& p) { sender->ping.get(serverClock.getElapsedTime(), *sender, p); });

    PM::handle_packet<NP_Leave>([&]() {
        if (sender->room != nullptr) sender->room->leave(*sender);
    });
    PM::handle_packet([&](const NP_LinesSent& p) {
        if (sender->room != nullptr) sender->room->sendLines(*sender, p.amount);
    });
    PM::handle_packet([&](const NP_GarbageCleared& p) { sender->roundStats.garbageCleared += p.amount; });
    PM::handle_packet([&](const NP_LinesBlocked& p) { sender->roundStats.score.lines_blocked += p.amount; });
    PM::handle_packet<NP_Away>([&]() { sender->goAway(); });
    PM::handle_packet<NP_NotAway>([&]() { sender->unAway(); });
    PM::handle_packet<NP_Ready>([&]() {
        if (!sender->ready && !sender->alive && sender->room != nullptr) {
            auto packet = PM::make<NP_ClientReady>(sender->id);
            sender->room->sendPacket(packet);
            sender->room->sendPacketToSpectators(packet);
        }
        sender->ready = true;
    });
    PM::handle_packet<NP_NotReady>([&]() {
        if (sender->ready && !sender->alive && sender->room != nullptr) {
            auto packet = PM::make<NP_ClientNotReady>(sender->id);
            sender->room->sendPacket(packet);
            sender->room->sendPacketToSpectators(packet);
        }
        sender->ready = false;
    });

    PM::handle_packet<NP_SpectatorLeave>([&]() {
        if (sender->spectating) sender->spectating->removeSpectator(*sender);
    });

    connectSignal("SendUDP", &Connections::sendUDP, this);
    connectSignal("SendAuthResult", &Connections::sendAuthResult, this);
    connectSignal("GetUploadData", &Connections::getUploadData, this);
    connectSignal("SendClientJoinedServerInfo", &Connections::sendClientJoinedServerInfo, this);
    connectSignal("GetServerKey", [&]() { return serverkey; });
}

bool Connections::setUpListener() {
    if (listener.listen(tcpPort) == sf::Socket::Done) {
        selector.add(listener);
        return true;
    } else
        return false;
}

void Connections::listen() {
    if (selector.wait(sf::milliseconds(100))) receive();
}

void Connections::receive() {
    if (selector.isReady(listener)) {
        static uint16_t idcount = 60000;
        clients.emplace_back(std::make_shared<HumanClient>());
        Client& newclient = *clients.back();
        newclient.id = idcount++;

        if (newclient.initHuman(listener, selector, serverClock.getElapsedTime())) {
            clientCount++;
            sendWelcomeMsg();
            lobby.sendTournamentList(newclient);
            lobby.challengeHolder.sendChallengeList(newclient);
        } else {
            std::cout << "CLient accept failed" << std::endl;
            clients.pop_back();
            idcount--;
        }
        if (idcount < 60000) idcount = 60000;
    }
    if (selector.isReady(udpSock)) {
        PM packet;
        status = udpSock.receive(packet, udpAdd, udpPort);

        if (status == sf::Socket::Done) {
            for (auto&& client : clients) {
                if (client->address == udpAdd && client->udpPort == udpPort) {
                    sender = client.get();
                    client->lastHeardFrom = serverClock.getElapsedTime();
                    packet.read();
                    break;
                }
            }
        }
    }
    for (auto it = clients.begin(); it != clients.end(); it++) {
        HumanClient& client = *(*it);
        if (selector.isReady(client.socket)) {
            PM packet;
            status = client.socket.receive(packet);
            if (status == sf::Socket::Done) {
                sender = it->get();
                sender->lastHeardFrom = serverClock.getElapsedTime();
                packet.read();
            } else if (status == sf::Socket::Disconnected) {
                disconnectClient(*it);
                it = clients.erase(it);
            }
        }
    }
}

void Connections::disconnectClient(std::shared_ptr<HumanClient>& client_ptr) {
    HumanClient& client = *client_ptr;  // Need the shared_ptr to push to uploadData
    sendClientLeftServerInfo(client);
    if (!client.guest) {
        uploadData.push_back(client_ptr);
        uploadData.back()->uploadTime = serverClock.getElapsedTime() + sf::seconds(240);
    }
    if (client.room != nullptr) client.room->leave(client);
    if (client.tournament != nullptr) client.tournament->removeObserver(client);
    if (client.spectating) client.spectating->removeSpectator(client);
    if (client.matchmaking) lobby.matchmaking1vs1.removeFromQueue(client);
    selector.remove(client.socket);
    client.socket.disconnect();
    std::cout << "Client " << client.id << " disconnected" << std::endl;
    clientCount--;
}

void Connections::send(Client& fromClient, HumanClient& toClient) {
    auto packet = PM::make<NP_Gamestate>(fromClient.data);
    status = udpSock.send(packet, toClient.address, toClient.udpPort);
    if (status != sf::Socket::Done) std::cout << "Error sending UDP packet from " << (int)fromClient.id << " to " << (int)toClient.id << std::endl;
}

void Connections::sendUDP(HumanClient& client, PM& packet) {
    status = udpSock.send(packet, client.address, client.udpPort);
    if (status != sf::Socket::Done) std::cout << "Error sending UDP packet to " << (int)client.id << std::endl;
}

void Connections::sendWelcomeMsg() {
    PM packet;
    NP_Welcome welcome{clients.back()->id, lobby.welcomeMsg};
    packet.write(welcome);
    packet.write_as<NP_ClientList>(*this);
    packet.write_as<NP_RoomList>(lobby);
    packet.write_as<NP_MatchMaking>(lobby.matchmaking1vs1);
    clients.back()->sendPacket(packet);
}

void Connections::sendAuthResult(uint8_t authresult, Client& client) {
    PM packet;
    if (authresult == 2) {
        for (auto&& client : clients)  // Checking for duplicate names, and sending back 4 if found
            if (client->id != sender->id && client->name == sender->name) authresult = 4;
    }
    NP_AuthResult result{authresult, client.id, client.name};
    client.sendPacket(packet);
}

void Connections::sendChatMsg(const NP_ChatMsg& p) {
    PM packet;
    packet.write(p);
    if (p.target == "room") {
        if (sender->room) {
            for (auto& client : sender->room->clients)
                if (client->id != sender->id) client->sendPacket(packet);
            for (auto& client : sender->room->spectators)
                if (client->id != sender->id) client->sendPacket(packet);
        } else if (sender->spectating) {
            for (auto& client : sender->spectating->clients)
                if (client->id != sender->id) client->sendPacket(packet);
            for (auto& client : sender->spectating->spectators)
                if (client->id != sender->id) client->sendPacket(packet);
        }
    } else if (p.target == "lobby") {
        for (auto& client : clients)
            if (client->id != sender->id) client->sendPacket(packet);
    } else {
        for (auto& client : clients)
            if (client->name == p.target) {
                client->sendPacket(packet);
                break;
            }
    }
}

void Connections::sendClientJoinedServerInfo(HumanClient& newclient) {
    PM packet;
    NP_ClientJoinedServer info{{newclient.id, newclient.name}};
    packet.write(info);
    for (auto& client : clients) client->sendPacket(packet);
}

void Connections::sendClientLeftServerInfo(HumanClient& client) {
    auto packet = PM::make<NP_ClientLeftServer>(client.id);
    for (auto& otherClient : clients)
        if (otherClient->id != client.id) otherClient->sendPacket(packet);
}

void Connections::validateClient(const NP_LoginRequest& p) {
    sender->name = p.nameOrHash;
    if (p.clientVersion != clientVersion) {
        sendAuthResult(3, *sender);
        std::cout << "Client tried to connect with wrong client version: " << p.clientVersion << std::endl;
    } else if (p.guest) {
        sendAuthResult(2, *sender);
        std::cout << "Guest confirmed: " << sender->name << std::endl;
        sendClientJoinedServerInfo(*sender);
        sender->sendAlert(
            "First time here using the latest version i see.\nTake the time to check out the Message of the Day under the Server tab, there you can find some tips on "
            "the new GUI and it's features.");
    } else
        AsyncTask::add([=]() { sender->authUser(); });
}

void Connections::validateUDP(const NP_ConfirmUdp& p) {
    for (auto&& client : clients)
        if (client->id == p.id) {
            if (client->udpPort != udpPort) client->udpPort = udpPort;
            client->sendPacket(PM::make<NP_UdpConfirmed>());
            cout << "Confirmed UDP port for " << client->id << endl;
            return;
        }
}

void Connections::getGamestate(const NP_Gamestate& p) {
    if ((p.count < 50 && sender->data.count > 200) || sender->data.count < p.count) {
        sender->data = p;
        sender->datavalid = true;
        /*sender->history.states.push_front({});
        if (sender->history.states.size() > 100) sender->history.states.pop_back();

        PacketCompressServer extractor;
        extractor.loadTmp(p.data);
        extractor.extract();
        sender->history.states.front() = extractor;
        sender->history.validate();*/
    }
}

void Connections::manageRooms() {
    for (auto&& room : lobby.rooms) {
        if (room->active) {
            room->sendGameData(udpSock);
            room->makeCountdown();
            room->checkIfRoundEnded();
        }
    }
    for (auto&& room : lobby.tmp_rooms) {
        if (room->active) {
            room->sendGameData(udpSock);
            room->makeCountdown();
            room->checkIfRoundEnded();
        }
    }
    lobby.removeIdleRooms();
}

void Connections::manageClients() {
    static sf::Time users_online_update = sf::seconds(0);
    JSONWrap jwrap;
    for (auto it = clients.begin(); it != clients.end(); it++) {
        HumanClient& client = *(*it);
        if (serverClock.getElapsedTime() - client.lastHeardFrom > sf::seconds(10)) {
            disconnectClient(*it);
            it = clients.erase(it);
            continue;
        }
        if (serverClock.getElapsedTime() - client.updateStatsTime > sf::seconds(60) && !client.guest) {
            client.updateStatsTime = serverClock.getElapsedTime();
            AsyncTask::add([&]() { client.sendData(); });
        }
        client.checkIfStatsSet();
        client.checkIfAuth();

        if (client.room)
            jwrap.addPair(std::to_string(client.id), client.room->gamemode);
        else
            jwrap.addPair(std::to_string(client.id), 0);
    }

    if (serverClock.getElapsedTime() > users_online_update && use_database) {
        users_online_update = serverClock.getElapsedTime() + sf::seconds(60);
        jwrap.addPair("key", serverkey);
        sf::Http::Response response = jwrap.sendPost("/set_usersonline.php");

        if (response.getStatus() != sf::Http::Response::Ok) std::cout << "set_usersonline failed: " << response.getBody() << std::endl;
    }

    manageUploadData();
}

void Connections::manageUploadData() {
    for (auto it = uploadData.begin(); it != uploadData.end(); it++) {
        HumanClient& client = *(*it);
        if (serverClock.getElapsedTime() > client.uploadTime) {
            client.sdataPut = true;
            AsyncTask::add([&]() { client.sendData(); });
            client.uploadTime = client.uploadTime + sf::seconds(1000);
        }
        if (client.sdataSet) {
            client.sdataSet = false;
            client.sdataSetFailed = false;
            it = uploadData.erase(it);
        } else if (client.sdataSetFailed) {
            client.sdataSet = false;
            client.sdataSetFailed = false;
            AsyncTask::add([&]() { client.sendData(); });
        }
    }
}

void Connections::manageTournaments() {
    for (auto&& tournament : lobby.tournaments) {
        tournament.checkIfStart();
        tournament.checkWaitTime();
        tournament.checkIfScoreWasSent();
    }
    lobby.regularTournaments();
    lobby.saveTournaments();
}

void Connections::manageMatchmaking() {
    while (lobby.matchmaking1vs1.checkQueue(serverClock.getElapsedTime())) lobby.pairMatchmaking();
}

bool Connections::getKey() {
    if (!use_database) return true;

    std::string line;
    std::ifstream file("key");

    if (file.is_open()) {
        getline(file, line);
        serverkey = line;
        getline(file, line);
        challongekey = line;
        return true;
    }
    return false;
}

bool Connections::getUploadData(HumanClient& newclient) {
    for (auto it = uploadData.begin(); it != uploadData.end(); it++) {
        HumanClient& client = *(*it);
        if (client.id == newclient.id && !client.sdataPut) {
            std::cout << "Getting data for " << id << " from uploadData" << std::endl;
            newclient.stats = client.stats;
            newclient.sdataInit = true;
            it = uploadData.erase(it);
            return true;
        }
    }

    return false;
}
