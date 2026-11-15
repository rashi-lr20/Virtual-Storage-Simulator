#pragma once
struct Block{
    bool occupied;
    int fileID;   //-1 when lock is free
    Block() : occupied(false), fileID(-1) {}
};