#include <cassert>
#include <iostream>

#include "automata/dfa.hpp"

using automata::DFA;

// DFA over {a, b} that accepts words ending with "ab".
static DFA ends_with_ab() {
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

static void test_ends_with_ab() {
    DFA d = ends_with_ab();
    assert(d.accepts("ab"));
    assert(d.accepts("aab"));
    assert(d.accepts("babab"));
    assert(!d.accepts(""));
    assert(!d.accepts("a"));
    assert(!d.accepts("aba"));
    assert(!d.accepts("abc"));  // 'c' has no transition
}

static void test_determinism_is_enforced() {
    DFA d(2);
    d.add_transition(0, 'a', 1);
    bool thrown = false;
    try {
        d.add_transition(0, 'a', 0);
    } catch (const std::invalid_argument&) {
        thrown = true;
    }
    assert(thrown);
}

static void test_read_from_file() {
    DFA d = automata::read_dfa("examples/ends_with_ab.dfa");
    assert(d.num_states() == 3);
    assert(d.accepts("abab"));
    assert(!d.accepts("abba"));
}

int main() {
    test_ends_with_ab();
    test_determinism_is_enforced();
    test_read_from_file();
    std::cout << "All DFA tests passed.\n";
    return 0;
}
