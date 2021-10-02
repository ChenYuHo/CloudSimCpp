//
// Created by Chen-Yu Ho on 9/27/21.
//

#ifndef CLOUDSIMCPP_JOB_H
#define CLOUDSIMCPP_JOB_H

#include <climits>
#include <utility>
#include "eventlist.h"
#include "config.h"
#include "worker.h"

class Cluster;

class Job : public EventSource {
private:
    std::shared_ptr<Worker> _worker{};


    static unsigned get_id() {
        static unsigned ID = 0;
        return ID++;
    }

    unsigned _id{};

public:
//    std::shared_ptr<Cluster> cluster(){return _worker->cluster();};
    unsigned id() { return _id; };
    unsigned gpus_required{4};
    unsigned n_iter{1};
    unsigned num_workers_allocated{0};
    simtime_picosec submit_time{ULLONG_MAX};
    simtime_picosec start_time{ULLONG_MAX};
    simtime_picosec finish_time{ULLONG_MAX};
    std::vector<uint64_t> model{50, 150405871, 103492470, 9396470, 159038920, 46686320, 134762900, 4176430};
//            100, 200, 300};//25393, 23232, 64, 307200, 192, 663552, 384, 884736, 256, 589824, 256, 37748736, 4096,
    //16777216, 4096, 4096000, 1000}; // number of parameter (gradient) elements per layer, default to AlexNet size

    explicit Job(std::shared_ptr<Cluster> cluster, simtime_picosec t, EventList &ev) :
            EventSource(ev, "job"),
            submit_time(t),
            _id(get_id()) {
        printf("Job %d constructor invoked\n", this->_id);
    }

    explicit Job(std::shared_ptr<Cluster> cluster, simtime_picosec t, EventList &ev, std::vector<uint64_t> model) :
            EventSource(ev, "job"),
            submit_time(t),
            _id(get_id()),
            model(std::move(model)) {
        printf("Job %d constructor invoked\n", this->_id);
    }

    void doNextEvent() override;
};


#endif //CLOUDSIMCPP_JOB_H
