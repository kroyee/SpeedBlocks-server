#include "LineSendAdjust.h"
#include "Client.h"

float LineSendAdjust::getAdjust(float sent, float sending) {
    float lineAdjust = 0;
    for (auto&& adj : adjust) {
        if (sent >= adj.amount)
            continue;
        else if (adj.players == 0)
            continue;
        if (adj.amount - sent < sending) {
            lineAdjust += (float)(adj.amount - sent) / (float)adj.players;
            sent += (float)(adj.amount - sent) / (float)adj.players;
            sending -= (float)(adj.amount - sent) / (float)adj.players;
        } else {
            lineAdjust += sending / (float)adj.players;
            sent += sending / (float)adj.players;
            sending -= sending / (float)adj.players;
        }
    }
    return lineAdjust;
}

void LineSendAdjust::addAdjust(Client& died, std::list<Client*>& clients, short players) {
    short second = 0, amount = 0;
    bool leaderDied = false;
    for (auto&& client : clients) {
        if (client->roundStats.score.lines_blocked > amount) {
            second = amount;
            amount = client->roundStats.score.lines_blocked;
            if (died.id == client->id)
                leaderDied = true;
            else
                leaderDied = false;
        } else if (client->roundStats.score.lines_blocked > second)
            second = client->roundStats.score.lines_blocked;
    }
    if (leaderDied) amount = second;
    adjust.emplace_back(players, amount);
}

void LineSendAdjust::clear() { adjust.clear(); }