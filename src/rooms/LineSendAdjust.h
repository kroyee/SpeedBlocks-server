#ifndef LINESENDADJUST_H
#define LINESENDADJUST_H

#include <vector>
#include <list>

class Client;

struct Adjust {
	Adjust(short _players, short _amount) : players(_players), amount(_amount) {}
	short players;
	short amount;
};

class LineSendAdjust {
	std::vector<Adjust> adjust;
public:
	float getAdjust(float linesSent, float amount);
	void addAdjust(Client& died, std::list<Client*>& clients, short players);
	void clear();
};

#endif