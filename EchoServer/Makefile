.PHONY:all
all:udp_client udp_server

udp_client:UdpClient.cc
	g++ -o $@ $^ -std=c++17

udp_server:UdpServer.cc
	g++ -o $@ $^ -std=c++17

.PHONY:clean
clean:
	rm -f udp_client udp_server