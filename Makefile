default: server

server: main.o network.o PacketCompress.o MingConverter.o
	g++ -std=c++11 -Wall -Wextra -pedantic -pthread main.o network.o PacketCompress.o MingConverter.o -o server -lsfml-network -lsfml-system

main.o: main.cpp network.h
	g++ -std=c++11 -Wall -Wextra -pedantic -c main.cpp -o main.o

network.o: network.cpp network.h
	g++ -std=c++11 -Wall -Wextra -pedantic -c network.cpp -o network.o

PacketCompress.o: PacketCompress.h PacketCompress.cpp
	g++ -std=c++11 -Wall -Wextra -pedantic -c PacketCompress.cpp -o PacketCompress.o

MingConverter.o: MingConverter.h MingConverter.cpp
	g++ -std=c++11 -Wall -Wextra -pedantic -c MingConverter.cpp -o MingConverter.o

run: server
	lxterminal -e /home/super/Documents/programming/server/server
