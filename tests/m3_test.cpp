// MEMBER 3 quick test: hand-built layout, checks the analyzer and the consistency checker.
// Build: g++ -std=c++17 -Wall -Wextra -g -Iinclude src/StorageManager.cpp src/FreeSegment.cpp src/FragmentationAnalyzer.cpp tests/m3_test.cpp -o m3_test
#include <iostream>
#include "StorageManager.h"
#include "FreeSegment.h"
#include "FragmentationAnalyzer.h"

int main() {
    // 14 blocks: free 3 | used 1 | free 4 | used 1 | free 5   (the "3,4,5" example)
    StorageManager st(14);
    FreeList fl;                                 // empty list, filled by hand
    st.markBlocks(3, 1, 0);
    st.markBlocks(8, 1, 1);
    fl.addFreeRun(0, 3);  fl.addFreeRun(4, 4);  fl.addFreeRun(9, 5);

    st.display();
    fl.print();

    Stats s = FragmentationAnalyzer::compute(st, fl);
    FragmentationAnalyzer an;
    an.record(AllocResult::SUCCESS);
    an.record(AllocResult::FRAGMENTATION);       // pretend a request of 8 failed
    an.print(s);                                 // expect free=12, segments=3, largest=5

    std::cout << "consistent (expect 1): " << FragmentationAnalyzer::checkConsistency(st, fl) << "\n";
    fl.addFreeRun(3, 1);                         // WRONG on purpose: block 3 is used, the list now lies
    std::cout << "after corrupting the list (expect 0): " << FragmentationAnalyzer::checkConsistency(st, fl) << "\n";
    return 0;
}
