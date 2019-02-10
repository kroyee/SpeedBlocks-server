#ifndef CONNECTTIONS_H
#define CONNECTTIONS_H
#include <SFML/Network.hpp>
#include <iostream>
#include <list>
#include <memory>
#include <thread>
#include "Lobby.h"
#include "PacketCompress.h"

class Connections {
   public:
    Connections(bool use_database = true);

    bool use_database = true;

    std::list<std::shared_ptr<HumanClient>> clients;

    std::list<std::shared_ptr<HumanClient>> uploadData;

    unsigned short tcpPort;

    uint8_t id;
    sf::TcpListener listener;
    sf::SocketSelector selector;
    sf::UdpSocket udpSock;
    sf::Socket::Status status;
    std::string serverkey, challongekey;

    sf::Clock serverClock;

    HumanClient* sender;

    sf::IpAddress udpAdd;
    unsigned short udpPort;

    uint16_t clientVersion;

    uint16_t clientCount;

    Lobby lobby;

    bool setUpListener();
    void listen();
    void receive();
    void disconnectClient(std::shared_ptr<HumanClient>&);
    void send(Client&);
    void send(Client&, HumanClient&);
    void send(Room&);
    void send(Room&, short);
    void sendUDP(HumanClient& client, PM& packet);

    void sendWelcomeMsg();
    void sendAuthResult(uint8_t authresult, Client& client);
    void sendChatMsg(const NP_ChatMsg&);
    void sendClientJoinedServerInfo(HumanClient& client);
    void sendClientLeftServerInfo(HumanClient& client);

    void validateClient(const NP_LoginRequest&);
    void validateUDP(const NP_ConfirmUdp&);
    void getGamestate(const NP_Gamestate&);

    void manageRooms();
    void manageClients();
    void manageUploadData();
    void manageTournaments();
    void manageMatchmaking();

    void handlePacket(sf::Packet& packet);

    bool getKey();
    bool getUploadData(HumanClient&);

    template <class F>
    auto serialize(F f) {
        NP_ClientList list;
        for (auto& client : lobby.aiManager.getBots()) list.clients.push_back({client.first, client.second});
        for (auto&& client : clients) list.clients.push_back({client->id, client->name});
        return f(list);
    }
};

#endif
