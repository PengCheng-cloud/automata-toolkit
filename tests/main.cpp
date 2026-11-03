#include <iostream>

#include "check.hpp"

void test_dfa();
void test_nfa();
void test_algorithms();
void test_opacity();
void test_timed();
void test_random();

int main() {
    test_dfa();
    test_nfa();
    test_algorithms();
    test_opacity();
    test_timed();
    test_random();
    if (failures() == 0) {
        std::cout << "All tests passed.\n";
        return 0;
    }
    std::cout << failures() << " check(s) failed.\n";
    return 1;
}
