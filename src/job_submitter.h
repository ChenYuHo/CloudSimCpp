#ifndef CLOUDSIMCPP_JOB_SUBMITTER_H
#define CLOUDSIMCPP_JOB_SUBMITTER_H

simcpp20::event<SIM_UNIT> broker(
        simcpp20::simulation<SIM_UNIT> &, std::vector<Job *> &, Cluster &,
        bool submit_all_jobs_in_the_beginning = false);
#endif //CLOUDSIMCPP_JOB_SUBMITTER_H
