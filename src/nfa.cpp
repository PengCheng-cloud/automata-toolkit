#include "automata/nfa.hpp"

#include <fstream>
#include <queue>
#include <sstream>
#include <stdexcept>

namespace automata {

NFA::NFA(int num_states) : num_states_(num_states), delta_(num_states) {
    if (num_states < 0) throw std::invalid_argument("number of states must be non-negative");
}

void NFA::check_state(int state) const {
    if (state < 0 || state >= num_states_)
        throw std::out_of_range("invalid state " + std::to_string(state));
}

void NFA::add_initial(int state) {
    check_state(state);
    initial_.insert(state);
}

void NFA::add_accepting(int state) {
    check_state(state);
    accepting_.insert(state);
}

void NFA::add_transition(int from, char symbol, int to) {
    check_state(from);
    check_state(to);
    delta_[from][symbol].insert(to);
    if (symbol != EPSILON) alphabet_.insert(symbol);
}

const std::map<char, std::set<int>>& NFA::transitions(int state) const {
    check_state(state);
    return delta_[state];
}

std::set<int> NFA::epsilon_closure(const std::set<int>& states) const {
    std::set<int> closure = states;
    std::vector<int> stack(states.begin(), states.end());
    while (!stack.empty()) {
        int s = stack.back();
        stack.pop_back();
        auto it = delta_[s].find(EPSILON);
        if (it == delta_[s].end()) continue;
        for (int t : it->second)
            if (closure.insert(t).second) stack.push_back(t);
    }
    return closure;
}

std::set<int> NFA::step(const std::set<int>& states, char symbol) const {
    std::set<int> next;
    for (int s : states) {
        auto it = delta_[s].find(symbol);
        if (it != delta_[s].end()) next.insert(it->second.begin(), it->second.end());
    }
    return epsilon_closure(next);
}

bool NFA::accepts(const std::string& word) const {
    std::set<int> current = epsilon_closure(initial_);
    for (char c : word) {
        current = step(current, c);
        if (current.empty()) return false;
    }
    for (int s : current)
        if (is_accepting(s)) return true;
    return false;
}

Determinized determinize(const NFA& nfa) {
    std::vector<std::set<int>> subsets;
    std::map<std::set<int>, int> index;
    std::vector<std::pair<int, std::pair<char, int>>> edges;  // from, (symbol, to)

    auto get_index = [&](const std::set<int>& subset, std::queue<int>& work) {
        auto [it, inserted] = index.emplace(subset, static_cast<int>(subsets.size()));
        if (inserted) {
            subsets.push_back(subset);
            work.push(it->second);
        }
        return it->second;
    };

    std::queue<int> work;
    get_index(nfa.epsilon_closure(nfa.initial()), work);
    while (!work.empty()) {
        int current = work.front();
        work.pop();
        for (char symbol : nfa.alphabet()) {
            std::set<int> next = nfa.step(subsets[current], symbol);
            if (next.empty()) continue;  // implicit dead state
            int target = get_index(next, work);
            edges.push_back({current, {symbol, target}});
        }
    }

    DFA dfa(static_cast<int>(subsets.size()));
    dfa.set_start(0);
    for (int i = 0; i < static_cast<int>(subsets.size()); ++i)
        for (int s : subsets[i])
            if (nfa.is_accepting(s)) {
                dfa.add_accepting(i);
                break;
            }
    for (const auto& [from, edge] : edges) dfa.add_transition(from, edge.first, edge.second);
    return {dfa, subsets};
}

NFA read_nfa(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open file: " + path);

    NFA nfa;
    bool has_states = false;
    std::string line;
    int line_no = 0;
    auto fail = [&](const std::string& msg) {
        throw std::runtime_error(path + ":" + std::to_string(line_no) + ": " + msg);
    };
    while (std::getline(in, line)) {
        ++line_no;
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string first;
        ss >> first;
        if (first.empty()) continue;

        if (first == "states") {
            int n;
            if (!(ss >> n)) fail("expected number of states");
            nfa = NFA(n);
            has_states = true;
        } else if (!has_states) {
            fail("'states' must come first");
        } else if (first == "initial" || first == "start") {
            int s;
            while (ss >> s) nfa.add_initial(s);
        } else if (first == "accept") {
            int s;
            while (ss >> s) nfa.add_accepting(s);
        } else {
            std::string symbol;
            int to;
            if (!(ss >> symbol >> to)) fail("expected '<from> <symbol> <to>'");
            int from = std::stoi(first);
            if (symbol == "eps")
                nfa.add_transition(from, EPSILON, to);
            else if (symbol.size() == 1)
                nfa.add_transition(from, symbol[0], to);
            else
                fail("symbols must be single characters or 'eps'");
        }
    }
    return nfa;
}

}  // namespace automata
