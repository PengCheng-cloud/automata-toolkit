#include "automata/opacity.hpp"
#include "check.hpp"

using namespace automata;

void test_opacity() {
    OpacityResult leak =
        check_current_state_opacity(read_nfa("examples/leaky_system.nfa"), {1}, {'a', 'b'});
    CHECK(!leak.opaque);
    CHECK(leak.leaking_observation == std::optional<std::string>("b"));
    CHECK((leak.estimate == std::set<int>{1}));

    OpacityResult ok =
        check_current_state_opacity(read_nfa("examples/opaque_system.nfa"), {1}, {'a', 'b'});
    CHECK(ok.opaque);

    // Hiding event 'b' from the attacker removes the leak.
    OpacityResult hidden =
        check_current_state_opacity(read_nfa("examples/leaky_system.nfa"), {1}, {'a'});
    CHECK(hidden.opaque);

    // A secret initial state is revealed by the empty observation.
    NFA trivial(1);
    trivial.add_initial(0);
    OpacityResult t = check_current_state_opacity(trivial, {0}, {'a'});
    CHECK(!t.opaque && t.leaking_observation == std::optional<std::string>(""));
}
