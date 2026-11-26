#include "WorkloadSimulator.h"
#include "StorageManager.h"
#include "FreeSegment.h"
#include "AllocationManager.h"
#include "FragmentationAnalyzer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <cctype>

static std::string idLabel(int id) { return id < 26 ? std::string(1, (char)('A' + id)) : std::to_string(id); }

int WorkloadSimulator::parseFileId(const std::string& t) {
    if (t.empty()) return -1;
    if (t.size() == 1 && std::isalpha((unsigned char)t[0])) return std::toupper((unsigned char)t[0]) - 'A';
    for (char c : t) if (!std::isdigit((unsigned char)c)) return -1;
    if (t.size() > 5) return -1;
    return std::stoi(t);
}

bool WorkloadSimulator::parseWorkloadFile(const std::string& path, std::vector<Op>& out) {
    std::ifstream in(path);
    if (!in) { std::cerr << "Cannot open " << path << "\n"; return false; }
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        lineNo++;
        std::istringstream ss(line);
        std::string cmd, idTok;
        if (!(ss >> cmd) || cmd[0] == '#') continue;
        for (char& c : cmd) c = (char)std::toupper((unsigned char)c);
        ss >> idTok;
        int id = parseFileId(idTok);
        if (cmd == "ALLOCATE") {
            int size;
            if (id < 0 || !(ss >> size)) { std::cerr << "Line " << lineNo << ": bad ALLOCATE, skipped\n"; continue; }
            out.push_back({true, id, size});
        } else if (cmd == "RELEASE") {
            if (id < 0) { std::cerr << "Line " << lineNo << ": bad RELEASE, skipped\n"; continue; }
            out.push_back({false, id, 0});
        } else {
            std::cerr << "Line " << lineNo << ": unknown command '" << cmd << "', skipped\n";
        }
    }
    return true;
}

std::vector<Op> WorkloadSimulator::generateWorkload(int numOps, unsigned seed, int minSize, int maxSize, double allocProb) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> coin(0.0, 1.0);
    std::uniform_int_distribution<int> sizeDist(minSize, maxSize);
    std::vector<Op> ops;
    std::vector<int> issued;          // ids allocated so far and not yet released (in the request stream)
    int nextId = 0;
    for (int i = 0; i < numOps; i++) {
        if (issued.empty() || coin(rng) < allocProb) {
            ops.push_back({true, nextId, sizeDist(rng)});
            issued.push_back(nextId++);
        } else {
            std::uniform_int_distribution<int> pick(0, (int)issued.size() - 1);
            int k = pick(rng);
            ops.push_back({false, issued[k], 0});
            issued[k] = issued.back();
            issued.pop_back();
        }
    }
    return ops;
}

RunResult WorkloadSimulator::run(const std::vector<Op>& ops, int totalBlocks, Strategy s, bool verbose, bool check) {
    int maxId = 0;
    for (const Op& o : ops) if (o.id > maxId) maxId = o.id;

    StorageManager st(totalBlocks, maxId + 1);
    FreeList fl(totalBlocks);
    AllocationManager am(st, fl, s);
    FragmentationAnalyzer an;

    bool consistent = true;
    double fragSum = 0, timeUs = 0;
    for (const Op& o : ops) {
        auto t0 = std::chrono::steady_clock::now();
        if (o.isAlloc) {
            AllocResult r = am.allocate(o.id, o.size);
            auto t1 = std::chrono::steady_clock::now();
            timeUs += std::chrono::duration<double, std::micro>(t1 - t0).count();
            an.record(r);
            if (verbose) std::cout << "ALLOCATE " << idLabel(o.id) << " (" << o.size << " blocks) -> " << resultName(r) << "\n";
        } else {
            bool ok = am.release(o.id);
            auto t1 = std::chrono::steady_clock::now();
            timeUs += std::chrono::duration<double, std::micro>(t1 - t0).count();
            if (!ok) an.recordBadRelease();
            if (verbose) std::cout << "RELEASE " << idLabel(o.id) << " -> " << (ok ? "released" : "ignored (file not allocated)") << "\n";
        }
        fragSum += FragmentationAnalyzer::fragmentationOf(fl);
        if (check && !FragmentationAnalyzer::checkConsistency(st, fl)) consistent = false;
        if (verbose) { st.display(); fl.print(); std::cout << "\n"; }
    }

    RunResult r;
    r.strategy = s;
    r.success = an.successCount(); r.failFrag = an.fragmentationFailures(); r.failNoSpace = an.noSpaceFailures();
    r.rejected = an.rejectedCount(); r.badReleases = an.badReleaseCount();
    Stats stt = FragmentationAnalyzer::compute(st, fl);
    r.freeBlocks = stt.freeBlocks; r.freeSegments = stt.freeSegments; r.largest = stt.largestSegment;
    r.finalFrag = stt.fragmentation;
    r.avgFrag = ops.empty() ? 0.0 : fragSum / ops.size();
    r.timeUs = timeUs;
    r.consistent = consistent;
    return r;
}

void WorkloadSimulator::printTable(const std::vector<RunResult>& rows) {
    std::cout << std::left << std::setw(11) << "Strategy" << std::right
              << std::setw(9) << "Success" << std::setw(10) << "FailFrag" << std::setw(10) << "FailFull"
              << std::setw(8) << "BadRel" << std::setw(7) << "Free" << std::setw(6) << "Segs"
              << std::setw(9) << "Largest" << std::setw(10) << "FinalFrag" << std::setw(9) << "AvgFrag"
              << std::setw(11) << "Time(us)" << std::setw(6) << "OK?" << "\n";
    for (const RunResult& r : rows) {
        std::cout << std::left << std::setw(11) << strategyName(r.strategy) << std::right
                  << std::setw(9) << r.success << std::setw(10) << r.failFrag << std::setw(10) << r.failNoSpace
                  << std::setw(8) << r.badReleases << std::setw(7) << r.freeBlocks << std::setw(6) << r.freeSegments
                  << std::setw(9) << r.largest << std::setw(10) << std::fixed << std::setprecision(3) << r.finalFrag
                  << std::setw(9) << r.avgFrag << std::setw(11) << std::setprecision(1) << r.timeUs
                  << std::setw(6) << (r.consistent ? "yes" : "NO") << "\n";
    }
}
