#include "GameSignals.h"

//Network
Signal<void, int, int, int> Signals::SendSignal;
Signal<void, sf::Packet&> Signals::SendPacket;
Signal<void, sf::Packet&> Signals::SendPacketUDP;
Signal<void, int, int> Signals::SendPing;

//Game
Signal<void, int> Signals::GameAddDelay;

//AI
Signal<void, int, int> Signals::DistributeLinesLocally;
Signal<void, uint8_t> Signals::AmountAI;
Signal<void, uint16_t> Signals::SpeedAI;
Signal<void, uint16_t, uint16_t> Signals::SeedRander;

// Packet delegation

std::vector<Signal<void, sf::Packet&>> Net::packetReceivers;
std::vector<std::unique_ptr<SignalHolder>> Net::signalReceivers;

bool Net::passOnPacket(std::size_t id, sf::Packet& packet) {
	if (id < packetReceivers.size()) {
		packetReceivers[id](packet);
		return true;
	}
	return false;
}