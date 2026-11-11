#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "automata/algorithms.hpp"
#include "automata/dfa.hpp"
#include "automata/nfa.hpp"
#include "automata/opacity.hpp"
#include "automata/timed.hpp"

using namespace automata;

namespace {

const char* kUsage = R"(Usage: automata <command> [arguments]

Finite automata
  run <file.dfa|file.nfa> <word>...       check which words are accepted
  determinize <file.nfa>                  subset construction, prints the DFA
  minimize <file.dfa>                     prints the minimal DFA
  dot <file.dfa>                          prints the DFA in Graphviz format
  equiv <a.dfa> <b.dfa>                   language equivalence with counterexample
  opacity <file.nfa> <secret> <observable>
                                          current-state opacity; secret states as
                                          a comma-separated list, observable events
                                          as a string, e.g.: opacity sys.nfa 3,4 ab

Timed automata (integer-time semantics)
  reach <file.ta> <location>              shortest run reaching a location
  et-opacity <file.ta> <secret> <final> <horizon>
                                          execution-time opacity
  states <file.ta>                        size of the reachable state space with the
                                          global clock bound and with per-location bounds
)";

bool ends_with(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string show(const std::string& word) { return word.empty() ? "(empty word)" : word; }

std::string show(const std::set<int>& s) {
    std::ostringstream out;
    out << "{";
    bool first = true;
    for (int x : s) {
        out << (first ? "" : ", ") << x;
        first = false;
    }
    out << "}";
    return out.str();
}

int cmd_run(const std::vector<std::string>& args) {
    if (args.size() < 2) throw std::invalid_argument("run needs a file and at least one word");
    bool is_nfa = ends_with(args[0], ".nfa");
    NFA nfa;
    DFA dfa;
    if (is_nfa) nfa = read_nfa(args[0]);
    else dfa = read_dfa(args[0]);
    for (size_t i = 1; i < args.size(); ++i) {
        bool ok = is_nfa ? nfa.accepts(args[i]) : dfa.accepts(args[i]);
        std::cout << show(args[i]) << ": " << (ok ? "accepted" : "rejected") << "\n";
    }
    return 0;
}

int cmd_determinize(const std::vector<std::string>& args) {
    if (args.size() != 1) throw std::invalid_argument("determinize needs one file");
    Determinized d = determinize(read_nfa(args[0]));
    for (size_t i = 0; i < d.subsets.size(); ++i)
        std::cout << "# DFA state " << i << " = NFA states " << show(d.subsets[i]) << "\n";
    std::cout << to_text(d.dfa);
    return 0;
}

int cmd_minimize(const std::vector<std::string>& args) {
    if (args.size() != 1) throw std::invalid_argument("minimize needs one file");
    DFA in = read_dfa(args[0]);
    DFA out = minimize(in);
    std::cout << "# " << in.num_states() << " states -> " << out.num_states() << " states\n"
              << to_text(out);
    return 0;
}

int cmd_dot(const std::vector<std::string>& args) {
    if (args.size() != 1) throw std::invalid_argument("dot needs one file");
    std::cout << to_dot(read_dfa(args[0]));
    return 0;
}

int cmd_equiv(const std::vector<std::string>& args) {
    if (args.size() != 2) throw std::invalid_argument("equiv needs two files");
    DFA a = read_dfa(args[0]), b = read_dfa(args[1]);
    if (auto w = inclusion_counterexample(a, b)) {
        std::cout << "not equivalent: " << show(*w) << " is accepted only by " << args[0] << "\n";
        return 0;
    }
    if (auto w = inclusion_counterexample(b, a)) {
        std::cout << "not equivalent: " << show(*w) << " is accepted only by " << args[1] << "\n";
        return 0;
    }
    std::cout << "equivalent\n";
    return 0;
}

int cmd_opacity(const std::vector<std::string>& args) {
    if (args.size() != 3) throw std::invalid_argument("opacity needs a file, secret states and observable events");
    NFA system = read_nfa(args[0]);
    std::set<int> secret;
    std::stringstream ss(args[1]);
    for (std::string part; std::getline(ss, part, ',');) secret.insert(std::stoi(part));
    std::set<char> observable(args[2].begin(), args[2].end());

    OpacityResult r = check_current_state_opacity(system, secret, observable);
    if (r.opaque) {
        std::cout << "current-state opaque: no observation reveals that the system is in "
                  << show(secret) << "\n";
    } else {
        std::cout << "NOT opaque: after observing " << show(*r.leaking_observation)
                  << " the attacker knows the state is in " << show(r.estimate) << "\n";
    }
    return 0;
}

int cmd_reach(const std::vector<std::string>& args) {
    if (args.size() != 2) throw std::invalid_argument("reach needs a file and a location");
    TimedAutomaton ta = read_timed_automaton(args[0]);
    int target = ta.location_index(args[1]);
    auto trace = reachability_witness(ta, target);
    if (!trace) {
        std::cout << args[1] << " is unreachable\n";
        return 0;
    }
    TAState s{ta.initial(), std::vector<int>(ta.num_clocks(), 0)};
    std::cout << args[1] << " is reachable:\n  " << format_state(ta, s) << "\n";
    for (const TraceStep& step : *trace)
        std::cout << "  --" << (step.is_delay ? "delay 1" : step.label) << "--> "
                  << format_state(ta, step.state) << "\n";
    return 0;
}

int cmd_et_opacity(const std::vector<std::string>& args) {
    if (args.size() != 4) throw std::invalid_argument("et-opacity needs a file, secret, final and horizon");
    TimedAutomaton ta = read_timed_automaton(args[0]);
    ExecutionTimes et = execution_times(ta, ta.location_index(args[1]), ta.location_index(args[2]),
                                        std::stoi(args[3]));
    std::cout << "execution times of runs through " << args[1] << ": " << show(et.secret_runs) << "\n"
              << "execution times of other runs:    " << show(et.public_runs) << "\n";
    if (et.opaque()) {
        std::cout << "execution-time opaque: the duration reveals nothing about the secret\n";
    } else {
        std::cout << "NOT execution-time opaque: some durations reveal whether " << args[1]
                  << " was visited\n";
    }
    return 0;
}

int cmd_states(const std::vector<std::string>& args) {
    if (args.size() != 1) throw std::invalid_argument("states needs a file");
    TimedAutomaton ta = read_timed_automaton(args[0]);

    std::cout << "relevant clock bounds per location (- means the clock is dead there):\n";
    auto bounds = ta.relevant_bounds();
    for (int l = 0; l < ta.num_locations(); ++l) {
        std::cout << "  " << ta.location_name(l) << ":";
        for (int c = 0; c < ta.num_clocks(); ++c) {
            std::cout << " " << ta.clock_name(c) << ":";
            if (bounds[l][c] < 0) std::cout << "-";
            else std::cout << bounds[l][c];
        }
        std::cout << "\n";
    }

    StateSpaceSize global = explore(ta, ClockAbstraction::Global);
    StateSpaceSize local = explore(ta, ClockAbstraction::PerLocation);
    std::cout << "global bound (" << ta.max_constant() << "): " << global.states << " states, "
              << global.transitions << " transitions\n"
              << "per-location bounds: " << local.states << " states, " << local.transitions
              << " transitions\n";
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << kUsage;
        return 1;
    }
    std::string cmd = argv[1];
    std::vector<std::string> args(argv + 2, argv + argc);
    try {
        if (cmd == "run") return cmd_run(args);
        if (cmd == "determinize") return cmd_determinize(args);
        if (cmd == "minimize") return cmd_minimize(args);
        if (cmd == "dot") return cmd_dot(args);
        if (cmd == "equiv") return cmd_equiv(args);
        if (cmd == "opacity") return cmd_opacity(args);
        if (cmd == "reach") return cmd_reach(args);
        if (cmd == "et-opacity") return cmd_et_opacity(args);
        if (cmd == "states") return cmd_states(args);
        std::cerr << "unknown command: " << cmd << "\n\n" << kUsage;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
