#include <iostream>
#include "StorageManager.h"

int main() {
    StorageManager st(20);
    std::cout << "Empty storage:\n";
    st.display();

    st.markBlocks(0, 5, 0);                    // file A = blocks 0-4
    st.markBlocks(5, 4, 1);                    // file B = blocks 5-8
    std::cout << "\nAfter A(5) and B(4):\n";
    st.display();
    std::cout << "occupied = " << st.occupiedCount() << "  (expect 9)\n";

    int s, len;
    st.releaseFile(0, s, len);                 // release A
    std::cout << "\nReleased A: start=" << s << " length=" << len << "  (expect 0 and 5)\n";
    st.display();

    std::cout << "\nFailure checks (all should print 0):\n";
    std::cout << "mark on occupied blocks : " << st.markBlocks(5, 2, 2) << "\n";
    std::cout << "mark out of range       : " << st.markBlocks(18, 5, 3) << "\n";
    std::cout << "duplicate file id       : " << st.markBlocks(10, 2, 1) << "\n";
    std::cout << "release unknown file    : " << st.releaseFile(9, s, len) << "\n";
    return 0;
}