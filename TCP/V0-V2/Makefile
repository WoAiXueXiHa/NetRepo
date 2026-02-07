.PHONY:all
all:tcp_client tcp_server

tcp_client:TcpClient.cc
	g++ -o $@ $^ -std=c++17 -lpthread

tcp_server:TcpServer.cc
	g++ -o $@ $^ -std=c++17 -lpthread

.PHONY:clean
clean:
	rm -f tcp_client tcp_server