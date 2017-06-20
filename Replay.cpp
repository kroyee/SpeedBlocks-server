#include "Replay.h"
#include "Client.h"
#include "Connections.h"
#include <fstream>
#include <string>

void Replay::save(std::string filename) {
	if (filename == "") {
		time_t rawtime;
		time(&rawtime);
		filename = std::string("Recordings/") + ctime(&rawtime);
	}
	else
		filename = filename;
	std::ofstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		std::cout << "Error saving recording" << std::endl;
		return;
	}

	file.write((char*)packet.getData(), packet.getDataSize());
	file.close();
}

bool Replay::load(std::string filename) {
	std::ifstream file(filename, std::ios::binary|std::ios::ate);

	if (!file.is_open()) {
		std::cout << "Error loading recording" << std::endl;
		return false;
	}

	size_t size = file.tellg();
    char* memblock = new char [size];
    file.seekg(0, std::ios::beg);
    file.read(memblock, size);
    file.close();

    packet.clear();
    packet.append((void*)memblock, size);

    delete[] memblock;
    return true;
}

void Replay::sendRecording(Client& client) {
	client.conn->packet = packet;
	client.conn->send(client);
}

void Replay::receiveRecording(sf::Packet& pack) {
	packet = pack;
}