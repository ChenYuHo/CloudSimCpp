// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        

#include "eventlist.h"

void
EventList::setEndtime(simtime_picosec endtime)
{
    _endtime = endtime;
}

bool
EventList::doNextEvent()
{
    if (_sim.empty())
        return false;
    _sim.step();
    return true;
}


void
EventList::sourceIsPending(EventSource &src, simtime_picosec when)
{
    if (_endtime == 0 || when < _endtime) {
        auto ev = _sim.event();
        ev.add_callback([&src](const auto &) { src.doNextEvent(); });
        _sim.schedule(ev, when - now());
    }

}
