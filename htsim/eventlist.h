#ifndef EVENTLIST_H
#define EVENTLIST_H

#include <map>
#include <ctime>
#include "config.h"
#include "loggertypes.h"
#include "fschuetz04/simcpp20.hpp"

class EventSource;
class EventList {
public:
    EventList(simcpp20::simulation<simtime_picosec> &sim) : _endtime(0), _sim(sim) {};

    void setEndtime(simtime_picosec endtime); // end simulation at endtime (rather than forever)
    bool doNextEvent(); // returns true if it did anything, false if there's nothing to do
    void sourceIsPending(EventSource &src, simtime_picosec when);

    void sourceIsPendingRel(EventSource &src, simtime_picosec timefromnow) {
        sourceIsPending(src, now() + timefromnow);
    }

    inline simtime_picosec now() const { return _sim.now(); }

    inline simcpp20::simulation<simtime_picosec> &sim() const { return _sim; }

private:
    simtime_picosec _endtime;
    typedef multimap<simtime_picosec, EventSource *> pendingsources_t;
    pendingsources_t _pendingsources;
    simcpp20::simulation<simtime_picosec> &_sim;

};

class EventSource : public Logged {
public:
    EventSource(EventList &eventlist, const string &name) : Logged(name),
                                                            _eventlist(eventlist) {};

    ~EventSource() override = default;

    virtual void doNextEvent() = 0;

    inline EventList &eventlist() const { return _eventlist; }

protected:
    EventList &_eventlist;
};

#endif
