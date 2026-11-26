#ifndef FRAGMENTATION_ANALYZER_H
#define FRAGMENTATION_ANALYZER_H
// MEMBER 3: statistics, failure classification, consistency checking.
#include "Common.h"
#include "StorageManager.h"
#include "FreeSegment.h"

struct Stats {
    int    totalBlocks, occupiedBlocks, freeBlocks, freeSegments, largestSegment;
    double fragmentation;          // 1 - largest/free (0 when nothing is free or one segment)
};

class FragmentationAnalyzer {
public:
    FragmentationAnalyzer() : success(0), failFrag(0), failNoSpace(0), rejected(0), badReleases(0) {}

    void record(AllocResult r);            // classify one allocation outcome
    void recordBadRelease() { badReleases++; }

    int successCount() const { return success; }
    int fragmentationFailures() const { return failFrag; }
    int noSpaceFailures() const { return failNoSpace; }
    int rejectedCount() const { return rejected; }
    int badReleaseCount() const { return badReleases; }
    int failedAllocations() const { return failFrag + failNoSpace; }

    static Stats compute(const StorageManager& st, const FreeList& fl);
    static double fragmentationOf(const FreeList& fl);
    // Rebuilds the free runs from the array and compares them with the list node by node.
    static bool checkConsistency(const StorageManager& st, const FreeList& fl);
    void print(const Stats& s) const;

private:
    int success, failFrag, failNoSpace, rejected, badReleases;
};

#endif
