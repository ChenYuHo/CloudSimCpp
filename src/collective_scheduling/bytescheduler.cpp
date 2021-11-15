#include "bytescheduler.h"
#include "job.h"
#include "cluster.h"
#include "worker.h"

simcpp20::event<SIM_UNIT>
ByteScheduler::enqueue(simcpp20::simulation<SIM_UNIT> &sim, Tensor *tensor) {
    queue[tensor->key].push_back(tensor);
    if (queue[tensor->key].size() == tensor->job->num_workers_allocated) {
        myprintf(7, "putting jid %u tid %u size %u into ready queue\n", tensor->job->id, tensor->tensor_id,
                 tensor->size);
        ready_queue[tensor->job->id].push(std::move(queue[tensor->key]));
        queue.erase(tensor->key);
        if (!active_jobs.contains(tensor->job->id)) { //  && !done_jobs.contain(tensor->job->id)
            active_jobs.insert(tensor->job->id);
            kick_off(sim, tensor->job->id);
        }
    }
    co_await sim.timeout(0);
}

simcpp20::event<SIM_UNIT> ByteScheduler::kick_off(simcpp20::simulation<SIM_UNIT> &sim, unsigned jid) {
    auto &pq = ready_queue[jid];
    while (active_jobs.contains(jid)) {
        if (pq.empty()) {
            co_await sim.timeout(timeFromMs(1));
            continue;
        }
        auto tensors = pq.top();
        auto front = tensors.front();
        if (front->allreduced_size + CHUNK_SIZE >= front->size) {
            myprintf(7, "popping jid %u tid %u\n",
                     jid, front->tensor_id);
            pq.pop();
        }
        std::vector<simcpp20::event<SIM_UNIT>> allreduce_events{};
        myprintf(7, "invoking allreduce for jid %u tid %u iter %u cid %u/%u\n",
                 jid, front->tensor_id, front->iter, front->allreduced_size / CHUNK_SIZE,
                 front->size % CHUNK_SIZE ? front->size / CHUNK_SIZE +1 : front->size / CHUNK_SIZE);
        for (auto tensor:tensors) {
            allreduce_events.push_back(std::move(tensor->machine->allreduce(sim, tensor, CHUNK_SIZE)));
        }
        co_await sim.all_of(allreduce_events);
    }
}

//simcpp20::event<SIM_UNIT> ByteScheduler::run_scheduler_once(simcpp20::simulation<SIM_UNIT> &sim) {
//    if (!ready_queue.empty()) {
////        for (auto &it: ready_queue) {
////        auto pq = it.second;
////        if (!pq.empty()) {
////            auto job_id = it.first;
////                myprintf("job %d has something!!\n", job_id);
////                running_allreduce[job_id] = true;
//        const static auto chunk_size = CHUNK_SIZE;
//
////        if (!running_allreduce.contains(job_id) || !running_allreduce[job_id]) {
//        // allreduce a chunk
//        auto &tensors = ready_queue.top();
//        auto job_id = tensors.front()->job->id;
//        std::vector<simcpp20::event<SIM_UNIT>> allreduce_events;
//        uint64_t allreduce_size;
//        myprintf("jid %u tid %u, %u\n", job_id, tensors.front()->tensor_id, running_allreduce[job_id]);
////        running_allreduce[job_id] = true;
//        for (auto &tensor: tensors) {
//            allreduce_events.push_back(tensor->machine->allreduce(sim, tensor, chunk_size));
//        }
//        auto &t = tensors.front();
////        auto *running = &running_allreduce[job_id];
//        auto ev = sim.all_of(allreduce_events);
//        ev.add_callback([&t, this](const auto &) {
////            *running = false;
//            if (t->allreduced_size >= t->size) {
//                myprintf("allreduce done!!\n");
////                ready_queue.pop();
//            }
//        });
////        }
////        }
////        }
//    }
//    co_await sim.timeout(0);
//}

//simcpp20::event<SIM_UNIT> allreduce_one_chunk_from(
//        simcpp20::simulation<SIM_UNIT> &sim,
//        std::priority_queue<std::vector<Tensor *>> &pq) {
//
//}
//
//void notify_job(unsigned job_id) {
//
//}


simcpp20::event<SIM_UNIT> ByteScheduler::collective_scheduler(
        simcpp20::simulation<SIM_UNIT> &sim,
        Cluster &cluster) {
//    while (!cluster.all_jobs_finished) {
//        for (auto &pair : ready_queue) {
//            auto jid = pair.first;
//            auto &pq = pair.second;
//            if (pq.empty()) continue;
//            auto &tensors = pq.top();
//
//
//        }
//        run_scheduler_once(sim);
    co_await sim.timeout(timeFromSec(1));
//    }
}

void ByteScheduler::cleanup_for_job(unsigned jid) {
    active_jobs.erase(jid);
    ready_queue.erase(jid);
}
