CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude
LIB_SRC  = src/StorageManager.cpp src/FreeSegment.cpp src/AllocationManager.cpp src/FragmentationAnalyzer.cpp src/WorkloadSimulator.cpp

# Windows needs the winsock libraries for the GUI server; Linux/Mac need -pthread
ifeq ($(OS),Windows_NT)
  NETLIBS = -lws2_32 -lwsock32
else
  NETLIBS = -pthread
endif

all: simulator gui

gui: gui_server

gui_server: $(LIB_SRC) src/server.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ $(NETLIBS)

simulator: $(LIB_SRC) src/main.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

tests_bin: $(LIB_SRC) tests/run_tests.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

test: tests_bin
	./tests_bin

clean:
	rm -f simulator tests_bin gui_server simulator.exe tests_bin.exe gui_server.exe
