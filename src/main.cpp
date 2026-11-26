// MEMBER 4 owns main.cpp. It only wires modules together: no algorithm code lives here.
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Common.h"
#include "StorageManager.h"
#include "FreeSegment.h"
#include "AllocationManager.h"
#include "FragmentationAnalyzer.h"
#include "WorkloadSimulator.h"

static const Strategy ALL[3] = {Strategy::FIRST_FIT, Strategy::BEST_FIT, Strategy::WORST_FIT};

static bool parseStrategy(const std::string& s, Strategy& out) {
    if (s == "first") { out = Strategy::FIRST_FIT; return true; }
    if (s == "best")  { out = Strategy::BEST_FIT;  return true; }
    if (s == "worst") { out = Strategy::WORST_FIT; return true; }
    return false;
}

static void compareAll(const std::vector<Op>& ops, int blocks) {
    std::vector<RunResult> rows;
    for (Strategy s : ALL) rows.push_back(WorkloadSimulator::run(ops, blocks, s, false, true));
    WorkloadSimulator::printTable(rows);
}

// Reproduces the worked example from the project guide.
static void demo() {
    std::vector<Op> ops = {{true,0,5},{true,1,4},{true,2,5},{false,0,0},{true,3,7}};
    std::cout << "Demo: 20 blocks, First Fit. A=5, B=4, C=5, release A, then D=7 (should fail)\n\n";
    WorkloadSimulator::run(ops, 20, Strategy::FIRST_FIT, true, true);
}

static void interactive() {
    int blocks = 40;
    std::cout << "Total blocks (e.g. 40): ";
    if (!(std::cin >> blocks) || blocks <= 0) blocks = 40;
    StorageManager st(blocks);
    FreeList fl(blocks);
    AllocationManager am(st, fl, Strategy::FIRST_FIT);
    FragmentationAnalyzer an;
    while (true) {
        std::cout << "\nStrategy: " << strategyName(am.getStrategy())
                  << "\n1) Allocate  2) Release  3) Show layout  4) Statistics  5) Change strategy  0) Exit\n> ";
        int choice;
        if (!(std::cin >> choice) || choice == 0) break;
        if (choice == 1) {
            std::string idTok; int n;
            std::cout << "File id (A-Z or number) and size: ";
            std::cin >> idTok >> n;
            int id = WorkloadSimulator::parseFileId(idTok);
            AllocResult r = (id < 0) ? AllocResult::INVALID_REQUEST : am.allocate(id, n);
            an.record(r);
            std::cout << resultName(r) << "\n";
        } else if (choice == 2) {
            std::string idTok;
            std::cout << "File id: ";
            std::cin >> idTok;
            int id = WorkloadSimulator::parseFileId(idTok);
            bool ok = id >= 0 && am.release(id);
            if (!ok) an.recordBadRelease();
            std::cout << (ok ? "Released\n" : "No such file\n");
        } else if (choice == 3) {
            st.display(); fl.print();
        } else if (choice == 4) {
            an.print(FragmentationAnalyzer::compute(st, fl));
        } else if (choice == 5) {
            std::string s; std::cout << "first / best / worst: "; std::cin >> s;
            Strategy ns; if (parseStrategy(s, ns)) am.setStrategy(ns); else std::cout << "Unknown strategy\n";
        }
    }
}

static void experiment() {
    struct Cfg { const char* name; int blocks, ops, minS, maxS; };
    Cfg cfgs[3] = {{"small", 50, 20, 1, 6}, {"medium", 500, 200, 1, 20}, {"large", 5000, 2000, 1, 100}};
    const int SEEDS = 5;
    std::ofstream csv("results/results.csv");
    if (!csv) { std::cerr << "Create the results/ folder first.\n"; return; }
    csv << "size,blocks,ops,seed,strategy,success,failFrag,failNoSpace,badReleases,freeBlocks,freeSegments,largest,finalFrag,avgFrag,timeUs,consistent\n";
    for (const Cfg& c : cfgs) {
        std::cout << "\n== " << c.name << ": " << c.blocks << " blocks, " << c.ops << " requests, averaged over " << SEEDS << " seeds ==\n";
        double sum[3][10] = {};
        for (int seed = 1; seed <= SEEDS; seed++) {
            std::vector<Op> ops = WorkloadSimulator::generateWorkload(c.ops, seed, c.minS, c.maxS, 0.6);
            for (int k = 0; k < 3; k++) {
                RunResult r = WorkloadSimulator::run(ops, c.blocks, ALL[k], false, true);
                csv << c.name << "," << c.blocks << "," << c.ops << "," << seed << "," << strategyName(ALL[k]) << ","
                    << r.success << "," << r.failFrag << "," << r.failNoSpace << "," << r.badReleases << ","
                    << r.freeBlocks << "," << r.freeSegments << "," << r.largest << ","
                    << r.finalFrag << "," << r.avgFrag << "," << r.timeUs << "," << (r.consistent ? 1 : 0) << "\n";
                sum[k][0] += r.success; sum[k][1] += r.failFrag; sum[k][2] += r.failNoSpace;
                sum[k][3] += r.freeSegments; sum[k][4] += r.largest; sum[k][5] += r.avgFrag; sum[k][6] += r.timeUs;
                sum[k][7] += r.badReleases; sum[k][8] += r.freeBlocks; sum[k][9] += r.finalFrag;
            }
        }
        std::vector<RunResult> avg;
        for (int k = 0; k < 3; k++) {
            RunResult r{};
            r.strategy = ALL[k];
            r.success = (int)(sum[k][0] / SEEDS + 0.5); r.failFrag = (int)(sum[k][1] / SEEDS + 0.5);
            r.failNoSpace = (int)(sum[k][2] / SEEDS + 0.5); r.freeSegments = (int)(sum[k][3] / SEEDS + 0.5);
            r.largest = (int)(sum[k][4] / SEEDS + 0.5); r.avgFrag = sum[k][5] / SEEDS; r.timeUs = sum[k][6] / SEEDS;
            r.badReleases = (int)(sum[k][7] / SEEDS + 0.5); r.freeBlocks = (int)(sum[k][8] / SEEDS + 0.5); r.finalFrag = sum[k][9] / SEEDS;
            r.consistent = true;
            avg.push_back(r);
        }
        WorkloadSimulator::printTable(avg);
    }
    std::cout << "\nPer-run rows written to results/results.csv\n";
}

static void usage() {
    std::cout << "Usage:\n"
              << "  simulator                              interactive menu\n"
              << "  simulator demo                         worked example with layouts\n"
              << "  simulator run <file> <blocks> [first|best|worst]   run a workload file\n"
              << "  simulator compare <blocks> <ops> <seed>            random workload, all 3 strategies\n"
              << "  simulator experiment                   small/medium/large, writes results/results.csv\n";
}

int main(int argc, char* argv[]) {
    if (argc == 1) { interactive(); return 0; }
    std::string cmd = argv[1];
    if (cmd == "demo") { demo(); return 0; }
    if (cmd == "experiment") { experiment(); return 0; }
    if (cmd == "compare" && argc >= 5) {
        int blocks = std::stoi(argv[2]), n = std::stoi(argv[3]);
        unsigned seed = (unsigned)std::stoul(argv[4]);
        compareAll(WorkloadSimulator::generateWorkload(n, seed, 1, blocks / 12 > 1 ? blocks / 12 : 2, 0.6), blocks);
        return 0;
    }
    if (cmd == "run" && argc >= 4) {
        std::vector<Op> ops;
        if (!WorkloadSimulator::parseWorkloadFile(argv[2], ops)) return 1;
        int blocks = std::stoi(argv[3]);
        Strategy s;
        if (argc >= 5 && parseStrategy(argv[4], s)) WorkloadSimulator::run(ops, blocks, s, true, true);
        else compareAll(ops, blocks);
        return 0;
    }
    usage();
    return 1;
}
