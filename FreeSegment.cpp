#include "FreeSegment.h"
#include <iostream>

FreeList::FreeList(int totalBlocks) : head(nullptr) {
    if (totalBlocks > 0)
        head = new FreeSegment(0, totalBlocks);
}

FreeList::~FreeList() {
    while (head) {
        FreeSegment* current = head;
        head = head->next;
        delete current;
    }
}

void FreeList::consume(FreeSegment* seg, int n) {
    if (!seg || n <= 0 || n > seg->size)
        return;

    seg->start += n;
    seg->size -= n;

    if (seg->size == 0) {
        FreeSegment* prev = nullptr;
        FreeSegment* current = head;

        while (current && current != seg) {
            prev = current;
            current = current->next;
        }

        if (!current) return;

        if (prev) prev->next = current->next;
        else head = current->next;

        delete current;
    }
}

void FreeList::addFreeRun(int start, int n) {
    if (start < 0 || n <= 0) return;

    FreeSegment* prev = nullptr;
    FreeSegment* current = head;

    while (current && current->start < start) {
        prev = current;
        current = current->next;
    }

    // Reject overlapping ranges; callers must only release genuinely free blocks.
    if ((prev && prev->start + prev->size > start) ||
        (current && start + n > current->start)) {
        return;
    }

    const bool joinsPrev =
        prev && prev->start + prev->size == start;
    const bool joinsNext =
        current && start + n == current->start;

    if (joinsPrev && joinsNext) {
        prev->size += n + current->size;
        prev->next = current->next;
        delete current;
    } else if (joinsPrev) {
        prev->size += n;
    } else if (joinsNext) {
        current->start = start;
        current->size += n;
    } else {
        FreeSegment* node = new FreeSegment(start, n, current);
        if (prev) prev->next = node;
        else head = node;
    }
}

int FreeList::segmentCount() const {
    int count = 0;
    for (FreeSegment* p = head; p; p = p->next) ++count;
    return count;
}

int FreeList::totalFree() const {
    int total = 0;
    for (FreeSegment* p = head; p; p = p->next) total += p->size;
    return total;
}

int FreeList::largest() const {
    int largestSize = 0;
    for (FreeSegment* p = head; p; p = p->next)
        if (p->size > largestSize) largestSize = p->size;
    return largestSize;
}

void FreeList::print() const {
    std::cout << "HEAD";
    for (FreeSegment* p = head; p; p = p->next)
        std::cout << " -> [" << p->start << ", " << p->size << "]";
    std::cout << " -> NULL\n";
}
