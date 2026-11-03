#include "automata/timed.hpp"
#include "check.hpp"

using namespace automata;

void test_timed() {
    TimedAutomaton leak = read_timed_automaton("examples/timing_leak.ta");
    CHECK(leak.max_constant() == 3);

    // reachability: done is reachable after one delay and one edge
    auto trace = reachability_witness(leak, leak.location_index("done"));
    CHECK(trace.has_value());
    CHECK(trace->size() == 2);
    CHECK((*trace)[0].is_delay);
    CHECK((*trace)[1].label == "first_byte_wrong");

    // the early-exit check leaks the secret through its execution time
    ExecutionTimes t = execution_times(leak, leak.location_index("first_ok"),
                                       leak.location_index("done"), 10);
    CHECK((t.secret_runs == std::set<int>{3}));
    CHECK((t.public_runs == std::set<int>{1}));
    CHECK(!t.opaque());

    // the constant-time version does not
    TimedAutomaton fixed = read_timed_automaton("examples/timing_constant.ta");
    ExecutionTimes f = execution_times(fixed, fixed.location_index("first_ok"),
                                       fixed.location_index("done"), 10);
    CHECK((f.secret_runs == std::set<int>{3}));
    CHECK(f.opaque());

    // a horizon that is too short sees no complete run
    ExecutionTimes s = execution_times(fixed, fixed.location_index("first_ok"),
                                       fixed.location_index("done"), 2);
    CHECK(s.secret_runs.empty() && s.public_runs.empty());

    // an unreachable location
    TimedAutomaton u;
    int a = u.add_location("a");
    int b = u.add_location("b");
    int x = u.add_clock("x");
    u.set_initial(a);
    u.add_invariant(a, {x, Cmp::LessEq, 2});
    u.add_edge({a, b, "go", {{x, Cmp::Greater, 5}}, {}});  // guard can never hold
    CHECK(!reachability_witness(u, b));
}
