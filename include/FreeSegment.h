#ifndef FREE_SEGMENT_H
#define FREE_SEGMENT_H

// A free contiguous range of blocks.
struct FreeSegment {
    int start;
    int size;
    FreeSegment* next;

    FreeSegment(int s, int z, FreeSegment* n = nullptr)
        : start(s), size(z), next(n) {}
};

// Sorted singly linked list of free ranges.
// Invariant: nodes do not overlap and adjacent ranges are merged.
class FreeList {
public:
    explicit FreeList(int totalBlocks = 0);
    ~FreeList();

    FreeList(const FreeList&) = delete;
    FreeList& operator=(const FreeList&) = delete;

    FreeSegment* getHead() const { return head; }

    // Consume the first n blocks from a segment. The segment may be deleted.
    void consume(FreeSegment* seg, int n);

    // Insert [start, start+n), merging with adjacent ranges.
    void addFreeRun(int start, int n);

    int segmentCount() const;
    int totalFree() const;
    int largest() const;
    void print() const;

private:
    FreeSegment* head;
};

#endif
