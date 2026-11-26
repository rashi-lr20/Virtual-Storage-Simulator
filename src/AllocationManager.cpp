#include "AllocationManager.h"

FreeSegment* AllocationManager::chooseSegment(const FreeList& fl, int n, Strategy s) {
    FreeSegment* chosen = nullptr;
    for (FreeSegment* p = fl.getHead(); p; p = p->next) {          // O(F)
        if (p->size < n) continue;
        if (s == Strategy::FIRST_FIT) return p;                    // first big enough
        if (!chosen) { chosen = p; continue; }
        if (s == Strategy::BEST_FIT  && p->size < chosen->size) chosen = p;   // smallest that fits
        if (s == Strategy::WORST_FIT && p->size > chosen->size) chosen = p;   // largest
    }
    return chosen;
}

AllocResult AllocationManager::allocate(int fileID, int n) {
    if (n <= 0 || fileID < 0 || fileID >= storage.maxFiles()) return AllocResult::INVALID_REQUEST;
    if (storage.hasFile(fileID)) return AllocResult::DUPLICATE_ID;

    FreeSegment* seg = chooseSegment(freeList, n, strategy);
    if (!seg)
        return freeList.totalFree() >= n ? AllocResult::FRAGMENTATION : AllocResult::NO_SPACE;

    int start = seg->start;                 // read before consume() changes the node
    storage.markBlocks(start, n, fileID);   // 1) update the array
    freeList.consume(seg, n);               // 2) update the free list
    return AllocResult::SUCCESS;
}

bool AllocationManager::release(int fileID) {
    int start, length;
    if (!storage.releaseFile(fileID, start, length)) return false;   // 1) array
    freeList.addFreeRun(start, length);                              // 2) list + merge
    return true;
}
