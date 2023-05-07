INCLUDES=-I includes/
CXXFLAGS=-std=c++2a -Wall -Wextra -Werror -pedantic -pthread
GXXFLAGS=-pthread
CXX=g++

.DEFAULT_GOAL := exec
.PHONY: exec debug monitor clean # tests

exec: CXXFLAGS += -O3 $(INCLUDES)
exec: bin/exec

exec: CXXFLAGS += -O3
exec: bin/monitor

debug: CXXFLAGS += -g $(INCLUDES)
debug: bin/debug

# tests: CXXFLAGS += -g
# tests: bin/tests

clean:
	rm -f bin/*

bin/exec: src/main.cpp src/message.cpp src/member.cpp src/utility.cpp src/node.cpp src/socket.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

bin/debug: src/main.cpp src/message.cpp src/member.cpp src/utility.cpp src/node.cpp src/socket.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

bin/monitor: worker.c
	gcc $(GXXFLAGS) $^ -o $@

# bin/tests: tests/unittests.cpp src/client_utils.cpp src/cmdRunner.cpp src/client_socket.cpp src/server_socket.cpp src/socket_utils.cpp
# 	$(CXX) $(CXXFLAGS) $^ -o $@