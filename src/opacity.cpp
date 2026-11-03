#include "automata/opacity.hpp"

#include <algorithm>
#include <map>
#include <queue>
#include <vector>

namespace automata {

namespace {

// Closure under silent moves: epsilon and unobservable events.
std::set<int> silent_closure(const NFA& nfa, const std::set<int>& states,
                             const std::set<char>& observable) {
    std::set<int> closure = states;
    std::vector<int> stack(states.begin(), states.end());
    while (!stack.empty()) {
        int s = stack.back();
        stack.pop_back();
        for (const auto& [symbol, targets] : nfa.transitions(s)) {
            if (symbol != EPSILON && observable.count(symbol)) continue;
            for (int t : targets)
                if (closure.insert(t).second) stack.push_back(t);
        }
    }
    return closure;
}

bool reveals_secret(const std::set<int>& estimate, const std::set<int>& secret) {
    if (estimate.empty()) return false;
    return std::all_of(estimate.begin(), estimate.end(),
                       [&](int s) { return secret.count(s) > 0; });
}

}  // namespace

OpacityResult check_current_state_opacity(const NFA& system, const std::set<int>& secret,
                                          const std::set<char>& observable) {
    std::map<std::set<int>, std::string> observation_for;  // estimate -> shortest observation
    std::queue<std::set<int>> q;

    std::set<int> init = silent_closure(system, system.initial(), observable);
    observation_for[init] = "";
    q.push(init);
    while (!q.empty()) {
        std::set<int> estimate = q.front();
        q.pop();
        const std::string& obs = observation_for[estimate];
        if (reveals_secret(estimate, secret)) return {false, obs, estimate};

        for (char o : observable) {
            std::set<int> moved;
            for (int s : estimate) {
                auto it = system.transitions(s).find(o);
                if (it != system.transitions(s).end())
                    moved.insert(it->second.begin(), it->second.end());
            }
            if (moved.empty()) continue;  // observation impossible
            std::set<int> next = silent_closure(system, moved, observable);
            if (observation_for.emplace(next, obs + o).second) q.push(next);
        }
    }
    return {true, std::nullopt, {}};
}

}  // namespace automata
