// Randomized cross-checks: the algorithms must agree with each other
// and with direct simulation on many random automata and words.
#include <random>

#include "automata/algorithms.hpp"
#include "automata/nfa.hpp"
#include "check.hpp"

using namespace automata;

namespace {

std::string random_word(std::mt19937& rng, int max_len) {
    std::uniform_int_distribution<int> len(0, max_len), sym(0, 1);
    std::string w;
    for (int i = len(rng); i > 0; --i) w += sym(rng) ? 'a' : 'b';
    return w;
}

NFA random_nfa(std::mt19937& rng) {
    std::uniform_int_distribution<int> size(1, 6);
    int n = size(rng);
    std::uniform_int_distribution<int> state(0, n - 1), coin(0, 99);
    NFA nfa(n);
    nfa.add_initial(state(rng));
    for (int s = 0; s < n; ++s) {
        if (coin(rng) < 30) nfa.add_accepting(s);
        for (char c : {'a', 'b'})
            for (int t = 0; t < n; ++t)
                if (coin(rng) < 25) nfa.add_transition(s, c, t);
        if (coin(rng) < 15) nfa.add_transition(s, EPSILON, state(rng));
    }
    return nfa;
}

}  // namespace

void test_random() {
    std::mt19937 rng(12345);
    for (int round = 0; round < 300; ++round) {
        NFA nfa = random_nfa(rng);
        DFA dfa = determinize(nfa).dfa;
        DFA min = minimize(dfa);
        CHECK(min.num_states() <= std::max(dfa.num_states(), 1));
        CHECK(equivalent(dfa, min));
        CHECK(minimize(min).num_states() == min.num_states());  // minimal is a fixpoint

        auto witness = shortest_accepted_word(min);
        for (int i = 0; i < 40; ++i) {
            std::string w = random_word(rng, 8);
            bool expected = nfa.accepts(w);
            CHECK(dfa.accepts(w) == expected);
            CHECK(min.accepts(w) == expected);
            if (expected) CHECK(witness.has_value() && witness->size() <= w.size());
        }
        if (witness) CHECK(nfa.accepts(*witness));
    }
}
