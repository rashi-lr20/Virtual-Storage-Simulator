# Member 2 — Allocation Engine

This folder contains Member 2's contribution to the Virtual Storage Simulator:

- Singly linked list of free contiguous segments.
- Insertion and merging of neighbouring free ranges.
- Consuming free space after allocation.
- First Fit, Best Fit and Worst Fit selection.
- Allocation/release coordination with Member 1's `StorageManager`.

## Folder structure

```text
member2_allocation_engine/
├── include/
│   ├── FreeSegment.h
│   └── AllocationManager.h
├── src/
│   ├── FreeSegment.cpp
│   └── AllocationManager.cpp
├── tests/
│   └── freelist_test.cpp
└── README.md
```

## 1. Test the linked list independently

This test does not need the other members' files.

### Windows (PowerShell)

```powershell
g++ -std=c++17 -Wall -Wextra -Iinclude src/FreeSegment.cpp tests/freelist_test.cpp -o freelist_test.exe
.\freelist_test.exe
```

### Linux / macOS

```bash
g++ -std=c++17 -Wall -Wextra -Iinclude src/FreeSegment.cpp tests/freelist_test.cpp -o freelist_test
./freelist_test
```

Expected output:

```text
All FreeList tests passed.
HEAD -> [0, 12] -> NULL
```

## 2. Integrate with the complete team project

`AllocationManager.h/.cpp` depend on shared files supplied by Member 1 / the team:

- `include/Common.h`
- `include/StorageManager.h`
- `src/StorageManager.cpp`

The expected `StorageManager` interface is:

```cpp
int maxFiles() const;
bool hasFile(int id) const;
bool markBlocks(int start, int n, int id);
bool releaseFile(int id, int& start, int& length);
```

`Common.h` must define:

```cpp
enum class Strategy { FIRST_FIT, BEST_FIT, WORST_FIT };

enum class AllocResult {
    SUCCESS,
    INVALID_REQUEST,
    DUPLICATE_ID,
    NO_SPACE,
    FRAGMENTATION
};
```

If the team's shared headers already define these types, do not define them a second time. Use the team's existing versions.

Once the shared files are in the same project, compile the integrated engine with:

```powershell
g++ -std=c++17 -Wall -Wextra -Iinclude src/StorageManager.cpp src/FreeSegment.cpp src/AllocationManager.cpp tests/m2_test.cpp -o m2_test.exe
.\m2_test.exe
```

The `tests/m2_test.cpp` file is the integration test from the main team project. Add it from the shared project guide/repository.

## 3. What the algorithms do

- **First Fit:** selects the first free segment large enough for the request.
- **Best Fit:** selects the smallest free segment that can satisfy the request.
- **Worst Fit:** selects the largest available free segment that can satisfy the request.

If enough blocks are free in total but no single segment is large enough, allocation returns `FRAGMENTATION`. If total free space is insufficient, it returns `NO_SPACE`.

## 4. Complexity

Let `F` be the number of free segments.

| Operation | Time complexity |
|---|---:|
| First Fit search | O(F) worst case |
| Best Fit search | O(F) |
| Worst Fit search | O(F) |
| `consume` | O(F) |
| `addFreeRun` | O(F) |
| `segmentCount` | O(F) |
| `totalFree` | O(F) |
| `largest` | O(F) |

## 5. Important integration note

The free list and storage array must always describe the same blocks. `AllocationManager` updates both during allocation and release. Do not use `FreeList` and `StorageManager` independently for the same live simulation unless you also keep their state synchronized.
