CXX = g++
CXXFLAGS = -std=c++17 -g -Wall

TARGET = httpserver
SRC = HttpServer.cc

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread

clean:
	rm -f $(TARGET)
