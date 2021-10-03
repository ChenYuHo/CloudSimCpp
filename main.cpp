//#include <cstdio>
#include <iostream>
//#include <utility>
//#include <vector>
//#include <deque>
//#include <list>
//#include <unordered_map>
#include <map>
//#include <queue>
//#include <ranges>
//#include <random>
//#include <algorithm>
//#include <sstream>
//#include <cstdint>
//#include <cstdlib>
//#include <utility>
#include "fschuetz04/simcpp20.hpp"
#include "resource.hpp"
#include "counter.hpp"

//#include "config.h"
//#include "common.h"
#include "cluster.h"
#include "job_submitter.h"
#include "job_scheduling/first_come_first_served.h"
#include "job_placement/random.h"
#include "collective_scheduling/first_in_first_out_one_by_one.h"
#include "job.h"
#include "worker.h"

//#define PRINT 1
//#define CHUNK_SIZE strtoull(std::getenv("CHUNK_SIZE"), nullptr, 10) // in number of elements
//#define METHOD std::getenv("METHOD")
//
//
//#define JID strtol(std::getenv("JID"), nullptr, 10)









//class ByteScheduler : public CollectiveScheduler {
//public:
//    std::unordered_map<std::pair<unsigned, unsigned>, std::deque<std::shared_ptr<Tensor>>, pair_hash> queue;
//
//    struct CustomCompare {
//        bool
//        operator()(const std::deque<std::shared_ptr<Tensor>> &lhs, const std::deque<std::shared_ptr<Tensor>> &rhs) {
//            auto &l = lhs.front();
//            auto &r = rhs.front();
//
//            return l->iter==r->iter ? l->tensor_id > r->tensor_id : l->iter > r->iter;
//        }
//    };
//
//    std::priority_queue<std::deque<std::shared_ptr<Tensor>>, std::vector<std::deque<std::shared_ptr<Tensor>>>, CustomCompare> ready_queue;
//
//    void enqueue(const std::shared_ptr<Tensor> &tensor) override {
//        auto key = std::make_pair(tensor->job->id, tensor->tensor_id);
//        queue[key].push_back(tensor);
////        printf("queue size %zu %d %d\n", queue[key].size(), tensor->tensor_id, tensor->job->id);
//        if (queue[key].size() == tensor->job->num_workers_allocated) {
//            ready_queue.push(std::move(queue[key]));
//            queue.erase(key);
//        }
////        auto key = std::make_pair(tensor->job->id, tensor->tensor_id);
////        queue[key].push_back(tensor);
////        printf("queue size %zu %d %d\n", queue[key].size(), tensor->tensor_id, tensor->job->id);
////        if (queue[key].size() == tensor->job->num_workers_allocated) {
////            std::deque<std::shared_ptr<Tensor>> partitioned;
////            for (auto &t: queue[key]) {
////                auto s = t->size;
////                partitioned.push_back(std::make_shared<Tensor>(Tensor{CHUNK_SIZE, t->machine, t->job, tensor_id++, 0, resource(sim, 1)}));
////            }
////            ready_queue.push(std::move(queue[key]));
////            queue.erase(key);
////        }
//
//    }
//
//    simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster) override {
//        auto chunk_size = CHUNK_SIZE;
//        while (!cluster.all_jobs_finished) {
//            if (!ready_queue.empty()) {
//                std::deque<std::shared_ptr<Tensor>> tensors = ready_queue.top();
//                std::vector<simcpp20::event<SIM_UNIT>> allreduce_events;
//                uint64_t allreduce_size;
//                for (auto &tensor: tensors) {
//                    if (tensor->size - tensor->allreduced_size > chunk_size) {
//                        allreduce_size = chunk_size;
//                    } else {
//                        allreduce_size = tensor->size % chunk_size;
//                    }
//                    allreduce_events.push_back(
//                            tensor->machine->allreduce(sim, allreduce_size, std::to_string(tensor->tensor_id) + "," +
//                                                                            std::to_string(tensor->chunk_id),
//                                                       tensor->job));
//                    tensor->allreduced_size += allreduce_size;
//                    tensor->chunk_id++;
//                }
//
//
//                auto begin = sim.now();
//                co_await sim.all_of(allreduce_events);
//                auto end = sim.now();
//                for (auto &tensor : tensors) {
//                    if (PRINT) {
//                        printf("[allreduce] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu cid %d\n",
//                               tensor->iter, tensor->job->id, tensor->machine->id, tensor->tensor_id, allreduce_size,
//                               begin, end - begin, end, tensor->chunk_id);
//                    }
//                }
//                if (tensors.front()->allreduced_size == tensors.front()->size) {
//                    for (auto &tensor : tensors) {
////                        printf("test\n");
//                        tensor->iter++;
//                        tensor->lock.release();
//                    }
//                    tensors.clear();
//                    ready_queue.pop();
//                }
//            }
//            co_await sim.timeout(1e6);
//        }
//    }
//};
//
////class NoName : public CollectiveScheduler {
////public:
////    std::unordered_map<std::pair<unsigned, unsigned>, std::deque<std::shared_ptr<Tensor>>, pair_hash> queue;
////    std::unordered_map<unsigned, std::priority_queue<std::deque<std::shared_ptr<Tensor>>, std::vector<std::deque<std::shared_ptr<Tensor>>>, CustomCompare>> ready_queue;
////    std::map<std::pair<unsigned, unsigned>, std::deque<pkt>> queues{}; // each job has a queue
////    std::map<unsigned, double>  quantums{}; // each job has a quantum
////    void enqueue(const std::shared_ptr<Tensor> &tensor) override {
////        auto key = std::make_pair(tensor->job->id, tensor->tensor_id);
////        queue[key].push_back(tensor);
////        if (queue[key].size() == tensor->job->num_workers_allocated) {
////            ready_queue[tensor->job->id].push(std::move(queue[key]));
////            queue.erase(key);
////        }
////    }
////    simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster) override {
////
////    }
////};
//
//class DeficitRoundRobin : public CollectiveScheduler {
//public:
//    struct CustomCompare {
//        bool
//        operator()(const std::deque<std::shared_ptr<Tensor>> &lhs, const std::deque<std::shared_ptr<Tensor>> &rhs) {
//            auto &l = lhs.front();
//            auto &r = rhs.front();
//            return l->iter == r->iter ? l->tensor_id > r->tensor_id : l->iter > r->iter;
//        }
//    };
//
//    std::unordered_map<std::pair<unsigned, unsigned>, std::deque<std::shared_ptr<Tensor>>, pair_hash> queue;
////    std::priority_queue<std::deque<std::shared_ptr<Tensor>>, std::vector<std::deque<std::shared_ptr<Tensor>>>, CustomCompare> ready_queue;
//    std::unordered_map<unsigned, std::priority_queue<std::deque<std::shared_ptr<Tensor>>, std::vector<std::deque<std::shared_ptr<Tensor>>>, CustomCompare>> ready_queue;
//    //std::map<unsigned, std::deque<pkt>> queues{}; // each job has a queue
//    std::map<std::pair<unsigned, unsigned>, std::deque<pkt>> queues{}; // each job has a queue
//    std::map<unsigned, double> quantums{}; // each job has a quantum
//    double quantum{2.};
//
//    static double compute_quantum(std::shared_ptr<Tensor> t) {
//        return 1.;
//    }
//
//    void enqueue(const std::shared_ptr<Tensor> &tensor) override {
//        auto key = std::make_pair(tensor->job->id, tensor->tensor_id);
//        queue[key].push_back(tensor);
////        printf("queue size %zu %d %d\n", queue[key].size(), tensor->tensor_id, tensor->job->id);
//        if (queue[key].size() == tensor->job->num_workers_allocated) {
//            ready_queue[tensor->job->id].push(std::move(queue[key]));
//            queue.erase(key);
//        }
//
//
////        auto it = queues.find(tensor->job->id);
////        quantums[tensor->job->id] = 0;
////        if (it == queues.end()) {
////            queues.emplace(tensor->job->id, std::deque<pkt>{pkt{tensor->size, tensor->machine, tensor->tensor_id, tensor->chunk_id, tensor->job}});
////        } else {
////            auto &queue = queues[tensor->job->id];
////            queue.emplace_back(pkt{tensor->size, tensor->machine, tensor->tensor_id, tensor->chunk_id, tensor->job});
////        }
////        printf("queue size %zu\n", queues[tensor->job->id].size());
//    }
//
////
//    simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster) override {
//        auto chunk_size = CHUNK_SIZE;
//        while (!cluster.all_jobs_finished) {
////            printf("scanning through queue\n");
//            for (auto &entry: ready_queue) {
//                auto job_id = entry.first;
//                auto &pqueue = entry.second; // priority queue
//                auto drr = quantums[job_id];
//                drr += quantum;
//                while (!pqueue.empty()) {
//                    printf("job %d drr %f\n", job_id, drr);
//                    auto tensors = pqueue.top();
//                    auto q = compute_quantum(tensors.front());
////                    double q = 1.;
//                    if (drr >= q) {
//                        drr -= q;
//                        std::vector<simcpp20::event<SIM_UNIT>> allreduce_events;
//                        uint64_t allreduce_size;
//                        for (auto &tensor: tensors) {
//                            if (tensor->size - tensor->allreduced_size > chunk_size) {
//                                allreduce_size = chunk_size;
//                            } else {
//                                allreduce_size = tensor->size % chunk_size;
//                            }
//                            allreduce_events.push_back(
//                                    tensor->machine->allreduce(sim, allreduce_size,
//                                                               std::to_string(tensor->tensor_id) + "," +
//                                                               std::to_string(tensor->chunk_id), tensor->job));
//                            tensor->allreduced_size += allreduce_size;
//                            tensor->chunk_id++;
//                        }
//                        auto begin = sim.now();
//                        co_await sim.all_of(allreduce_events);
//                        auto end = sim.now();
//                        for (auto &tensor : tensors) {
//                            if (PRINT) {
//                                printf("[allreduce] iter %d jid %d mid %d tid %d size %lu start %llu duration %llu end %llu cid %d\n",
//                                       tensor->iter, tensor->job->id, tensor->machine->id, tensor->tensor_id,
//                                       allreduce_size, begin, end - begin, end, tensor->chunk_id);
//                            }
//                        }
//                        printf("=== allreduced %lu %lu\n", tensors.front()->allreduced_size, tensors.front()->size);
//                        if (tensors.front()->allreduced_size >= tensors.front()->size) {
//                            for (auto &tensor : tensors) {
//                                tensor->iter++;
//                                tensor->lock.release();
//                            }
//                            tensors.clear();
//                            pqueue.pop();
//                        }
////                        unsigned count = 0;
//
////                        for (auto &p : pqueue) { // check all worker enqueued
////                            if (p.tid == pkt.tid && p.cid == pkt.cid) count++;
////                            if (count == pkt.job->num_workers_allocated) break;
////                        }
////                        if (count == pkt.job->num_workers_allocated) {
////                            for (auto it = pqueue.begin(); it != pqueue.end();) {
////                                auto &p = *it;
////                                if (p.tid == pkt.tid && p.cid == pkt.cid) {
////                                    printf("[%llu]\tinvoking allreduce within cs s%llu t%d c%d j%d m%d\n", sim.now(),
////                                           p.size, p.tid, p.cid, p.job->id, p.machine->id);
////                                    allreduce_events.push_back(p.machine->allreduce(sim, p.size, p.tid, p.cid, p.job));
////                                    it = pqueue.erase(it);
////                                } else {
////                                    ++it;
////                                }
////                            }
////                        }
////                        co_await sim.all_of(allreduce_events);
////                        pkt.machine->allreduce(sim, pkt.size, pkt.tid, pkt.job);
////                        pqueue.pop_front();
//                    } else break;
//                }
//            }
//            co_await sim.timeout(1e6);
//        }
//
//    }
//};
//
//
//class MyStrategy : public CollectiveScheduler {
//public:
//    struct CustomCompare {
//        bool
//        operator()(const std::deque<std::shared_ptr<Tensor>> &lhs, const std::deque<std::shared_ptr<Tensor>> &rhs) {
//            auto &l = lhs.front();
//            auto &r = rhs.front();
//            return l->iter == r->iter ? l->tensor_id > r->tensor_id : l->iter > r->iter;
//        }
//    };
//
//    std::unordered_map<std::pair<unsigned, unsigned>, std::deque<std::shared_ptr<Tensor>>, pair_hash> queue;
////    std::priority_queue<std::deque<std::shared_ptr<Tensor>>, std::vector<std::deque<std::shared_ptr<Tensor>>>, CustomCompare> ready_queue;
//    std::unordered_map<unsigned, std::priority_queue<std::deque<std::shared_ptr<Tensor>>, std::vector<std::deque<std::shared_ptr<Tensor>>>, CustomCompare>> ready_queue;
//    // jid -> priority queue
//    std::map<std::pair<unsigned, unsigned>, std::deque<pkt>> queues{}; // each job has a queue
//    std::map<unsigned, double> quantums{}; // each job has a quantum
//
//    void enqueue(const std::shared_ptr<Tensor> &tensor) override {
//        auto key = std::make_pair(tensor->job->id, tensor->tensor_id);
//        queue[key].push_back(tensor);
////        printf("queue size %zu %d %d\n", queue[key].size(), tensor->tensor_id, tensor->job->id);
//        if (queue[key].size() == tensor->job->num_workers_allocated) {
//            ready_queue[tensor->job->id].push(std::move(queue[key]));
//            queue.erase(key);
//        }
//    }
//
////
//    simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster) override {
//        auto chunk_size = CHUNK_SIZE;
//        while (!cluster.all_jobs_finished) {
////            printf("scanning through queue\n");
//            for (auto &entry: ready_queue) {
//                auto job_id = entry.first;
//                auto &queue = entry.second; // priority queue
//                auto drr = quantums[job_id];
//                while (!queue.empty()) {
//                    printf("job %d drr %f\n", job_id, drr);
//                    auto tensors = queue.top();
//                    std::vector<simcpp20::event<SIM_UNIT>> allreduce_events;
//                    uint64_t allreduce_size;
//                    for (auto &tensor: tensors) {
//                        if (tensor->size - tensor->allreduced_size > chunk_size) {
//                            allreduce_size = chunk_size;
//                        } else {
//                            allreduce_size = tensor->size % chunk_size;
//                        }
//                        allreduce_events.push_back(
//                                tensor->machine->allreduce(sim, allreduce_size,
//                                                           std::to_string(tensor->tensor_id) + "," +
//                                                           std::to_string(tensor->chunk_id), tensor->job));
//                        tensor->allreduced_size += allreduce_size;
//                        tensor->chunk_id++;
//                    }
//                    auto begin = sim.now();
//                    co_await sim.all_of(allreduce_events);
//                    auto end = sim.now();
//                    for (auto &tensor : tensors) {
//                        if (PRINT) {
//                            printf("[allreduce] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu cid %d\n",
//                                   tensor->iter, tensor->job->id, tensor->machine->id, tensor->tensor_id,
//                                   allreduce_size, begin, end - begin, end, tensor->chunk_id);
//                        }
//                    }
//                    printf("=== allreduced %llu %llu\n", tensors.front()->allreduced_size, tensors.front()->size);
//                    if (tensors.front()->allreduced_size >= tensors.front()->size) {
//                        for (auto &tensor : tensors) {
//                            tensor->iter++;
//                            tensor->lock.release();
//                        }
//                        tensors.clear();
//                        queue.pop();
//                    }
//                }
//            }
//            co_await sim.timeout(1e6);
//        }
//
//    }
//};
//
//
//


//void Worker::finish_job(Job job) {
//    gpu += job.gpu;
//    assert(gpu <= gpu_capacity);
//    cluster.check_if_all_jobs_finished();
//}

//class SchedulingAlgo {
//public:
//    virtual std::shared_ptr<Job> choose_job_to_execute_in(Cluster &) = 0;
//    virtual ~SchedulingAlgo() = default;
//};
//
//class PlacementAlgo {
//public:
//    virtual std::unordered_map<unsigned, unsigned> place_job_in(Cluster &, std::shared_ptr<Job>) = 0;
//    virtual ~PlacementAlgo() = default;
//};









//constexpr unsigned int str2int(const char* str, int h = 0)
//{
//    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
//}

int main() {
    simcpp20::simulation<SIM_UNIT> sim;

    config conf{
            .m_per_tor = 2,
            .n_tor = 1,
            .g_per_m = 4,
    };

    std::vector<std::shared_ptr<Job>> jobs = std::vector<std::shared_ptr<Job>>{
            std::make_shared<Job>(1e12, sim),
            std::make_shared<Job>(1e12, sim,
                                  std::vector<uint64_t>{1000}
//                                  std::vector<uint64_t>{25393, 23232, 64, 307200, 192, 663552, 384, 884736, 256,
//                                                        589824, 256, 37748736, 4096, 16777216, 4096, 4096000, 1000}
            )};
    Cluster cluster = Cluster();
    cluster.setup(conf);
    PlacementAlgo *placement_algo;
    SchedulingAlgo *scheduling_algo;
    RandomPlacement r = RandomPlacement();
    placement_algo = &r;
    FirstComeFirstServed f = FirstComeFirstServed();
    scheduling_algo = &f;

    CollectiveScheduler *cs = new FirstInFirstOutOneByOne();
//    auto m = METHOD;
//    switch(str2int(METHOD))
//    {
//        case str2int("drr"):
//            cs = new DeficitRoundRobin();
//            break;
//        case str2int("bytescheduler"):
//            cs = new ByteScheduler();
//            break;
//        default:
//            cs = new FirstInFirstOutOneByOne();
//            break;
//    }

    broker(sim, jobs, cluster);
    cluster_scheduler(sim, cluster, scheduling_algo, placement_algo, cs);
    cs->collective_scheduler(sim, cluster);
    sim.run();
//    delete placement_algo;
//    delete scheduling_algo;
    std::cout << "done" << std::endl;
    delete cs;
    return 0;
}
