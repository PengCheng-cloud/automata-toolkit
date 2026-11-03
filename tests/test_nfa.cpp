#include "automata/nfa.hpp"
#include "check.hpp"

using namespace automata;

void test_nfa() {
    NFA n = read_nfa("examples/contains_ab.nfa");
    CHECK(n.accepts("ab"));
    CHECK(n.accepts("bbabaa"));
    CHECK(!n.accepts(""));
    CHECK(!n.accepts("ba"));
    CHECK(!n.accepts("bbbaaa"));

    // epsilon closure follows chains of epsilon edges
    NFA e(3);
    e.add_initial(0);
    e.add_transition(0, EPSILON, 1);
    e.add_transition(1, EPSILON, 2);
    CHECK((e.epsilon_closure({0}) == std::set<int>{0, 1, 2}));

    Determinized d = determinize(n);
    for (std::string w : {"", "a", "b", "ab", "ba", "aab", "bab", "abba", "bbba", "aaaa"})
        CHECK(d.dfa.accepts(w) == n.accepts(w));
    // every DFA state corresponds to a non-empty set of NFA states
    for (const auto& s : d.subsets) CHECK(!s.empty());
}
