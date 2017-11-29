#include "PingHandle.h"
#include "Connections.h"

void PingHandle::get(const sf::Time& t, Client& client) {
	sf::Uint8 pingId;
	client.conn->packet >> pingId;

	for (auto& packet : packets) {
		if (packet.id == pingId) {
			packet.received = t;
			packet.ping = (packet.received-packet.sent).asMilliseconds();
			packet.returned = true;
			return;
		}
	}
	client.conn->sendUDP(client);
	PingPacket packet;
	packet.id=pingId;
	packet.sent=t;
	packetCount++;
	packets.push_front(packet);
	if (packetCount > 16) {
		packets.pop_back();
		packetCount--;
	}
}

int PingHandle::getAverage() {
	int count, totalPing;
	for (auto& packet : packets)
		if (packet.returned) {
			count++;
			totalPing+=packet.ping;
		}
	if (!count)
		return 0;
	return totalPing/count;
}

float PingHandle::getPacketLoss(const sf::Time& t) {
	bool ret=false;
	int total, dropped;
	for (auto& packet : packets) {
		if (!ret) {
			if (packet.returned)
				ret=true;
			else if (t - packet.sent > sf::milliseconds(1000))
				ret=true;
		}
		if (ret) {
			if (packet.returned)
				total++;
			else {
				total++;
				dropped++;
			}
		}
	}
	if (!total)
		return 0;
	return (float)dropped/(float)total;
}

int PingHandle::getLowest() {
	int lowest=100000;
	for (auto& packet : packets)
		if (packet.returned)
			if (packet.ping < lowest)
				lowest=packet.ping;
	return lowest;
}