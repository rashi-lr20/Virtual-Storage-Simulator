#ifndef WORKLOAD_SIMULATOR_H
#define WORKLOAD_SIMULATOR_H
// MEMBER 4: workloads, running one workload per strategy, results.
#include <string>
#include <vector>
#include "Common.h"

struct Op {
    bool isAlloc;
    int  id;
    int  size;       // ignored for RELEASE
};

struct RunResult {
    Strategy strategy;
    int    success, failFrag, failNoSpace, rejected, badReleases;
    int    freeBlocks, freeSegments, largest;
    double finalFrag, avgFrag;     // fragmentation at the end / averaged over all steps
    double timeUs;                 // time spent inside allocate/release only
    bool   consistent;             // array and free list agreed after every step
};

class WorkloadSimulator {
public:
    // "A".."Z" -> 0..25, otherwise a non-negative integer below 100000. Returns -1 if invalid.
    static int parseFileId(const std::string& token);

    // Lines: ALLOCATE <id> <size> | RELEASE <id> | # comment. Bad lines are reported and skipped.
    static bool parseWorkloadFile(const std::string& path, std::vector<Op>& out);

    // Same seed => same workload, so every strategy is compared on identical input.
    static std::vector<Op> generateWorkload(int numOps, unsigned seed, int minSize, int maxSize, double allocProb);

    static RunResult run(const std::vector<Op>& ops, int totalBlocks, Strategy s, bool verbose, bool checkEveryStep);

    static void printTable(const std::vector<RunResult>& rows);
};

#endif
