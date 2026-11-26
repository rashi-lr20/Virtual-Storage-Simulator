
#ifndef FRAGMENTATION_ANALYZER_H
#define FRAGMENTATION_ANALYZER_H

#include "Common.h"
#include "StorageManager.h"
#include "FreeSegment.h"

struct Stats {
    int totalBlocks = 0;
    int occupiedBlocks = 0;
    int freeBlocks = 0;
    int freeSegments = 0;
    int largestSegment = 0;
    double fragmentation = 0.0;
};

class FragmentationAnalyzer {
private:
    int success = 0;
    int failFrag = 0;
    int failNoSpace = 0;
    int rejected = 0;

public:
    void record(AllocResult r);

    static double fragmentationOf(const FreeList& fl);

    static Stats compute(
        const StorageManager& st,
        const FreeList& fl
    );

    static bool checkConsistency(
        const StorageManager& st,
        const FreeList& fl
    );

    void print(const Stats& s) const;
};

#endif
