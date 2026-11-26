#include "AllocationManager.h"

FreeSegment* AllocationManager::chooseSegment(
    const FreeList& fl, int n, Strategy strategy) {

    if (n <= 0) return nullptr;

    FreeSegment* chosen = nullptr;

    for (FreeSegment* p = fl.getHead(); p; p = p->next) {
        if (p->size < n) continue;

        if (strategy == Strategy::FIRST_FIT)
            return p;

        if (!chosen) {
            chosen = p;
            continue;
        }

        if (strategy == Strategy::BEST_FIT && p->size < chosen->size)
            chosen = p;

        if (strategy == Strategy::WORST_FIT && p->size > chosen->size)
            chosen = p;
    }

    return chosen;
}

AllocResult AllocationManager::allocate(int fileID, int n) {
    if (n <= 0 || fileID < 0 || fileID >= storage.maxFiles())
        return AllocResult::INVALID_REQUEST;

    if (storage.hasFile(fileID))
        return AllocResult::DUPLICATE_ID;

    FreeSegment* segment = chooseSegment(freeList, n, strategy);

    if (!segment) {
        return freeList.totalFree() >= n
            ? AllocResult::FRAGMENTATION
            : AllocResult::NO_SPACE;
    }

    const int start = segment->start;

    // Keep the array and free-list representation synchronized.
    if (!storage.markBlocks(start, n, fileID))
        return AllocResult::INVALID_REQUEST;

    freeList.consume(segment, n);
    return AllocResult::SUCCESS;
}

bool AllocationManager::release(int fileID) {
    int start = 0;
    int length = 0;

    if (!storage.releaseFile(fileID, start, length))
        return false;

    freeList.addFreeRun(start, length);
    return true;
}
