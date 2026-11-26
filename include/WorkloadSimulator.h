#ifndef WORKLOAD_SIMULATOR_H
#define WORKLOAD_SIMULATOR_H

#include <string>
#include <vector>
#include "Common.h"

struct Op {
    bool isAlloc;
    int  id;
    int  size;     

struct RunResult {
    Strategy strategy;
    int    success, failFrag, failNoSpace, rejected, badReleases;
    int    freeBlocks, freeSegments, largest;
    double finalFrag, avgFrag;     
    double timeUs;                 
    bool   consistent;             
};

class WorkloadSimulator {
public:
    
    static int parseFileId(const std::string& token);

    
    static bool parseWorkloadFile(const std::string& path, std::vector<Op>& out);

 
    static std::vector<Op> generateWorkload(int numOps, unsigned seed, int minSize, int maxSize, double allocProb);

    static RunResult run(const std::vector<Op>& ops, int totalBlocks, Strategy s, bool verbose, bool checkEveryStep);

    static void printTable(const std::vector<RunResult>& rows);
};
