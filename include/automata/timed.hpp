#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>

namespace automata {

// Timed automata with integer-time (discrete-time) semantics.
//
// Time advances in steps of one unit and all clocks advance together.
// Clock values larger than the largest constant in the model cannot be
// distinguished by any guard or invariant, so they are capped at
// (max constant + 1). This keeps the state space finite.

enum class Cmp { Less, LessEq, Equal, GreaterEq, Greater };

struct ClockConstraint {
    int clock;
    Cmp op;
    int value;
    bool holds(const std::vector<int>& clocks) const;
};

struct TAEdge {
    int from;
    int to;
    std::string label;
    std::vector<ClockConstraint> guard;  // conjunction
    std::vector<int> resets;
};

class TimedAutomaton {
public:
    int add_clock(const std::string& name);
    int add_location(const std::string& name);
    void set_initial(int location);
    void add_invariant(int location, ClockConstraint c);
    void add_edge(TAEdge edge);

    int num_clocks() const { return static_cast<int>(clock_names_.size()); }
    int num_locations() const { return static_cast<int>(location_names_.size()); }
    int initial() const { return initial_; }
    const std::string& clock_name(int c) const { return clock_names_.at(c); }
    const std::string& location_name(int l) const { return location_names_.at(l); }
    int clock_index(const std::string& name) const;     // throws if unknown
    int location_index(const std::string& name) const;  // throws if unknown
    const std::vector<TAEdge>& edges() const { return edges_; }
    const std::vector<ClockConstraint>& invariant(int location) const;

    // Largest constant used in any guard or invariant.
    int max_constant() const;

private:
    std::vector<std::string> clock_names_;
    std::vector<std::string> location_names_;
    std::vector<std::vector<ClockConstraint>> invariants_;
    std::vector<TAEdge> edges_;
    int initial_ = 0;
};

struct TAState {
    int location;
    std::vector<int> clocks;
    bool operator==(const TAState& o) const {
        return location == o.location && clocks == o.clocks;
    }
    bool operator<(const TAState& o) const {
        return location != o.location ? location < o.location : clocks < o.clocks;
    }
};

// One step of a run: either one time unit passes, or an edge is taken.
struct TraceStep {
    bool is_delay;
    std::string label;  // edge label (empty for delays)
    TAState state;      // state after the step
};

// Shortest run (in number of steps) reaching `target`, or nullopt if unreachable.
std::optional<std::vector<TraceStep>> reachability_witness(const TimedAutomaton& ta, int target);

// Execution-time opacity: an attacker only observes how long the system takes
// to reach `final_location`. We compute the possible execution times (up to
// `horizon`) of runs that visited `secret_location` and of runs that did not.
// If both sets are equal, the execution time reveals nothing about the secret.
struct ExecutionTimes {
    std::set<int> secret_runs;
    std::set<int> public_runs;
    bool opaque() const { return secret_runs == public_runs; }
};

ExecutionTimes execution_times(const TimedAutomaton& ta, int secret_location,
                               int final_location, int horizon);

// Text format, see examples/*.ta.
TimedAutomaton read_timed_automaton(const std::string& path);

std::string format_state(const TimedAutomaton& ta, const TAState& s);

}  // namespace automata
