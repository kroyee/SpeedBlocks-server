default: server

server: main.o Connections.o Lobby.o Room.o Client.o PacketCompress.o MingwConvert.o Packets.o Tournament.o
	g++ -std=c++11 -Wall -Wextra -pedantic -pthread main.o Connections.o Lobby.o Room.o Client.o PacketCompress.o MingwConvert.o Packets.o Tournament.o -o server -lsfml-network -lsfml-system

main.o: main.cpp Connections.h Lobby.h Room.h Client.h
	g++ -std=c++11 -Wall -Wextra -pedantic -c main.cpp -o main.o

Connections.o: Connections.cpp Connections.h Lobby.h Room.h Client.h
	g++ -std=c++11 -Wall -Wextra -pedantic -c Connections.cpp -o Connections.o

Lobby.o: Lobby.cpp Lobby.h Room.h Client.h
	g++ -std=c++11 -Wall -Wextra -pedantic -c Lobby.cpp -o Lobby.o

Room.o: Room.cpp Room.h Client.h
	g++ -std=c++11 -Wall -Wextra -pedantic -c Room.cpp -o Room.o

Client.o: Client.cpp Client.h
	g++ -std=c++11 -Wall -Wextra -pedantic -c Client.cpp -o Client.o

PacketCompress.o: PacketCompress.h PacketCompress.cpp
	g++ -std=c++11 -Wall -Wextra -pedantic -c PacketCompress.cpp -o PacketCompress.o

MingwConvert.o: MingwConvert.h MingwConvert.cpp
	g++ -std=c++11 -Wall -Wextra -pedantic -c MingwConvert.cpp -o MingwConvert.o

Packets.o: Packets.cpp Connections.h Client.h Room.h Lobby.h
	g++ -std=c++11 -Wall -Wextra -pedantic -c Packets.cpp -o Packets.o

Tournament.o: Tournament.cpp Tournament.h
	g++ -std=c++11 -Wall -Wextra -pedantic -c Tournament.cpp -o Tournament.o

run: server
	lxterminal -e /home/super/Documents/programming/server/server
