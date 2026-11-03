# automata-toolkit

[![CI](https://github.com/PengCheng-cloud/automata-toolkit/actions/workflows/ci.yml/badge.svg)](https://github.com/PengCheng-cloud/automata-toolkit/actions/workflows/ci.yml)

A C++17 library and command-line tool for analysing finite automata and
timed automata, with a focus on **opacity**: can an observer infer a secret
from what it sees, for example from the response time of a program?

## Features

**Finite automata**
- DFA and NFA (with ε-transitions): construction, simulation, text file format
- Subset construction (NFA → DFA), only reachable subsets
- Reachability analysis (BFS) with shortest witness words
- Product construction (intersection, union, difference) over the union of both alphabets
- Language inclusion and equivalence with shortest counterexamples
- DFA minimization (Moore's partition refinement), removing unreachable and dead states
- Graphviz export

**Opacity**
- Current-state opacity: an attacker observes only some events and tries to
  determine whether the system is in a secret state. The tool builds the
  attacker's observer (a subset construction over the observable events) and
  returns a shortest leaking observation if one exists.

**Timed automata (integer-time semantics)**
- Clocks, guards, invariants and resets; clock values above the largest
  constant are capped, which keeps the state space finite
- Reachability with a shortest witness run (delays and edges)
- Execution-time opacity: compares the possible execution times of runs that
  visit a secret location with those of runs that do not
- A small timing-attack case study (early-exit password check vs. constant-time check)

## Build

Requires a C++17 compiler (clang++ or g++) and `make`. No other dependencies.

```bash
make          # builds build/automata
make test     # builds and runs all tests
```

The test suite contains unit tests for every module and randomized tests that
cross-check determinization, minimization and equivalence checking on 300
random NFAs. CI runs the tests on Linux and macOS, and with
AddressSanitizer and UndefinedBehaviorSanitizer on Linux.

## Examples

### Timing attack

`examples/timing_leak.ta` models a password check that compares byte by byte
and returns at the first wrong byte. The secret is whether the first byte was
correct; the attacker only measures the response time.

```
$ ./build/automata et-opacity examples/timing_leak.ta first_ok done 10
execution times of runs through first_ok: {3}
execution times of other runs:    {1}
NOT execution-time opaque: some durations reveal whether first_ok was visited

$ ./build/automata et-opacity examples/timing_constant.ta first_ok done 10
execution times of runs through first_ok: {3}
execution times of other runs:    {3}
execution-time opaque: the duration reveals nothing about the secret
```

Shortest run to the final location:

```
$ ./build/automata reach examples/timing_leak.ta done
done is reachable:
  (start, x=0)
  --delay 1--> (start, x=1)
  --first_byte_wrong--> (done, x=1)
```

### Current-state opacity

```
$ ./build/automata opacity examples/leaky_system.nfa 1 ab
NOT opaque: after observing b the attacker knows the state is in {1}

$ ./build/automata opacity examples/opaque_system.nfa 1 ab
current-state opaque: no observation reveals that the system is in {1}
```

### Finite automata

```
$ ./build/automata determinize examples/contains_ab.nfa
# DFA state 0 = NFA states {0, 1}
# DFA state 1 = NFA states {0, 1, 2}
# DFA state 2 = NFA states {0, 1, 3}
# DFA state 3 = NFA states {0, 1, 2, 3}
states 4
...

$ ./build/automata minimize examples/ends_with_ab_redundant.dfa
# 5 states -> 3 states
...

$ ./build/automata equiv examples/ends_with_ab.dfa examples/ends_with_ab_redundant.dfa
equivalent
```

Run `./build/automata` without arguments for all commands.

## File formats

**DFA / NFA** (`.dfa`, `.nfa`)

```
states 4          # states are numbered 0 .. n-1
initial 0         # NFA: one or more initial states (DFA: "start 0")
accept 3          # one or more accepting states
0 a 1             # transition: <from> <symbol> <to>
0 eps 2           # epsilon transition (NFA only)
```

**Timed automata** (`.ta`)

```
clocks x y
locations start check done
initial start
invariant check x<=3                      # conjunction with &
edge start check go reset=x               # edge <from> <to> <label> [guard=...] [reset=...]
edge check done answer guard=x==3&y>=1
```

## Design notes

- Missing transitions mean "reject" (an implicit dead state), so automata stay small.
- The product construction tracks a rejected component as state `-1` instead of
  completing both automata first.
- Integer-time semantics: time advances in steps of one unit. For each clock,
  all values above the largest constant `c` behave the same, so they are capped
  at `c + 1`. The state space is therefore finite and can be searched with BFS.
- Execution-time opacity is computed on the product of the timed automaton
  with a flag "secret visited" and the elapsed time, bounded by a horizon.

## Project structure

```
include/automata/   public headers (dfa, nfa, algorithms, opacity, timed)
src/                library implementation and command-line tool
tests/              unit tests and randomized tests
examples/           example automata
```

## License

MIT
