#ifndef CLOUDSIMCPP_JOB_SCHEDULER_H
#define CLOUDSIMCPP_JOB_SCHEDULER_H

#include <map>
#include "collective_scheduler.h"

class Job;

class Cluster;

class SchedulingAlgo {
public:
    virtual Job *choose_job_to_execute_in(Cluster &cluster) = 0;
    virtual ~SchedulingAlgo() = default;
};

class PlacementAlgo {
public:
    virtual std::map<unsigned, unsigned> place_job_in(Cluster &, Job *) = 0;
    virtual ~PlacementAlgo() = default;
};

simcpp20::event<SIM_UNIT>
cluster_scheduler(simcpp20::simulation<SIM_UNIT> &sim,
                  Cluster &cluster,
                  SchedulingAlgo *s,
                  CollectiveScheduler *cs);

#endif //CLOUDSIMCPP_JOB_SCHEDULER_H
