default: server

server: main.o network.o PacketCompress.o
	g++ -std=c++11 -pthread main.o network.o PacketCompress.o -o server -lsfml-network -lsfml-system

main.o: main.cpp network.h
	g++ -std=c++11 -c main.cpp -o main.o

network.o: network.cpp network.h
	g++ -std=c++11 -c network.cpp -o network.o

PacketCompress.o: PacketCompress.h PacketCompress.cpp
	g++ -std=c++11 -c PacketCompress.cpp -o PacketCompress.o

run: server
	lxterminal -e /home/super/Documents/programming/server/server