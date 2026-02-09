CXX = g++
CXXFLAGS = -Wall -std=c++17 -lpthread
TARGET = httpserver
SRC = HttpServer.cc 

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) -o $@ $^ $(CXXFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGET)