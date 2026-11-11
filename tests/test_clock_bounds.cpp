// Location-dependent clock bounds: unit tests and a randomized cross-check
// against the global bound.
#include <random>

#include "automata/timed.hpp"
#include "check.hpp"

using namespace automata;

namespace {

// A chain of `phases` locations. Phase i has its own clock x_i, must be left
// between 1 and `limit` time units after it was entered, and the edge to the next
// phase resets the next clock. All other clocks keep running.
TimedAutomaton phase_chain(int phases, int limit) {
    TimedAutomaton ta;
    for (int i = 0; i < phases; ++i) ta.add_clock("x" + std::to_string(i));
    for (int i = 0; i <= phases; ++i) ta.add_location("p" + std::to_string(i));
    ta.set_initial(0);
    for (int i = 0; i < phases; ++i) {
        ta.add_invariant(i, {i, Cmp::LessEq, limit});
        std::vector<int> resets;
        if (i + 1 < phases) resets.push_back(i + 1);
        ta.add_edge({i, i + 1, "next", {{i, Cmp::GreaterEq, 1}}, resets});
    }
    return ta;
}

TimedAutomaton random_ta(std::mt19937& rng) {
    std::uniform_int_distribution<int> n_loc(2, 5), n_clock(1, 3), constant(0, 6), op(0, 4), coin(0, 1);
    TimedAutomaton ta;
    int clocks = n_clock(rng), locations = n_loc(rng);
    for (int c = 0; c < clocks; ++c) ta.add_clock("x" + std::to_string(c));
    for (int l = 0; l < locations; ++l) ta.add_location("l" + std::to_string(l));
    ta.set_initial(0);
    std::uniform_int_distribution<int> any_clock(0, clocks - 1), any_loc(0, locations - 1);
    const Cmp ops[] = {Cmp::Less, Cmp::LessEq, Cmp::Equal, Cmp::GreaterEq, Cmp::Greater};

    for (int l = 0; l < locations; ++l)
        if (coin(rng)) ta.add_invariant(l, {any_clock(rng), Cmp::LessEq, 1 + constant(rng)});

    std::uniform_int_distribution<int> n_edges(locations, 3 * locations);
    for (int e = n_edges(rng); e > 0; --e) {
        TAEdge edge{any_loc(rng), any_loc(rng), "e", {}, {}};
        for (int g = coin(rng) + coin(rng); g > 0; --g)
            edge.guard.push_back({any_clock(rng), ops[op(rng)], constant(rng)});
        for (int c = 0; c < clocks; ++c)
            if (coin(rng) && coin(rng)) edge.resets.push_back(c);
        ta.add_edge(edge);
    }
    return ta;
}

}  // namespace

void test_clock_bounds() {
    // --- relevant_bounds on the timing-attack example ---
    // x is compared with 1 in start and with 3 in first_ok, which is reached from start
    // without a reset. In done nothing depends on x any more.
    TimedAutomaton leak = read_timed_automaton("examples/timing_leak.ta");
    auto bounds = leak.relevant_bounds();
    CHECK(bounds[leak.location_index("start")][0] == 3);
    CHECK(bounds[leak.location_index("first_ok")][0] == 3);
    CHECK(bounds[leak.location_index("done")][0] == -1);

    // a reset cuts the propagation: after the reset only the constant 2 matters
    TimedAutomaton r;
    int a = r.add_location("a"), b = r.add_location("b"), c = r.add_location("c");
    int x = r.add_clock("x");
    r.set_initial(a);
    r.add_edge({a, b, "go", {{x, Cmp::GreaterEq, 7}}, {x}});
    r.add_edge({b, c, "go", {{x, Cmp::LessEq, 2}}, {}});
    auto rb = r.relevant_bounds();
    CHECK(rb[a][x] == 7);
    CHECK(rb[b][x] == 2);
    CHECK(rb[c][x] == -1);

    // --- phase chain: the global bound keeps every clock apart everywhere ---
    TimedAutomaton chain = phase_chain(4, 5);
    StateSpaceSize global = explore(chain, ClockAbstraction::Global);
    StateSpaceSize local = explore(chain, ClockAbstraction::PerLocation);
    CHECK(global.locations == local.locations);
    CHECK(local.states < global.states);
    // per location: one live clock with values 0..5, the final location has one state
    CHECK(local.states == 4 * 6 + 1);

    // both abstractions give the same execution times
    ExecutionTimes tg = execution_times(chain, 2, 4, 30, ClockAbstraction::Global);
    ExecutionTimes tl = execution_times(chain, 2, 4, 30, ClockAbstraction::PerLocation);
    CHECK(tg.secret_runs == tl.secret_runs);
    CHECK(tg.public_runs == tl.public_runs);
    CHECK((*tl.secret_runs.begin() == 4) && (*tl.secret_runs.rbegin() == 20));

    // --- randomized cross-check on 1000 timed automata ---
    std::mt19937 rng(20261111);
    int smaller = 0;
    for (int round = 0; round < 1000; ++round) {
        TimedAutomaton ta = random_ta(rng);
        StateSpaceSize g = explore(ta, ClockAbstraction::Global);
        StateSpaceSize l = explore(ta, ClockAbstraction::PerLocation);
        CHECK(g.locations == l.locations);
        CHECK(l.states <= g.states);
        if (l.states < g.states) ++smaller;

        for (int target = 0; target < ta.num_locations(); ++target) {
            auto wg = reachability_witness(ta, target, ClockAbstraction::Global);
            auto wl = reachability_witness(ta, target, ClockAbstraction::PerLocation);
            CHECK(wg.has_value() == wl.has_value());
            if (wg && wl) CHECK(wg->size() == wl->size());  // both are shortest runs
        }

        std::uniform_int_distribution<int> any_loc(0, ta.num_locations() - 1);
        int secret = any_loc(rng), final_location = any_loc(rng);
        ExecutionTimes eg = execution_times(ta, secret, final_location, 15, ClockAbstraction::Global);
        ExecutionTimes el = execution_times(ta, secret, final_location, 15, ClockAbstraction::PerLocation);
        CHECK(eg.secret_runs == el.secret_runs);
        CHECK(eg.public_runs == el.public_runs);
    }
    CHECK(smaller > 0);  // the finer abstraction does make a difference on random models
}
