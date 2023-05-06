#include "allocator.h"
#include <vector>
#include <iostream>
int main() {
    std::cout << "Hello, World!" << std::endl;
    std::vector<int, allocator<int>> v;
    for(int i=0; i<100; i++) {
        v.push_back(rand() % 1000);
    }
    for (int val : v) {
        std::cout<<val<<" ";
    }
    std::cout<<std::endl;
    return 0;
}
