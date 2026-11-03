#pragma once

#include <optional>
#include <set>
#include <string>

#include "automata/nfa.hpp"

namespace automata {

// Current-state opacity of a finite system.
//
// An attacker knows the model and observes only the events in `observable`
// (all other events, and epsilon, are silent). From an observation it can
// compute the set of states the system may currently be in (its "estimate").
// The system is current-state opaque if no observation lets the attacker be
// sure that the system is in a secret state, i.e. no reachable non-empty
// estimate is a subset of `secret`.
//
// The check builds the attacker's observer with a subset construction over
// the observable events and searches it with BFS.
struct OpacityResult {
    bool opaque = true;
    // If not opaque: a shortest observation that reveals the secret,
    // and the attacker's estimate after it.
    std::optional<std::string> leaking_observation;
    std::set<int> estimate;
};

OpacityResult check_current_state_opacity(const NFA& system, const std::set<int>& secret,
                                          const std::set<char>& observable);

}  // namespace automata
