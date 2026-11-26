#include "../AllocationManager.h"
#include <cassert>
#include <iostream>

int main() {
    // Test First Fit
    {
        StorageManager storage(20);
        FreeList freeList(20);
        AllocationManager manager(storage, freeList, Strategy::FIRST_FIT);

        assert(manager.allocate(1, 5) == AllocResult::SUCCESS);

        int start = -1, length = -1;
        assert(storage.getFile(1, start, length));
        assert(start == 0 && length == 5);
        assert(manager.release(1));
        assert(freeList.totalFree() == 20);
    }

    // Test Best Fit
    {
        FreeList freeList;
        freeList.addFreeRun(0, 5);
        freeList.addFreeRun(10, 8);
        freeList.addFreeRun(25, 12);

        FreeSegment* chosen =
            AllocationManager::chooseSegment(
                freeList, 7, Strategy::BEST_FIT);

        assert(chosen != nullptr);
        assert(chosen->start == 10);
    }

    // Test Worst Fit
    {
        FreeList freeList;
        freeList.addFreeRun(0, 5);
        freeList.addFreeRun(10, 8);
        freeList.addFreeRun(25, 12);

        FreeSegment* chosen =
            AllocationManager::chooseSegment(
                freeList, 7, Strategy::WORST_FIT);

        assert(chosen != nullptr);
        assert(chosen->start == 25);
    }

    std::cout << "All AllocationManager tests passed.\n";
    return 0;
}