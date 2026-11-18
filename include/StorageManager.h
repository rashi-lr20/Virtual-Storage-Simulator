#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include "Block.h"

struct FileEntry {
    bool exists;
    int start;
    int length;
    FileEntry() : exists(false), start(0), length(0) {}
};

class StorageManager {
    public:
    explicit StorageManager(int totalBlocks, int maxFiles = 100000);
    ~StorageManager();
    StorageManager(const StorageManager&) = delete;
    StorageManager& operator=(const StorageManager&) = delete;

    int size() const {return total;}
    int maxFiles() const {return maxFileCount;}
    const Block& at(int i) const {return storage[i];}

    bool hasFile(int id) const;
    bool getFile(int id, int& start, int& length) const;

    bool markBlocks(int start, int n, int id);

    bool releaseFile(int id, int& start, int&length);

    int occupiedCount() const;
    void display(int perRow = 20)
    const;

    private:
    int total;
    int maxFileCount;
    Block* storage;
    FileEntry* files;
};

#endif