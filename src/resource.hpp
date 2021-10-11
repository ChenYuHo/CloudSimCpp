// Copyright © 2021 Felix Schütz.
// Licensed under the MIT license. See the LICENSE file for details.

#pragma once

#include <cstdint>
#include <queue>

#include "fschuetz04/simcpp20.hpp"
#include "config.h"

template<typename T=double>
class resource {
public:
    explicit resource(simcpp20::simulation<T> &sim, uint64_t available=1)
            : sim{sim}, available_{available} {}

    simcpp20::event<T> request() {
        auto ev = sim.event();
        evs.push(ev);
        trigger_evs();
        return ev;
    }

    void release() {
        ++available_;
        trigger_evs();
    }

    uint64_t available() { return available_; }

private:
    std::queue<simcpp20::event<T>> evs{};
    simcpp20::simulation<T> &sim;
    uint64_t available_;

    void trigger_evs() {
        while (available() > 0 && evs.size() > 0) {
            auto ev = evs.front();
            evs.pop();
            if (ev.aborted()) {
                continue;
            }

            ev.trigger();
            --available_;
        }
    }
};