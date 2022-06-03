#include <algorithm>
#include "custom.h"
#include "cluster.h"
#include "worker.h"
#include "job.h"

std::map<unsigned, unsigned> CustomPlacement::place_job_in(Cluster &cluster, Job *job) {
    std::map<unsigned, unsigned> counter{};
    // counter: wid: num_gpus_allocated
    for (unsigned i=0; i<SWITCH_PORTS; ++i)
        counter[i] = 1;
//    if (job->id == 0) {
//        counter[0] = 1;
//        counter[1] = 1;
//    } else if (job->id == 1) {
//        counter[1] = 1;
//        counter[2] = 1;
//    }
    return counter;
}
