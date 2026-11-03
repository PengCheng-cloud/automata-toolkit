#include "automata/algorithms.hpp"
#include "automata/nfa.hpp"
#include "check.hpp"

using namespace automata;

void test_algorithms() {
    DFA a = read_dfa("examples/ends_with_ab.dfa");
    DFA r = read_dfa("examples/ends_with_ab_redundant.dfa");

    // reachability and shortest witnesses
    CHECK(reachable_states(a).size() == 3);
    CHECK(shortest_accepted_word(a) == std::optional<std::string>("ab"));
    DFA empty(2);
    empty.add_transition(0, 'a', 1);
    CHECK(!shortest_accepted_word(empty));

    // minimization
    DFA m = minimize(r);
    CHECK(m.num_states() == 3);
    CHECK(equivalent(m, r));
    CHECK(minimize(a).num_states() == 3);

    // product, inclusion and equivalence
    DFA contains_ab = determinize(read_nfa("examples/contains_ab.nfa")).dfa;
    CHECK(!inclusion_counterexample(a, contains_ab));  // ends with ab => contains ab
    auto cex = inclusion_counterexample(contains_ab, a);
    CHECK(cex && contains_ab.accepts(*cex) && !a.accepts(*cex));
    CHECK(cex == std::optional<std::string>("aba"));
    CHECK(!equivalent(a, contains_ab));

    DFA both = product(a, contains_ab, ProductMode::Intersection);
    DFA either = product(a, contains_ab, ProductMode::Union);
    for (std::string w : {"", "ab", "aba", "bab", "abb", "bbab"}) {
        CHECK(both.accepts(w) == (a.accepts(w) && contains_ab.accepts(w)));
        CHECK(either.accepts(w) == (a.accepts(w) || contains_ab.accepts(w)));
    }
}
