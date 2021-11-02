// Copyright © 2021 Felix Schütz.
// Licensed under the MIT license. See the LICENSE file for details.

#pragma once

#include <cstdint>
#include <queue>
#include "fschuetz04/simcpp20.hpp"

template <typename T=double>
class counter {
public:
    counter(simcpp20::simulation<T> &sim, unsigned needed, unsigned initial_cnt=0)
            : sim{sim}, needed_{needed}, count_{initial_cnt} {}

    simcpp20::event<T> done() {
        auto ev = sim.event();
        evs.push(ev);
        trigger_evs();
        return ev;
    }

    void add_counter() {
        ++count_;
        trigger_evs();
    }

    [[nodiscard]] unsigned count() const { return count_; }

private:
    std::queue<simcpp20::event<T>> evs{};
    simcpp20::simulation<T> &sim;
    unsigned needed_;
    unsigned count_;

    void trigger_evs() {
        while (count() == needed_ && !evs.empty()) {
            auto ev = evs.front();
            evs.pop();
            if (ev.aborted()) {
                continue;
            }
            ev.trigger();
        }
        if (count() == needed_) { // now evs are all triggered, clear counter
            count_ = 0;
        }
    }
};