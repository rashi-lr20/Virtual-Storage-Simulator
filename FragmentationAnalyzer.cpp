#include "FragmentationAnalyzer.h"
#include <iostream>
#include <iomanip>

void FragmentationAnalyzer::record(AllocResult r) {
    switch (r) {
        case AllocResult::SUCCESS:       success++;     break;
        case AllocResult::FRAGMENTATION: failFrag++;    break;
        case AllocResult::NO_SPACE:      failNoSpace++; break;
        default:                         rejected++;    break;
    }
}

double FragmentationAnalyzer::fragmentationOf(const FreeList& fl) {
    int f = fl.totalFree();
    if (f == 0) return 0.0;
    return 1.0 - (double)fl.largest() / f;
}

Stats FragmentationAnalyzer::compute(const StorageManager& st, const FreeList& fl) {
    Stats s;
    s.totalBlocks    = st.size();
    s.occupiedBlocks = st.occupiedCount();     // from the array
    s.freeBlocks     = fl.totalFree();         // from the list
    s.freeSegments   = fl.segmentCount();
    s.largestSegment = fl.largest();
    s.fragmentation  = fragmentationOf(fl);
    return s;
}

bool FragmentationAnalyzer::checkConsistency(const StorageManager& st, const FreeList& fl) {
    const FreeSegment* seg = fl.getHead();
    int n = st.size(), i = 0, freeCount = 0;
    while (i < n) {
        if (!st.at(i).occupied) {
            int j = i;
            while (j < n && !st.at(j).occupied) j++;      // maximal free run [i, j)
            if (!seg || seg->start != i || seg->size != j - i) return false;
            freeCount += j - i;
            seg = seg->next;
            i = j;
        } else {
            i++;
        }
    }
    return seg == nullptr && freeCount == fl.totalFree() && freeCount + st.occupiedCount() == n;
}

void FragmentationAnalyzer::print(const Stats& s) const {
    std::cout << "=================================\n"
              << "STORAGE STATISTICS\n"
              << "=================================\n"
              << "Total Blocks       : " << s.totalBlocks << "\n"
              << "Occupied Blocks    : " << s.occupiedBlocks << "\n"
              << "Free Blocks        : " << s.freeBlocks << "\n"
              << "Free Segments      : " << s.freeSegments << "\n"
              << "Largest Segment    : " << s.largestSegment << "\n"
              << "Fragmentation      : " << std::fixed << std::setprecision(3) << s.fragmentation << "\n"
              << "Successful Allocs  : " << success << "\n"
              << "Failed (fragment.) : " << failFrag << "\n"
              << "Failed (no space)  : " << failNoSpace << "\n"
              << "Rejected requests  : " << rejected << "\n"
              << "=================================\n";
}
