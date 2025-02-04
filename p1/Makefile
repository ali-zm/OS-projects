s.out, c.out: server.o room.o player.o client.o
	g++ server.o room.o player.o -o s.out
	g++ client.o -o c.out
client.o: client.cpp room.cpp room.hpp manual.hpp
	g++ -c client.cpp
player.o: player.cpp player.hpp
	g++ -c player.cpp
room.o: room.cpp room.hpp manual.hpp
	g++ -c room.cpp
server.o: server.cpp player.cpp room.cpp player.hpp room.hpp manual.hpp
	g++ -c server.cpp
clean:
	rm *.o