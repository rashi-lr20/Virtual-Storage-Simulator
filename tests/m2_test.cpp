#include <iostream>
#include "FreeSegment.h"
#include "AllocationManager.h"

int main() {
    std::cout << "--- Merge cases ---\n";
    FreeList fl;                               // empty list
    fl.addFreeRun(4, 4);   fl.print();         // [4,4]
    fl.addFreeRun(12, 4);  fl.print();         // [4,4] -> [12,4]   (isolated)
    fl.addFreeRun(8, 4);   fl.print();         // both sides touch -> [4,12]
    fl.addFreeRun(0, 4);   fl.print();         // touches next    -> [0,16]
    fl.addFreeRun(16, 4);  fl.print();         // touches prev    -> [0,20]

    std::cout << "\n--- consume ---\n";
    FreeList f2(20);       f2.print();         // [0,20]
    f2.consume(f2.getHead(), 5);  f2.print();  // [5,15]
    f2.consume(f2.getHead(), 15); f2.print();  // list empty

    std::cout << "\n--- Strategies: gaps 3, 8, 5, 12 and a request of 4 ---\n";
    FreeList g;
    g.addFreeRun(0, 3); g.addFreeRun(4, 8); g.addFreeRun(13, 5); g.addFreeRun(19, 12);
    g.print();
    std::cout << "First Fit  -> size " << AllocationManager::chooseSegment(g, 4, Strategy::FIRST_FIT)->size << "  (expect 8)\n";
    std::cout << "Best Fit   -> size " << AllocationManager::chooseSegment(g, 4, Strategy::BEST_FIT)->size  << "  (expect 5)\n";
    std::cout << "Worst Fit  -> size " << AllocationManager::chooseSegment(g, 4, Strategy::WORST_FIT)->size << "  (expect 12)\n";
    std::cout << "Request 13 -> " << (AllocationManager::chooseSegment(g, 13, Strategy::FIRST_FIT) ? "found" : "none") << "  (expect none)\n";
    return 0;
}