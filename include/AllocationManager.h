#ifndef ALLOCATION_MANAGER_H
#define ALLOCATION_MANAGER_H

#include "Common.h"
#include "StorageManager.h"
#include "FreeSegment.h"

class AllocationManager {
public:
    AllocationManager(StorageManager& st, FreeList& fl, Strategy s)
        : storage(st), freeList(fl), strategy(s) {}

    void setStrategy(Strategy s) { strategy = s; }
    Strategy getStrategy() const { return strategy; }

    AllocResult allocate(int fileID, int n);
    bool release(int fileID);

    // Returns the selected free segment, or nullptr if none can fit n blocks.
    static FreeSegment* chooseSegment(const FreeList& fl, int n, Strategy s);

private:
    StorageManager& storage;
    FreeList& freeList;
    Strategy strategy;
};

#endif
