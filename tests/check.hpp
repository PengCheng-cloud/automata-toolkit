#pragma once

#include <iostream>

// Minimal test helpers (work with or without NDEBUG).
inline int& failures() {
    static int count = 0;
    return count;
}

#define CHECK(cond)                                                                   \
    do {                                                                              \
        if (!(cond)) {                                                                \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond "\n"; \
            ++failures();                                                             \
        }                                                                             \
    } while (0)

#define CHECK_THROWS(expr)                                \
    do {                                                  \
        bool thrown_ = false;                             \
        try {                                             \
            (void)(expr);                                 \
        } catch (...) {                                   \
            thrown_ = true;                               \
        }                                                 \
        CHECK(thrown_ && "expected exception: " #expr);   \
    } while (0)
