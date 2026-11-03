#pragma once

#include <optional>
#include <set>
#include <string>

#include "automata/dfa.hpp"

namespace automata {

// States reachable from the start state (BFS).
std::set<int> reachable_states(const DFA& dfa);

// A shortest word accepted by the DFA, or std::nullopt if the language is empty.
// Doubles as a reachability check with a counterexample trace.
std::optional<std::string> shortest_accepted_word(const DFA& dfa);

enum class ProductMode { Intersection, Union, Difference };

// Product construction over the union of both alphabets.
// Missing transitions are treated as moving to a rejecting sink.
//   Intersection: L(a) ∩ L(b),  Union: L(a) ∪ L(b),  Difference: L(a) \ L(b)
DFA product(const DFA& a, const DFA& b, ProductMode mode);

// A shortest word in L(a) \ L(b), or std::nullopt if L(a) ⊆ L(b).
std::optional<std::string> inclusion_counterexample(const DFA& a, const DFA& b);

bool equivalent(const DFA& a, const DFA& b);

// Minimal DFA for the same language (Moore's partition refinement).
// Unreachable and dead states are removed, so missing transitions mean "reject".
DFA minimize(const DFA& dfa);

}  // namespace automata
