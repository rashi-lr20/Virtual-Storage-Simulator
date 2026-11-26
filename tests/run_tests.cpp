// Minimal test harness (no framework needed). Build with:  make test
#include <iostream>
#include <cmath>
#include <cmath>
#include "AllocationManager.h"
#include "FragmentationAnalyzer.h"
#include "WorkloadSimulator.h"

static int passed = 0, failed = 0;
#define CHECK(name, cond) do { if (cond) { passed++; } else { failed++; std::cout << "FAIL: " << name << "  (line " << __LINE__ << ")\n"; } } while (0)

struct Env {
    StorageManager st; FreeList fl; AllocationManager am;
    Env(int n, Strategy s = Strategy::FIRST_FIT) : st(n, 1000), fl(n), am(st, fl, s) {}
    bool ok() { return FragmentationAnalyzer::checkConsistency(st, fl); }
};

static bool listIs(const FreeList& fl, std::initializer_list<int> flat) {
    const FreeSegment* p = fl.getHead();
    for (auto it = flat.begin(); it != flat.end(); it += 2) {
        if (!p || p->start != *it || p->size != *(it + 1)) return false;
        p = p->next;
    }
    return p == nullptr;
}

void testNormal() {
    Env e(20);
    CHECK("alloc A=5 succeeds", e.am.allocate(0, 5) == AllocResult::SUCCESS);
    CHECK("list is [5,15]", listIs(e.fl, {5, 15}));
    CHECK("consistent", e.ok());
    CHECK("release A ok", e.am.release(0));
    CHECK("list back to [0,20]", listIs(e.fl, {0, 20}));
}

void testBoundary() {
    Env e(20);
    CHECK("alloc whole storage", e.am.allocate(0, 20) == AllocResult::SUCCESS);
    CHECK("free list empty", e.fl.getHead() == nullptr && e.fl.totalFree() == 0);
    CHECK("next request has no space", e.am.allocate(1, 1) == AllocResult::NO_SPACE);
    Env one(1);
    CHECK("1-block storage", one.am.allocate(0, 1) == AllocResult::SUCCESS && one.ok());
}

void testFailures() {
    Env e(20);
    CHECK("oversized 21 of 20", e.am.allocate(0, 21) == AllocResult::NO_SPACE);
    CHECK("size 0 rejected", e.am.allocate(0, 0) == AllocResult::INVALID_REQUEST);
    CHECK("negative size rejected", e.am.allocate(0, -3) == AllocResult::INVALID_REQUEST);
    CHECK("negative id rejected", e.am.allocate(-1, 3) == AllocResult::INVALID_REQUEST);
    e.am.allocate(1, 4);
    CHECK("duplicate id rejected", e.am.allocate(1, 2) == AllocResult::DUPLICATE_ID);
    CHECK("release unknown id", !e.am.release(7));
    CHECK("state unchanged after failures", listIs(e.fl, {4, 16}) && e.ok());
}

void testMergeCases() {
    Env e(20);
    for (int i = 0; i < 5; i++) e.am.allocate(i, 4);
    e.am.release(1);
    CHECK("isolated", listIs(e.fl, {4, 4}) && e.ok());
    e.am.release(3);
    CHECK("two nodes", listIs(e.fl, {4, 4, 12, 4}) && e.ok());
    e.am.release(2);
    CHECK("merge both", listIs(e.fl, {4, 12}) && e.ok());
    e.am.release(0);
    CHECK("merge next", listIs(e.fl, {0, 16}) && e.ok());
    e.am.release(4);
    CHECK("merge prev", listIs(e.fl, {0, 20}) && e.ok());
}

void testFragmentationFailure() {
    Env e(14);
    e.am.allocate(0, 3); e.am.allocate(1, 1);
    e.am.allocate(2, 4); e.am.allocate(3, 1);
    e.am.allocate(4, 5);
    e.am.release(0); e.am.release(2); e.am.release(4);
    CHECK("segments 3,4,5", listIs(e.fl, {0, 3, 4, 4, 9, 5}));
    CHECK("total free is 12", e.fl.totalFree() == 12 && e.fl.largest() == 5);
    CHECK("request 8 fails by fragmentation", e.am.allocate(9, 8) == AllocResult::FRAGMENTATION);
    CHECK("fragmentation is 1 - 5/12", std::abs(FragmentationAnalyzer::fragmentationOf(e.fl) - (1.0 - 5.0 / 12)) < 1e-9);
    CHECK("state unchanged after failed request", listIs(e.fl, {0, 3, 4, 4, 9, 5}) && e.ok());
}

void testStrategies() {
    FreeList fl;
    fl.addFreeRun(0, 3); fl.addFreeRun(4, 8);
    fl.addFreeRun(13, 5); fl.addFreeRun(19, 12);
    CHECK("first fit picks 8", AllocationManager::chooseSegment(fl, 4, Strategy::FIRST_FIT)->size == 8);
    CHECK("best fit picks 5", AllocationManager::chooseSegment(fl, 4, Strategy::BEST_FIT)->size == 5);
    CHECK("worst fit picks 12", AllocationManager::chooseSegment(fl, 4, Strategy::WORST_FIT)->size == 12);
    CHECK("no segment for 13", AllocationManager::chooseSegment(fl, 13, Strategy::BEST_FIT) == nullptr);
    FreeList tie;
    tie.addFreeRun(0, 5); tie.addFreeRun(10, 5);
    CHECK("best fit tie -> lowest start", AllocationManager::chooseSegment(tie, 5, Strategy::BEST_FIT)->start == 0);
    CHECK("worst fit tie -> lowest start", AllocationManager::chooseSegment(tie, 5, Strategy::WORST_FIT)->start == 0);
}

void testWorkloads() {
    auto a = WorkloadSimulator::generateWorkload(200, 42, 1, 40, 0.6);
    auto b = WorkloadSimulator::generateWorkload(200, 42, 1, 40, 0.6);
    bool same = a.size() == b.size();
    for (size_t i = 0; same && i < a.size(); i++)
        same = a[i].isAlloc == b[i].isAlloc && a[i].id == b[i].id && a[i].size == b[i].size;
    CHECK("same seed gives same workload", same);

    for (Strategy s : {Strategy::FIRST_FIT, Strategy::BEST_FIT, Strategy::WORST_FIT}) {
        RunResult r = WorkloadSimulator::run(a, 500, s, false, true);
        CHECK("array and list agree after every step", r.consistent);
        CHECK("accounting adds up", r.freeBlocks >= 0 && r.freeBlocks <= 500);
    }

    auto big = WorkloadSimulator::generateWorkload(2000, 7, 1, 400, 0.6);
    CHECK("large workload consistent", WorkloadSimulator::run(big, 5000, Strategy::BEST_FIT, false, true).consistent);
    CHECK("parseFileId letters/numbers", WorkloadSimulator::parseFileId("C") == 2 && WorkloadSimulator::parseFileId("17") == 17
          && WorkloadSimulator::parseFileId("x1") == -1 && WorkloadSimulator::parseFileId("") == -1);
}

int main() {
    testNormal(); testBoundary(); testFailures(); testMergeCases();
    testFragmentationFailure(); testStrategies(); testWorkloads();
    std::cout << passed << " checks passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
