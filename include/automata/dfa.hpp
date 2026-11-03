#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

namespace automata {

// A deterministic finite automaton (DFA).
// States are numbered 0 .. num_states-1, symbols are single characters.
class DFA {
public:
    explicit DFA(int num_states = 0);

    int num_states() const { return num_states_; }
    int start() const { return start_; }
    const std::set<char>& alphabet() const { return alphabet_; }

    void set_start(int state);
    void add_accepting(int state);
    void add_transition(int from, char symbol, int to);

    bool is_accepting(int state) const;
    const std::set<int>& accepting() const { return accepting_; }

    // Outgoing transitions of a state: symbol -> target state.
    const std::map<char, int>& transitions(int state) const;

    // Returns the next state, or -1 if there is no transition (dead state).
    int next(int state, char symbol) const;

    // Runs the DFA on the input word and returns true if it ends in an accepting state.
    bool accepts(const std::string& word) const;

private:
    void check_state(int state) const;

    int num_states_;
    int start_ = 0;
    std::set<char> alphabet_;
    std::set<int> accepting_;
    std::vector<std::map<char, int>> delta_;
};

// Reads a DFA from a simple text format (see examples/*.dfa).
DFA read_dfa(const std::string& path);

// Writes a DFA in the same text format.
std::string to_text(const DFA& dfa);

// Writes a DFA in Graphviz DOT format (render with: dot -Tpng file.dot -o file.png).
std::string to_dot(const DFA& dfa);

}  // namespace automata
