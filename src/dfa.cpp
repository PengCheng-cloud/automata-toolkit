#include "automata/dfa.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace automata {

DFA::DFA(int num_states) : num_states_(num_states), delta_(num_states) {
    if (num_states < 0) throw std::invalid_argument("number of states must be non-negative");
}

void DFA::check_state(int state) const {
    if (state < 0 || state >= num_states_)
        throw std::out_of_range("invalid state " + std::to_string(state));
}

void DFA::set_start(int state) {
    check_state(state);
    start_ = state;
}

void DFA::add_accepting(int state) {
    check_state(state);
    accepting_.insert(state);
}

void DFA::add_transition(int from, char symbol, int to) {
    check_state(from);
    check_state(to);
    auto [it, inserted] = delta_[from].emplace(symbol, to);
    if (!inserted && it->second != to)
        throw std::invalid_argument("DFA must be deterministic: two transitions from state " +
                                    std::to_string(from) + " on '" + symbol + "'");
    alphabet_.insert(symbol);
}

bool DFA::is_accepting(int state) const { return accepting_.count(state) > 0; }

const std::map<char, int>& DFA::transitions(int state) const {
    check_state(state);
    return delta_[state];
}

int DFA::next(int state, char symbol) const {
    check_state(state);
    auto it = delta_[state].find(symbol);
    return it == delta_[state].end() ? -1 : it->second;
}

bool DFA::accepts(const std::string& word) const {
    if (num_states_ == 0) return false;
    int state = start_;
    for (char c : word) {
        state = next(state, c);
        if (state == -1) return false;  // no transition: reject
    }
    return is_accepting(state);
}

// File format (lines starting with '#' are comments):
//   states 3
//   start 0
//   accept 2
//   0 a 1
//   1 b 2
DFA read_dfa(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open file: " + path);

    DFA dfa;
    bool has_states = false;
    std::string line;
    int line_no = 0;
    while (std::getline(in, line)) {
        ++line_no;
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string first;
        ss >> first;
        if (first.empty()) continue;

        if (first == "states") {
            int n;
            ss >> n;
            dfa = DFA(n);
            has_states = true;
        } else if (!has_states) {
            throw std::runtime_error(path + ":" + std::to_string(line_no) +
                                     ": 'states' must come first");
        } else if (first == "start") {
            int s;
            ss >> s;
            dfa.set_start(s);
        } else if (first == "accept") {
            int s;
            while (ss >> s) dfa.add_accepting(s);
        } else {
            int from = std::stoi(first), to;
            char symbol;
            if (!(ss >> symbol >> to))
                throw std::runtime_error(path + ":" + std::to_string(line_no) +
                                         ": expected '<from> <symbol> <to>'");
            dfa.add_transition(from, symbol, to);
        }
    }
    return dfa;
}

std::string to_text(const DFA& dfa) {
    std::ostringstream out;
    out << "states " << dfa.num_states() << "\n";
    out << "start " << dfa.start() << "\n";
    out << "accept";
    for (int s : dfa.accepting()) out << " " << s;
    out << "\n";
    for (int s = 0; s < dfa.num_states(); ++s)
        for (const auto& [symbol, to] : dfa.transitions(s))
            out << s << " " << symbol << " " << to << "\n";
    return out.str();
}

std::string to_dot(const DFA& dfa) {
    std::ostringstream out;
    out << "digraph DFA {\n  rankdir=LR;\n  __start [shape=point];\n";
    for (int s = 0; s < dfa.num_states(); ++s)
        out << "  " << s << " [shape=" << (dfa.is_accepting(s) ? "doublecircle" : "circle")
            << "];\n";
    if (dfa.num_states() > 0) out << "  __start -> " << dfa.start() << ";\n";
    for (int s = 0; s < dfa.num_states(); ++s)
        for (const auto& [symbol, to] : dfa.transitions(s))
            out << "  " << s << " -> " << to << " [label=\"" << symbol << "\"];\n";
    out << "}\n";
    return out.str();
}

}  // namespace automata
