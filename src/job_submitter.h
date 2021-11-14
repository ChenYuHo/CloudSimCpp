#ifndef CLOUDSIMCPP_JOB_SUBMITTER_H
#define CLOUDSIMCPP_JOB_SUBMITTER_H

simcpp20::event<SIM_UNIT> broker(
        simcpp20::simulation<SIM_UNIT> &sim,
        std::vector<Job *> &jobs,
        Cluster &cluster);
#endif //CLOUDSIMCPP_JOB_SUBMITTER_H
