#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "automata/dfa.hpp"

namespace automata {

// Symbol used for epsilon transitions. In files it is written as "eps".
constexpr char EPSILON = '\0';

// A nondeterministic finite automaton with epsilon transitions.
class NFA {
public:
    explicit NFA(int num_states = 0);

    int num_states() const { return num_states_; }
    const std::set<int>& initial() const { return initial_; }
    const std::set<int>& accepting() const { return accepting_; }
    // Alphabet without epsilon.
    const std::set<char>& alphabet() const { return alphabet_; }

    void add_initial(int state);
    void add_accepting(int state);
    void add_transition(int from, char symbol, int to);

    bool is_accepting(int state) const { return accepting_.count(state) > 0; }
    const std::map<char, std::set<int>>& transitions(int state) const;

    // All states reachable from `states` using only epsilon transitions.
    std::set<int> epsilon_closure(const std::set<int>& states) const;

    // States reachable from `states` by reading `symbol`, followed by epsilon closure.
    std::set<int> step(const std::set<int>& states, char symbol) const;

    bool accepts(const std::string& word) const;

private:
    void check_state(int state) const;

    int num_states_;
    std::set<int> initial_;
    std::set<int> accepting_;
    std::set<char> alphabet_;
    std::vector<std::map<char, std::set<int>>> delta_;
};

// Result of the subset construction: the DFA and, for every DFA state,
// the set of NFA states it represents.
struct Determinized {
    DFA dfa;
    std::vector<std::set<int>> subsets;
};

// Subset construction (NFA -> DFA). Only reachable subsets are created;
// the empty subset is not represented (missing transitions mean "reject").
Determinized determinize(const NFA& nfa);

// Reads an NFA. Format as for DFAs, but with "initial" instead of "start"
// (several initial states allowed) and "eps" for epsilon transitions.
NFA read_nfa(const std::string& path);

}  // namespace automata
