FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y g++ && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY include ./include
COPY src ./src
COPY web ./web

RUN g++ -std=c++17 -O2 -Wall -Wextra -Iinclude \
    src/StorageManager.cpp \
    src/FreeSegment.cpp \
    src/AllocationManager.cpp \
    src/FragmentationAnalyzer.cpp \
    src/WorkloadSimulator.cpp \
    src/Server.cpp \
    -pthread -o gui_server

CMD ["./gui_server"]
