#include "FreeSegment.h"
#include <cassert>
#include <iostream>

static bool listIs(const FreeList& list, std::initializer_list<int> values) {
    const FreeSegment* p = list.getHead();

    for (auto it = values.begin(); it != values.end(); it += 2) {
        if (!p || p->start != *it || p->size != *(it + 1))
            return false;
        p = p->next;
    }
    return p == nullptr;
}

int main() {
    FreeList list(20);
    assert(listIs(list, {0, 20}));

    FreeSegment* first = list.getHead();
    list.consume(first, 5);
    assert(listIs(list, {5, 15}));

    list.addFreeRun(0, 5);
    assert(listIs(list, {0, 20}));

    // Create two separated free ranges, then bridge them.
    FreeList mergeTest;
    mergeTest.addFreeRun(0, 4);
    mergeTest.addFreeRun(8, 4);
    mergeTest.addFreeRun(4, 4);
    assert(listIs(mergeTest, {0, 12}));

    assert(mergeTest.segmentCount() == 1);
    assert(mergeTest.totalFree() == 12);
    assert(mergeTest.largest() == 12);

    std::cout << "All FreeList tests passed.\n";
    mergeTest.print();
    return 0;
}
