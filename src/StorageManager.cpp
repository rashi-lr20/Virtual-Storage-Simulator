#include "StorageManager.h"
#include<iostream>
#include<iomanip>

StorageManager::StorageManager(int totalBlocks, int maxFiles): total(totalBlocks), maxFileCount(maxFiles) {
    storage = new Block[total];
    files = new FileEntry[maxFileCount];
}

StorageManager::~StorageManager(){
    delete[] storage;
    delete[] files;
}

bool StorageManager::hasFile(int id) const{ 
    return id>=0 && id<maxFileCount && files[id].exists;
}

bool StorageManager::getFile(int id, int& start, int& length) const{
    if(!hasFile(id)) return false;
    start = files[id].start;
    length = files[id].length;
    return true;
}

bool StorageManager::markBlocks(int start, int n, int id){
    if(id<0 || id>=maxFileCount || files[id].exists) return false;
    if(n<=0 || start<0 || start+n>total) return false;
    for(int i=start; i<start+n; i++)
    if(storage[i].occupied) return false;
    for(int i=start; i<start+n; i++){
        storage[i].occupied = true;
        storage[i].fileID = id;
    }
    files[id].exists = true;
    files[id].start  = start;
    files[id].length = n;
    return true;
}

bool StorageManager::releaseFile(int id, int& start, int& length) {
    if (!getFile(id, start, length)) return false;
    for (int i = start; i < start + length; i++) {
        storage[i].occupied = false;
        storage[i].fileID   = -1;
    }
    files[id].exists = false;
    return true;
}

int StorageManager::occupiedCount() const {
    int c = 0;
    for (int i = 0; i < total; i++) if (storage[i].occupied) c++;
    return c;
}

void StorageManager::display(int perRow) const {
    for (int i = 0; i < total; i++) {
        if (i % perRow == 0) {
            if (i != 0) std::cout << "\n";
            std::cout << std::setw(5) << i << " | ";
        }
        std::cout << "[" << (storage[i].occupied ? idToChar(storage[i].fileID) : ' ') << "]";
    }
    std::cout << "\n";
}
