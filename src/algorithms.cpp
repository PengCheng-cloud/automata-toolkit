#include "automata/algorithms.hpp"

#include <algorithm>
#include <map>
#include <queue>
#include <vector>

namespace automata {

std::set<int> reachable_states(const DFA& dfa) {
    std::set<int> seen;
    if (dfa.num_states() == 0) return seen;
    std::queue<int> q;
    q.push(dfa.start());
    seen.insert(dfa.start());
    while (!q.empty()) {
        int s = q.front();
        q.pop();
        for (const auto& [symbol, t] : dfa.transitions(s))
            if (seen.insert(t).second) q.push(t);
    }
    return seen;
}

std::optional<std::string> shortest_accepted_word(const DFA& dfa) {
    if (dfa.num_states() == 0) return std::nullopt;
    // BFS that remembers how every state was reached, so we can rebuild the word.
    std::vector<int> parent(dfa.num_states(), -1);
    std::vector<char> via(dfa.num_states(), 0);
    std::vector<bool> seen(dfa.num_states(), false);
    std::queue<int> q;
    q.push(dfa.start());
    seen[dfa.start()] = true;
    while (!q.empty()) {
        int s = q.front();
        q.pop();
        if (dfa.is_accepting(s)) {
            std::string word;
            for (int cur = s; cur != dfa.start(); cur = parent[cur]) word += via[cur];
            std::reverse(word.begin(), word.end());
            return word;
        }
        for (const auto& [symbol, t] : dfa.transitions(s)) {
            if (seen[t]) continue;
            seen[t] = true;
            parent[t] = s;
            via[t] = symbol;
            q.push(t);
        }
    }
    return std::nullopt;
}

DFA product(const DFA& a, const DFA& b, ProductMode mode) {
    // A component state of -1 means "already rejected" (implicit sink).
    using Pair = std::pair<int, int>;
    std::set<char> alphabet = a.alphabet();
    alphabet.insert(b.alphabet().begin(), b.alphabet().end());

    auto accepting = [&](const Pair& p) {
        bool in_a = p.first != -1 && a.is_accepting(p.first);
        bool in_b = p.second != -1 && b.is_accepting(p.second);
        switch (mode) {
            case ProductMode::Intersection: return in_a && in_b;
            case ProductMode::Union: return in_a || in_b;
            case ProductMode::Difference: return in_a && !in_b;
        }
        return false;
    };
    auto step = [](const DFA& d, int s, char c) { return s == -1 ? -1 : d.next(s, c); };

    std::map<Pair, int> index;
    std::vector<Pair> states;
    std::vector<std::pair<int, std::pair<char, int>>> edges;
    std::queue<int> q;
    auto get = [&](const Pair& p) {
        auto [it, inserted] = index.emplace(p, static_cast<int>(states.size()));
        if (inserted) {
            states.push_back(p);
            q.push(it->second);
        }
        return it->second;
    };

    Pair start{a.num_states() ? a.start() : -1, b.num_states() ? b.start() : -1};
    get(start);
    while (!q.empty()) {
        int i = q.front();
        q.pop();
        Pair cur = states[i];
        for (char c : alphabet) {
            Pair next{step(a, cur.first, c), step(b, cur.second, c)};
            if (next.first == -1 && next.second == -1) continue;  // both rejected: dead
            edges.push_back({i, {c, get(next)}});
        }
    }

    DFA result(static_cast<int>(states.size()));
    result.set_start(0);
    for (int i = 0; i < static_cast<int>(states.size()); ++i)
        if (accepting(states[i])) result.add_accepting(i);
    for (const auto& [from, e] : edges) result.add_transition(from, e.first, e.second);
    return result;
}

std::optional<std::string> inclusion_counterexample(const DFA& a, const DFA& b) {
    return shortest_accepted_word(product(a, b, ProductMode::Difference));
}

bool equivalent(const DFA& a, const DFA& b) {
    return !inclusion_counterexample(a, b) && !inclusion_counterexample(b, a);
}

DFA minimize(const DFA& dfa) {
    if (dfa.num_states() == 0) return DFA(0);

    // 1. Keep only reachable states and add an explicit sink to make the DFA complete.
    std::set<int> reach = reachable_states(dfa);
    std::vector<int> old_states(reach.begin(), reach.end());
    std::map<int, int> renum;
    for (int i = 0; i < static_cast<int>(old_states.size()); ++i) renum[old_states[i]] = i;
    const int n = static_cast<int>(old_states.size()) + 1;  // last state = sink
    const int sink = n - 1;
    std::vector<char> sigma(dfa.alphabet().begin(), dfa.alphabet().end());

    std::vector<std::vector<int>> delta(n, std::vector<int>(sigma.size(), sink));
    std::vector<bool> acc(n, false);
    for (int i = 0; i < sink; ++i) {
        acc[i] = dfa.is_accepting(old_states[i]);
        for (size_t k = 0; k < sigma.size(); ++k) {
            int t = dfa.next(old_states[i], sigma[k]);
            if (t != -1) delta[i][k] = renum[t];
        }
    }

    // 2. Moore's algorithm: refine blocks until no block can be split any more.
    std::vector<int> block(n);
    for (int i = 0; i < n; ++i) block[i] = acc[i] ? 1 : 0;
    int num_blocks = 0;
    while (true) {
        std::map<std::vector<int>, int> signature_to_block;
        std::vector<int> new_block(n);
        for (int i = 0; i < n; ++i) {
            std::vector<int> sig{block[i]};
            for (size_t k = 0; k < sigma.size(); ++k) sig.push_back(block[delta[i][k]]);
            auto [it, inserted] =
                signature_to_block.emplace(sig, static_cast<int>(signature_to_block.size()));
            new_block[i] = it->second;
        }
        int count = static_cast<int>(signature_to_block.size());
        block = new_block;
        if (count == num_blocks) break;
        num_blocks = count;
    }

    // 3. Remove dead blocks (blocks that cannot reach an accepting block).
    std::vector<std::vector<int>> block_delta(num_blocks, std::vector<int>(sigma.size()));
    std::vector<bool> block_acc(num_blocks, false);
    for (int i = 0; i < n; ++i) {
        block_acc[block[i]] = acc[i];
        for (size_t k = 0; k < sigma.size(); ++k) block_delta[block[i]][k] = block[delta[i][k]];
    }
    std::vector<bool> alive(num_blocks, false);
    for (int b = 0; b < num_blocks; ++b) alive[b] = block_acc[b];
    for (bool changed = true; changed;) {
        changed = false;
        for (int b = 0; b < num_blocks; ++b) {
            if (alive[b]) continue;
            for (size_t k = 0; k < sigma.size(); ++k)
                if (alive[block_delta[b][k]]) {
                    alive[b] = changed = true;
                    break;
                }
        }
    }

    // 4. Build the result, numbering blocks in BFS order from the start for a canonical form.
    int start_block = block[0];
    if (!alive[start_block]) return DFA(1);  // empty language: one rejecting state
    std::map<int, int> final_id;
    std::vector<int> order;
    std::queue<int> q;
    final_id[start_block] = 0;
    order.push_back(start_block);
    q.push(start_block);
    while (!q.empty()) {
        int b = q.front();
        q.pop();
        for (size_t k = 0; k < sigma.size(); ++k) {
            int t = block_delta[b][k];
            if (alive[t] && !final_id.count(t)) {
                final_id[t] = static_cast<int>(order.size());
                order.push_back(t);
                q.push(t);
            }
        }
    }
    DFA result(static_cast<int>(order.size()));
    result.set_start(0);
    for (int b : order) {
        if (block_acc[b]) result.add_accepting(final_id[b]);
        for (size_t k = 0; k < sigma.size(); ++k) {
            int t = block_delta[b][k];
            if (alive[t]) result.add_transition(final_id[b], sigma[k], final_id[t]);
        }
    }
    return result;
}

}  // namespace automata
