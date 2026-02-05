.PHONY: all clean

all: server client


server: TcpServer.cc
	g++ -o $@ $^ -std=c++11 -lpthread -ljsoncpp

client: TcpClient.cc
	g++ -o $@ $^ -std=c++11 -lpthread -ljsoncpp

clean:
	rm -f server client