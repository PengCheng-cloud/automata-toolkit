#include "automata/dfa.hpp"
#include "check.hpp"

using automata::DFA;

namespace {

// DFA over {a, b} that accepts words ending with "ab".
DFA ends_with_ab() {
    DFA d(3);
    d.set_start(0);
    d.add_accepting(2);
    d.add_transition(0, 'a', 1);
    d.add_transition(0, 'b', 0);
    d.add_transition(1, 'a', 1);
    d.add_transition(1, 'b', 2);
    d.add_transition(2, 'a', 1);
    d.add_transition(2, 'b', 0);
    return d;
}

}  // namespace

void test_dfa() {
    DFA d = ends_with_ab();
    CHECK(d.accepts("ab"));
    CHECK(d.accepts("aab"));
    CHECK(d.accepts("babab"));
    CHECK(!d.accepts(""));
    CHECK(!d.accepts("a"));
    CHECK(!d.accepts("aba"));
    CHECK(!d.accepts("abc"));  // 'c' has no transition

    DFA nd(2);
    nd.add_transition(0, 'a', 1);
    CHECK_THROWS(nd.add_transition(0, 'a', 0));  // would make it nondeterministic
    CHECK_THROWS(nd.add_transition(0, 'b', 5));  // invalid state

    DFA f = automata::read_dfa("examples/ends_with_ab.dfa");
    CHECK(f.num_states() == 3);
    CHECK(f.accepts("abab"));
    CHECK(!f.accepts("abba"));
}
