# Compiler settings
CXX = g++
CXXFLAGS = -Wall -std=c++17

all: servermain serverA serverB serverC

MainServer: servermain.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

ServerA: server.cpp
	$(CXX) $(CXXFLAGS) -DBACKEND_SERVER_NAME=SERVER_A $^ -o $@

ServerB: server.cpp
	$(CXX) $(CXXFLAGS) -DBACKEND_SERVER_NAME=SERVER_B $^ -o $@

ServerC: server.cpp
	$(CXX) $(CXXFLAGS) -DBACKEND_SERVER_NAME=SERVER_C $^ -o $@

runA: ServerA
	./serverA

runB: ServerB
	./serverB

runC: ServerC
	./serverC

runMain: MainServer
	./servermain

clean:
	rm -f servermain serverA serverB serverC

