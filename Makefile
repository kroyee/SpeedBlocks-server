default: server

server: main.o network.o
	g++ -std=c++0x -pthread main.o network.o -o server -lsfml-network -lsfml-system

main.o: main.cpp network.h
	g++ -std=c++0x -c main.cpp -o main.o

network.o: network.cpp network.h
	g++ -std=c++0x -c network.cpp -o network.o

run: server
	lxterminal -e /home/super/Documents/programming/server/server