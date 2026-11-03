# automata-toolkit

A small C++17 library and command-line tool for working with finite automata.

## Features

- [x] Deterministic finite automata (DFA): construction, simulation, reading from a text file
- [ ] Nondeterministic finite automata (NFA) and subset construction (NFA → DFA)
- [ ] Reachability analysis with BFS
- [ ] Product automaton of two DFAs
- [ ] DFA minimization

## Build

Requires a C++17 compiler (clang++ or g++) and `make`.

```bash
make          # builds build/automata
make test     # builds and runs the unit tests
```

## Usage

```bash
./build/automata examples/ends_with_ab.dfa ab abab aba
```

Output:

```
ab: accepted
abab: accepted
aba: rejected
```

## File format

```
# lines starting with '#' are comments
states 3        # states are numbered 0 .. n-1
start 0
accept 2        # one or more accepting states
0 a 1           # transition: <from> <symbol> <to>
1 b 2
```

## Project structure

```
include/automata/   public headers
src/                library and command-line tool
tests/              unit tests
examples/           example automata
```

## License

MIT
