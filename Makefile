all: client server

server.o: server.cpp net.hpp
	g++ -c server.cpp

server: server.o
	g++ -o server server.o

client.o: client.cpp net.hpp
	g++ -c client.cpp

client: client.o
	g++ -o client client.o

clean:
	rm server client *.o	