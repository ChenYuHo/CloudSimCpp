#include "ready_and_go.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"

void ReadyAndGo::enqueue(const std::shared_ptr<Tensor> &tensor) {
    auto key = std::make_pair(tensor->job->id, tensor->tensor_id);
    queue[key].push_back(tensor);
    if (queue[key].size() == tensor->job->num_workers_allocated) {
        for(auto & t: queue[key]) {
            myprintf("Allreduce go %p\n", t.get());
            t->machine->allreduce(sim, t);

        }
        queue[key].clear();
    }
}
