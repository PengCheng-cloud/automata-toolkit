// Size of the reachable state space with the global clock bound and with
// location-dependent bounds, on a family of models that grows with a parameter.
//
//   make bench
//
// Model "phase chain": a protocol with n phases in a row. Phase i has its own
// clock x_i and must be left between 1 and 5 time units after it was entered.
// The last phase is a long timeout of up to 30 time units. The clocks of the
// other phases keep running; they are never compared with anything outside
// their own phase. Because of the one large constant, the global bound tracks
// every clock up to 31 in every location.
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "automata/timed.hpp"

using namespace automata;

namespace {

TimedAutomaton phase_chain(int phases, int limit, int last_limit) {
    TimedAutomaton ta;
    for (int i = 0; i < phases; ++i) ta.add_clock("x" + std::to_string(i));
    for (int i = 0; i <= phases; ++i) ta.add_location("p" + std::to_string(i));
    ta.set_initial(0);
    for (int i = 0; i < phases; ++i) {
        ta.add_invariant(i, {i, Cmp::LessEq, i + 1 == phases ? last_limit : limit});
        std::vector<int> resets;
        if (i + 1 < phases) resets.push_back(i + 1);
        ta.add_edge({i, i + 1, "next", {{i, Cmp::GreaterEq, 1}}, resets});
    }
    return ta;
}

struct Measurement {
    StateSpaceSize size;
    double milliseconds;
};

Measurement measure(const TimedAutomaton& ta, ClockAbstraction abstraction) {
    auto start = std::chrono::steady_clock::now();
    StateSpaceSize size = explore(ta, abstraction);
    std::chrono::duration<double, std::milli> elapsed = std::chrono::steady_clock::now() - start;
    return {size, elapsed.count()};
}

}  // namespace

int main() {
    std::cout << "phases   global: states       ms   per location: states       ms\n";
    for (int phases = 1; phases <= 7; ++phases) {
        TimedAutomaton ta = phase_chain(phases, 5, 30);
        Measurement global = measure(ta, ClockAbstraction::Global);
        Measurement local = measure(ta, ClockAbstraction::PerLocation);
        if (global.size.locations != local.size.locations) {
            std::cerr << "abstractions disagree on the reachable locations\n";
            return 1;
        }
        std::cout << std::setw(6) << phases << std::setw(17) << global.size.states << std::setw(9)
                  << std::fixed << std::setprecision(1) << global.milliseconds << std::setw(23)
                  << local.size.states << std::setw(9) << local.milliseconds << "\n";
    }
    return 0;
}
