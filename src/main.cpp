//#define NO_DATABASE

#include <SFML/Network.hpp>
#include <iostream>
#include <thread>
#include "AsyncTask.h"
#include "Connections.h"
#include "TaskQueue.h"
#include "Tournament.h"
using std::cout;
using std::endl;

bool quit = false;
bool status = false;

void getInput() {
    std::string s;
    while (!quit) {
        std::cin >> s;
        if (s == "quit")
            quit = true;
        else if (s == "status")
            status = true;
    }
}

static int use_database = false;

int main() {
    srand(time(NULL));
    StatsHolder::mapStringsToVariables();

    Connections conn(use_database);
    conn.lobby.loadTournaments();

    if (!conn.getKey()) {
        cout << "Failed to get serverkey" << endl;
        return 1;
    }

    if (!conn.setUpListener()) return 1;

    std::thread t(&getInput);

    cout << "Listener set up" << endl;

    conn.lobby.idcount = 1;  // Adding permanent rooms
    conn.lobby.addRoom("Ranked FFA", 0, 1, 3);
    conn.lobby.addRoom("Ranked Hero", 0, 2, 3);
    conn.lobby.addRoom("Casual", 0, 3, 3);
    conn.lobby.aiManager.add(*conn.lobby.rooms.back()).setSpeed(100).setCompetative(Competative::Low);
    conn.lobby.aiManager.add(*conn.lobby.rooms.back()).setSpeed(100).setCompetative(Competative::High);
    conn.lobby.idcount = 10;

    conn.lobby.setMsg(
        "Press Enter at any time to activate the chat. Press Enter again to send a message or Esc to deactivate it. Press TAB while the chat is active to change where "
        "the message will go to, Room, Lobby or latest priv (shown next to chatbox). Use /w nickname to send private msg.\n\nYou can find some new visual options under "
        "the Visual tab. Including disabling that the menu reacts to the mouse.\nAPM in the score screen shows sent+blocked per minute.\n\nEnjoy! :-)");

    while (!quit) {
        conn.listen();

        conn.manageRooms();
        conn.manageClients();
        conn.manageTournaments();
        conn.manageMatchmaking();
        conn.lobby.challengeHolder.saveChallenges();
        AsyncTask::check();
        TaskQueue::perform(0);
        if (status) {
            for (auto&& client : conn.clients) {
                cout << client->id << ": " << client->name;
                if (client->room != nullptr) cout << " in room " << client->room->name;
                cout << endl;
            }
            status = false;
        }
    }

    for (auto&& it : conn.clients) {
        it->disconnect();
    }
    conn.listener.close();
    conn.udpSock.unbind();

    t.join();
    AsyncTask::exit();
    conn.lobby.aiManager.clear();

    for (auto& room : conn.lobby.rooms) room->endRound();
    return 0;
}
