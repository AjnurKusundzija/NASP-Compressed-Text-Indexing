#pragma once

#include <chrono>

namespace fmi {

class Stopwatch {
public:
    Stopwatch() : start_(Clock::now()) {}

    void restart() { start_ = Clock::now(); }

    double seconds() const {
        return std::chrono::duration<double>(Clock::now() - start_).count();
    }

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point start_;
};

}
