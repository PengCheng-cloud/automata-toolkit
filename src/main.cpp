#include <iostream>

#include "automata/dfa.hpp"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <file.dfa> <word> [<word> ...]\n";
        return 1;
    }
    try {
        automata::DFA dfa = automata::read_dfa(argv[1]);
        for (int i = 2; i < argc; ++i) {
            std::string word = argv[i];
            std::cout << (word.empty() ? "(empty)" : word) << ": "
                      << (dfa.accepts(word) ? "accepted" : "rejected") << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
