//
// Copyright(c) 2019 Eran Gilad, https://github.com/erangi/kdmt
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef HASHEDKEY_TIMER_H
#define HASHEDKEY_TIMER_H

#include <string>
#include <chrono>

enum class timer_start
{
    Now
};

template<class Resolution>
class TimerGen
{
    using Clock = std::chrono::high_resolution_clock;
    template<class T> class TypeWrapper {};

    Clock::time_point started;

    static std::string toStr(double t, TypeWrapper<std::chrono::seconds>) {
        return std::to_string(t) + " secs";
    }

    static std::string toStr(double t, TypeWrapper<std::chrono::milliseconds>) {
        return std::to_string(t) + " ms";
    }

    static std::string toStr(double t, TypeWrapper<std::chrono::microseconds>) {
        return std::to_string(t) + " us";
    }

    static std::string toStr(double t, TypeWrapper<std::chrono::nanoseconds>) {
        return std::to_string(t) + " ns";
    }

public:

    TimerGen() = default;
    explicit TimerGen(timer_start) {
        start();
    }

    void start() {
        started = Clock::now();
    }

    size_t elapsed() {
        auto dur = std::chrono::duration_cast<Resolution>(Clock::now() - started);
        return dur.count();
    }

    static std::string countToStr(double count) {
        return toStr(count, TypeWrapper<Resolution>());
    }

    std::string elapsed_str() {
        size_t durCount = elapsed();
        return countToStr(durCount);
    }
};

using TimerSec = TimerGen<std::chrono::seconds>;
using timer_ms = TimerGen<std::chrono::milliseconds>;
using TimerUS = TimerGen<std::chrono::microseconds>;
using TimerNS = TimerGen<std::chrono::nanoseconds>;

#endif //HASHEDKEY_TIMER_H
