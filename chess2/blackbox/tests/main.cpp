#include <iostream>
#include "dfs.h"

int main() {
    try {
        tests::run_all();
        std::cout << "All tests passed\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Test failed: " << ex.what() << std::endl;
        return 1;
    }
}
