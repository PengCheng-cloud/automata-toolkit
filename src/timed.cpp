#include "automata/timed.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace automata {

bool ClockConstraint::holds(const std::vector<int>& clocks) const {
    int v = clocks.at(clock);
    switch (op) {
        case Cmp::Less: return v < value;
        case Cmp::LessEq: return v <= value;
        case Cmp::Equal: return v == value;
        case Cmp::GreaterEq: return v >= value;
        case Cmp::Greater: return v > value;
    }
    return false;
}

int TimedAutomaton::add_clock(const std::string& name) {
    clock_names_.push_back(name);
    return num_clocks() - 1;
}

int TimedAutomaton::add_location(const std::string& name) {
    location_names_.push_back(name);
    invariants_.emplace_back();
    return num_locations() - 1;
}

void TimedAutomaton::set_initial(int location) {
    if (location < 0 || location >= num_locations()) throw std::out_of_range("invalid location");
    initial_ = location;
}

void TimedAutomaton::add_invariant(int location, ClockConstraint c) {
    invariants_.at(location).push_back(c);
}

void TimedAutomaton::add_edge(TAEdge edge) {
    if (edge.from < 0 || edge.from >= num_locations() || edge.to < 0 || edge.to >= num_locations())
        throw std::out_of_range("edge with invalid location");
    edges_.push_back(std::move(edge));
}

int TimedAutomaton::clock_index(const std::string& name) const {
    auto it = std::find(clock_names_.begin(), clock_names_.end(), name);
    if (it == clock_names_.end()) throw std::invalid_argument("unknown clock: " + name);
    return static_cast<int>(it - clock_names_.begin());
}

int TimedAutomaton::location_index(const std::string& name) const {
    auto it = std::find(location_names_.begin(), location_names_.end(), name);
    if (it == location_names_.end()) throw std::invalid_argument("unknown location: " + name);
    return static_cast<int>(it - location_names_.begin());
}

const std::vector<ClockConstraint>& TimedAutomaton::invariant(int location) const {
    return invariants_.at(location);
}

int TimedAutomaton::max_constant() const {
    int m = 0;
    for (const auto& inv : invariants_)
        for (const auto& c : inv) m = std::max(m, c.value);
    for (const auto& e : edges_)
        for (const auto& c : e.guard) m = std::max(m, c.value);
    return m;
}

namespace {

bool all_hold(const std::vector<ClockConstraint>& cs, const std::vector<int>& clocks) {
    return std::all_of(cs.begin(), cs.end(), [&](const ClockConstraint& c) { return c.holds(clocks); });
}

// All successors of a state: one delay step and every enabled edge.
std::vector<TraceStep> successors(const TimedAutomaton& ta, const TAState& s, int cap) {
    std::vector<TraceStep> result;

    TAState delayed = s;
    for (int& v : delayed.clocks) v = std::min(v + 1, cap);
    if (all_hold(ta.invariant(s.location), delayed.clocks)) result.push_back({true, "", delayed});

    for (const TAEdge& e : ta.edges()) {
        if (e.from != s.location || !all_hold(e.guard, s.clocks)) continue;
        TAState next{e.to, s.clocks};
        for (int c : e.resets) next.clocks.at(c) = 0;
        if (all_hold(ta.invariant(e.to), next.clocks)) result.push_back({false, e.label, next});
    }
    return result;
}

TAState initial_state(const TimedAutomaton& ta) {
    TAState init{ta.initial(), std::vector<int>(ta.num_clocks(), 0)};
    if (!all_hold(ta.invariant(init.location), init.clocks))
        throw std::runtime_error("initial state violates the invariant");
    return init;
}

}  // namespace

std::optional<std::vector<TraceStep>> reachability_witness(const TimedAutomaton& ta, int target) {
    const int cap = ta.max_constant() + 1;
    TAState init = initial_state(ta);

    std::map<TAState, std::pair<TAState, TraceStep>> parent;  // state -> (previous, step)
    std::set<TAState> seen{init};
    std::queue<TAState> q;
    q.push(init);
    while (!q.empty()) {
        TAState s = q.front();
        q.pop();
        if (s.location == target) {
            std::vector<TraceStep> trace;
            for (TAState cur = s; !(cur == init);) {
                const auto& [prev, step] = parent.at(cur);
                trace.push_back(step);
                cur = prev;
            }
            std::reverse(trace.begin(), trace.end());
            return trace;
        }
        for (const TraceStep& step : successors(ta, s, cap)) {
            if (seen.insert(step.state).second) {
                parent.emplace(step.state, std::make_pair(s, step));
                q.push(step.state);
            }
        }
    }
    return std::nullopt;
}

ExecutionTimes execution_times(const TimedAutomaton& ta, int secret_location,
                               int final_location, int horizon) {
    const int cap = ta.max_constant() + 1;
    // Extended state: (TA state, visited secret?, elapsed time).
    using Ext = std::tuple<TAState, bool, int>;
    ExecutionTimes result;

    TAState init = initial_state(ta);
    std::set<Ext> seen;
    std::queue<Ext> q;
    Ext start{init, init.location == secret_location, 0};
    seen.insert(start);
    q.push(start);
    while (!q.empty()) {
        auto [s, secret, time] = q.front();
        q.pop();
        if (s.location == final_location) {
            (secret ? result.secret_runs : result.public_runs).insert(time);
            continue;  // the run ends when the final location is reached
        }
        for (const TraceStep& step : successors(ta, s, cap)) {
            int t = time + (step.is_delay ? 1 : 0);
            if (t > horizon) continue;
            bool sec = secret || step.state.location == secret_location;
            Ext next{step.state, sec, t};
            if (seen.insert(next).second) q.push(next);
        }
    }
    return result;
}

namespace {

ClockConstraint parse_constraint(const TimedAutomaton& ta, const std::string& text) {
    static const std::vector<std::pair<std::string, Cmp>> ops = {
        {"<=", Cmp::LessEq}, {">=", Cmp::GreaterEq}, {"==", Cmp::Equal},
        {"<", Cmp::Less},    {">", Cmp::Greater},    {"=", Cmp::Equal}};
    for (const auto& [sym, op] : ops) {
        auto pos = text.find(sym);
        if (pos == std::string::npos) continue;
        std::string clock = text.substr(0, pos);
        int value = std::stoi(text.substr(pos + sym.size()));
        return {ta.clock_index(clock), op, value};
    }
    throw std::invalid_argument("cannot parse clock constraint: " + text);
}

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> parts;
    std::string part;
    std::istringstream ss(s);
    while (std::getline(ss, part, sep))
        if (!part.empty()) parts.push_back(part);
    return parts;
}

}  // namespace

TimedAutomaton read_timed_automaton(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open file: " + path);
    TimedAutomaton ta;
    std::string line;
    int line_no = 0;
    try {
        while (std::getline(in, line)) {
            ++line_no;
            auto hash = line.find('#');
            if (hash != std::string::npos) line = line.substr(0, hash);
            std::istringstream ss(line);
            std::string key;
            if (!(ss >> key)) continue;
            if (key == "clocks") {
                std::string name;
                while (ss >> name) ta.add_clock(name);
            } else if (key == "locations") {
                std::string name;
                while (ss >> name) ta.add_location(name);
            } else if (key == "initial") {
                std::string name;
                ss >> name;
                ta.set_initial(ta.location_index(name));
            } else if (key == "invariant") {
                std::string loc, cs;
                ss >> loc >> cs;
                for (const auto& c : split(cs, '&'))
                    ta.add_invariant(ta.location_index(loc), parse_constraint(ta, c));
            } else if (key == "edge") {
                std::string from, to, label, opt;
                if (!(ss >> from >> to >> label))
                    throw std::invalid_argument("expected 'edge <from> <to> <label>'");
                TAEdge e{ta.location_index(from), ta.location_index(to), label, {}, {}};
                while (ss >> opt) {
                    if (opt.rfind("guard=", 0) == 0) {
                        for (const auto& c : split(opt.substr(6), '&'))
                            e.guard.push_back(parse_constraint(ta, c));
                    } else if (opt.rfind("reset=", 0) == 0) {
                        for (const auto& c : split(opt.substr(6), ','))
                            e.resets.push_back(ta.clock_index(c));
                    } else {
                        throw std::invalid_argument("unknown edge option: " + opt);
                    }
                }
                ta.add_edge(e);
            } else {
                throw std::invalid_argument("unknown keyword: " + key);
            }
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(path + ":" + std::to_string(line_no) + ": " + e.what());
    }
    return ta;
}

std::string format_state(const TimedAutomaton& ta, const TAState& s) {
    std::ostringstream out;
    out << "(" << ta.location_name(s.location);
    for (int c = 0; c < ta.num_clocks(); ++c) out << ", " << ta.clock_name(c) << "=" << s.clocks[c];
    out << ")";
    return out.str();
}

}  // namespace automata
