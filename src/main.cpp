
#include <iostream>
#include <limits>
#include "Common.h"
#include "StorageManager.h"
#include "FreeSegment.h"
#include "AllocationManager.h"
#include "FragmentationAnalyzer.h"

using namespace std;

// Safely read an integer
int readInt(const string& message) {
    int value;

    while (true) {
        cout << message;

        if (cin >> value)
            return value;

        cout << "Invalid input! Please enter an integer.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

// Select allocation strategy
Strategy selectStrategy() {
    int choice;

    cout << "\nSelect Allocation Strategy:\n";
    cout << "1. First Fit\n";
    cout << "2. Best Fit\n";
    cout << "3. Worst Fit\n";

    while (true) {
        choice = readInt("Enter choice: ");

        switch (choice) {
            case 1: return Strategy::FIRST_FIT;
            case 2: return Strategy::BEST_FIT;
            case 3: return Strategy::WORST_FIT;
            default:
                cout << "Invalid strategy! Choose 1, 2 or 3.\n";
        }
    }
}

int main() {
    cout << "====================================\n";
    cout << "       VIRTUAL STORAGE SIMULATOR\n";
    cout << "====================================\n";

    int totalBlocks, maxFiles;

    do {
        totalBlocks = readInt("Enter total storage blocks: ");
        if (totalBlocks <= 0)
            cout << "Storage blocks must be positive.\n";
    } while (totalBlocks <= 0);

    do {
        maxFiles = readInt("Enter maximum number of files: ");
        if (maxFiles <= 0)
            cout << "Maximum files must be positive.\n";
    } while (maxFiles <= 0);

    // Initialize all three modules
    StorageManager storage(totalBlocks, maxFiles);
    FreeList freeList(totalBlocks);

    Strategy strategy = selectStrategy();

    AllocationManager allocator(storage, freeList, strategy);
    FragmentationAnalyzer analyzer;

    int choice;

    do {
        cout << "\n====================================\n";
        cout << "             MAIN MENU\n";
        cout << "====================================\n";
        cout << "1. Allocate File\n";
        cout << "2. Deallocate File\n";
        cout << "3. Change Allocation Strategy\n";
        cout << "4. Display Storage Layout\n";
        cout << "5. Display Free Segments\n";
        cout << "6. Display Fragmentation Statistics\n";
        cout << "7. Check Consistency\n";
        cout << "8. Exit\n";
        cout << "====================================\n";

        choice = readInt("Enter your choice: ");

        switch (choice) {

            case 1: {
                int fileID = readInt("Enter File ID: ");
                int blocks = readInt("Enter number of blocks required: ");

                AllocResult result = allocator.allocate(fileID, blocks);

                analyzer.record(result);

                cout << "\nAllocation Result: "
                     << resultName(result) << "\n";
                break;
            }

            case 2: {
                int fileID = readInt("Enter File ID to deallocate: ");

                if (allocator.release(fileID)) {
                    cout << "File deallocated successfully!\n";
                } else {
                    cout << "Deallocation failed! File does not exist.\n";
                }
                break;
            }

            case 3: {
                strategy = selectStrategy();
                allocator.setStrategy(strategy);

                cout << "Allocation strategy updated successfully!\n";
                break;
            }

            case 4: {
                cout << "\nSTORAGE LAYOUT\n";
                storage.display();
                break;
            }

            case 5: {
                cout << "\nFREE SEGMENTS\n";
                freeList.print();

                cout << "Total Free Blocks: "
                     << freeList.totalFree() << "\n";

                cout << "Number of Free Segments: "
                     << freeList.segmentCount() << "\n";

                cout << "Largest Free Segment: "
                     << freeList.largest() << "\n";
                break;
            }

            case 6: {
                Stats stats = FragmentationAnalyzer::compute(
                    storage, freeList
                );

                analyzer.print(stats);
                break;
            }

            case 7: {
                if (FragmentationAnalyzer::checkConsistency(
                        storage, freeList)) {
                    cout << "Consistency Check: PASSED\n";
                } else {
                    cout << "Consistency Check: FAILED\n";
                }
                break;
            }

            case 8:
                cout << "\nExiting Virtual Storage Simulator...\n";
                cout << "Thank you!\n";
                break;

            default:
                cout << "Invalid menu choice! Try again.\n";
        }

    } while (choice != 8);

    return 0;
}
