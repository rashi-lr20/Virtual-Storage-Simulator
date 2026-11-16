#ifndef BLOCK_H
#define BLOCK_H

struct Block{
    bool occupied;
    int fileID;   //-1 when free
    Block() : occupied(false), fileID(-1) {}
};

inline char idToChar(int id) {
    static const char* s = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    return s[id % 62];
}

#endif