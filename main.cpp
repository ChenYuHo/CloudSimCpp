//#include <cstdio>
#include <iostream>
//#include <utility>
//#include <vector>
//#include <deque>
//#include <list>
//#include <unordered_map>
//#include <map>
//#include <queue>
//#include <ranges>
//#include <random>
//#include <algorithm>
//#include <sstream>
//#include <cstdint>
//#include <climits>
//#include <cstdlib>
//#include <utility>
#include "fschuetz04/simcpp20.hpp"
#include "resource.hpp"
#include "counter.hpp"

#include "config.h"
#include "topology/hierarchical_topology.h"
#include "logfile.h"
#include "job.h"
#include "job_submitter.h"

uint32_t RTT = 1; // this is per link delay in us; identical RTT microseconds = 0.02 ms
int DEFAULT_NODES = 16;

//#define PRINT 1
//#define CHUNK_SIZE strtoull(std::getenv("CHUNK_SIZE"), nullptr, 10) // in number of elements
#define METHOD std::getenv("METHOD")
typedef simtime_picosec SIM_UNIT;

#define JID strtol(std::getenv("JID"), nullptr, 10)

//struct config {
//    int m_per_tor;
//    int n_tor;
//    int g_per_m;
//};

simtime_picosec pkt_transfer_time(uint64_t pkt_size) {
    return pkt_size/*num elements*/* 320; // 100Gbps
}

//class SwitchMLWorker : public PacketSink, public EventSource {
//public:
//    SwitchMLWorker(EventList &eventlist) : EventSource(eventlist, "SwitchMLWorker") {
//
//    };
//
//    uint32_t get_id() { return id; }
//
//    void startflow();
//
//    void doNextEvent();
//
//    virtual void receivePacket(Packet &pkt);
//
//    virtual const string &nodename() { return _nodename; }
//
//    uint32_t _drops;
//    SwitchMLSwitch *_switch;
//    bool _rtx_timeout_pending;
//    const Route *_route;
//
//    void send_packets() {
//
//    };
//private:
//    void retransmit_packet();
//
//    string _nodename;
//};
//
//class SwitchMLSwitch : public PacketSink, public DataReceiver, public Logged {
//    friend class SwitchMLWorker;
//
//public:
//    SwitchMLSwitch();
//
//    void receivePacket(Packet &pkt);
//
//    uint64_t _packets;
//    uint32_t _drops;
//
//    uint32_t get_id() { return id; }
//
//    virtual const string &nodename() { return _nodename; }
//
//    SwitchMLWorker *_workers;
//private:
//    const Route *_route;
//    string _nodename;
//};
//
//
//class Cluster;
//
//class Switch_;
//
//class Machine;
//
//class ByteScheduler;
//
//class NoName;
//
//class DeficitRoundRobin;
//
//class CollectiveScheduler;
//
//struct Tensor {
//    uint64_t size;
//    uint64_t allreduced_size;
//    Machine *machine;
//    std::shared_ptr<Job> job;
//    unsigned tensor_id;
//    unsigned chunk_id;
//    unsigned iter;
//    resource<SIM_UNIT> lock;
//};
//
//struct pkt {
//    uint64_t size;
//    Machine *machine;
//    unsigned tid;
//    unsigned cid;
//    std::shared_ptr<Job> job;
//};
//
////struct AllReduce {
////    uint64_t size;
////    Machine *machine;
////};
//
//
//class Machine {
//private:
//    static unsigned get_id() {
//        static unsigned ID = 0;
//        return ID++;
//    }
//
//public:
//    unsigned id;
//    unsigned gpu{4};
//    unsigned gpu_capacity{4};
//    std::vector<std::shared_ptr<Job>> jobs;
//    Cluster *cluster;
//    std::shared_ptr<Switch_> tor;
//    std::deque<pkt> buffer;
//    std::unordered_map<unsigned, std::unordered_map<unsigned, std::shared_ptr<resource<SIM_UNIT>>>> allreduce_lock;
//
//
//    explicit Machine(Cluster *cluster, std::shared_ptr<Switch_> tor) : id(get_id()), cluster(cluster),
//                                                                       tor(std::move(tor)) {
//        printf("Machine %d constructor invoked\n", id);
//    }
//
//    explicit Machine(config conf, Cluster *cluster, Switch_ *tor) : id(get_id()), gpu(conf.g_per_m),
//                                                                    gpu_capacity(conf.g_per_m), cluster(cluster),
//                                                                    tor(tor) {
//        printf("Machine %d constructor invoked\n", id);
//    }
//
//    //TODO: model forward pass
//    static simtime_picosec forward_pass_time(const uint64_t &size) {
//        return size * 50;
//    }
//
//    //TODO: model backward pass
//    static simtime_picosec backward_pass_time(const uint64_t &size) {
//        return size * 50;
//    }
//
////    int running_jobs() {
////        int n = 0;
////        for (Job* job: jobs)
////            // started but not finished
////            if (job->start_time >= 0 && job->finish_time < 0) n += 1;
////        return n;
////    }
//
////    void finish_job(Job job);
//
////    void start_job(Job job);
//
////    simcpp20::event<SIM_UNIT> run_job(simcpp20::simulation<SIM_UNIT> &sim, const std::shared_ptr<Job> &job) const {
////        job->start_time = sim.now();
////        printf("[%llu]\tMachine %d running job %d\n", sim.now(), id, job->id);
////        for (unsigned i = 0; i < job->n_iter; ++i) {
////            for (const auto &grad_size: job->model) {
////                printf("[%llu]\tMachine %d job %d forward pass for %d started\n", sim.now(), id, job->id, grad_size);
////                co_await sim.timeout(1);
////                printf("[%llu]\tMachine %d job %d forward pass for %d done\n", sim.now(), id, job->id, grad_size);
////            }
////        }
////    }
//    simcpp20::event<SIM_UNIT>
//    execute_job(simcpp20::simulation<SIM_UNIT> &, std::shared_ptr<Job>, unsigned, CollectiveScheduler *);
//
//    simcpp20::event<SIM_UNIT> allreduce(simcpp20::simulation<SIM_UNIT> &, uint64_t, std::string, std::shared_ptr<Job>);
//};
//
//class Switch_ {
//private:
//    static unsigned get_id() {
//        static unsigned ID = 0;
//        return ID++;
//    }
//
//public:
//    unsigned id;
//    std::vector<std::shared_ptr<Machine>> machines;
//    std::deque<pkt> buffer;
//    std::unordered_map<unsigned, unsigned> pkt_counter;
//    std::unordered_map<std::string, std::shared_ptr<counter<SIM_UNIT>>> counter_map;
//    std::unordered_map<unsigned, std::shared_ptr<resource<SIM_UNIT>>> pkt_lock_map;
//    std::unordered_map<unsigned, std::shared_ptr<resource<SIM_UNIT>>> pkt_lock_map2;
////    std::shared_ptr<resource<SIM_UNIT>> pkt_lock;
//
//    explicit Switch_() : id(get_id()) {
//        printf("Switch_ %d constructor invoked\n", id);
//    }
//
////    int running_jobs() {
////        int n = 0;
////        for (const std::shared_ptr<Machine>& m: machines)
////            n += m->running_jobs();
////        return n;
////    }
//    simcpp20::event<SIM_UNIT> send_receive(simcpp20::simulation<SIM_UNIT> &, uint64_t, std::string, unsigned, unsigned);
//};
//
//class Cluster {
//public:
//    Cluster(int no_of_nodes, mem_b queuesize, Logfile *log, EventList *ev, FirstFit *f, queue_type q) :
//            HierarchicalTopology(no_of_nodes, queuesize, log, ev, f, q) {};
//    std::vector<std::shared_ptr<Machine>> machines;
//    std::unordered_map<unsigned, std::shared_ptr<Machine>> machine_map;
//    std::vector<std::shared_ptr<Switch_>> tors;
//    std::vector<std::shared_ptr<Job>> jobs;
//    bool all_jobs_submitted{};
//    bool all_jobs_started{};
//    bool all_jobs_finished{};
//
//    void add_job(const std::shared_ptr<Job> &job) {
//        jobs.push_back(job);
//    }
//
//    void setup(config conf) {
//        for (int i = 0; i < conf.n_tor; i++) {
//            std::shared_ptr<Switch_> s = std::make_shared<Switch_>();
//            for (int j = 0; j < conf.m_per_tor; j++) {
//                std::shared_ptr<Machine> m = std::make_shared<Machine>(this, s);
//                machines.push_back(m);
//                machine_map[m->id] = machines.back();
//                s->machines.push_back(m);
//            }
//            tors.push_back(s);
//        }
//    }
//
//    void check_if_all_jobs_started() {
//        if (!all_jobs_submitted) return;
//        for (const std::shared_ptr<Job> &j:jobs) {
//            if (j->start_time == ULLONG_MAX) return; // at least one job not started yet
//        }
//        all_jobs_started = true;
//    }
//
//    void check_if_all_jobs_finished() {
//        if (!all_jobs_started) return;
//        for (const std::shared_ptr<Job> &j:jobs) {
//            if (j->finish_time == ULLONG_MAX) return; // at least one job not finished yet
//        }
//        all_jobs_finished = true;
//    }
//
//};
//
//
//class CollectiveScheduler {
//public:
////    std::map<unsigned, std::deque<pkt>> queues{}; // each job has a queue
////    virtual void enqueue(uint64_t size, Machine *machine, unsigned tensor_id, unsigned chunk_id,
////                         const std::shared_ptr<Job> &job) = 0;
//    virtual void enqueue(const std::shared_ptr<Tensor> &) = 0;
////    {
////        auto it = queues.find(job->id);
////        if (it == queues.end()) {
////            queues.emplace(job->id, std::deque<pkt>{pkt{size, machine, tensor_id, chunk_id, job}});
////        } else {
////            auto &queue = queues[job->id];
////            queue.emplace_back(pkt{size, machine, tensor_id, chunk_id, job});
////        }
////        printf("queue size %zu\n", queues[job->id].size());
////    }
//
//    virtual simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &, Cluster &) = 0;
//
//    virtual ~CollectiveScheduler() = default;
//};
//
//struct pair_hash {
//    template<class T1, class T2>
//    std::size_t operator()(const std::pair<T1, T2> &pair) const {
//        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
//    }
//};
//
////class FirstInFirstOut : public CollectiveScheduler {
////public:
////    std::unordered_map<std::pair<unsigned, unsigned>, std::deque<std::shared_ptr<Tensor>>, pair_hash> queue;
////    std::queue<std::deque<std::shared_ptr<Tensor>>> ready_queue;
////
////    void enqueue(const std::shared_ptr<Tensor> &tensor) override {
////        auto key = std::make_pair(tensor->job->id, tensor->tensor_id);
////        queue[key].push_back(tensor);
//////        printf("queue size %zu %d %d\n", queue[key].size(), tensor->tensor_id, tensor->job->id);
////        if (queue[key].size() == tensor->job->num_workers_allocated) {
////            ready_queue.push(std::move(queue[key]));
////            queue.erase(key);
////        }
////    }
////
////    simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster) override {
////        while (!cluster.all_jobs_finished) {
////            if (ready_queue.size() > 0) {
////                auto &tensors = ready_queue.front();
////                std::vector<simcpp20::event<SIM_UNIT>> allreduce_events;
////                for (auto &tensor : tensors) {
//////                    printf("[%llu]\tinvoking allreduce within cs s%llu j%d m%d\n", sim.now(),
//////                           tensor->size, tensor->job->id, tensor->machine->id);
////                    auto allreduce_start = sim.now();
////                    auto allreduce_event = tensor->machine->allreduce(sim, tensor->size, std::to_string(tensor->tensor_id),
////                                                                      tensor->job);
////                    allreduce_event.add_callback([&, tensor, allreduce_start](const simcpp20::event<SIM_UNIT> &) {
////                        auto end = sim.now();
////                        if (PRINT) {
////                            printf("[allreduce] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu\n",
////                                   tensor->iter, tensor->job->id, tensor->machine->id, tensor->tensor_id, tensor->size,
////                                   allreduce_start,
////                                   end - allreduce_start, end);
////                        }
////                        tensor->iter++;
////                        tensor->lock.release();
////                    });
////                    allreduce_events.push_back(allreduce_event);
////                }
//////                auto begin = sim.now();
//////                co_await sim.all_of(allreduce_events);
//////                auto end = sim.now();
//////                for (auto &tensor : tensors) {
//////                    if (PRINT) {
//////                        printf("[allreduce] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu\n",
//////                               tensor->iter, tensor->job->id, tensor->machine->id, tensor->tensor_id, tensor->size,
//////                               begin,
//////                               end - begin, end);
//////                    }
//////                    tensor->iter++;
//////                    tensor->lock.release();
//////                }
////                ready_queue.pop();
//////                auto events = sim.all_of(allreduce_events);
//////                events.add_callback([](const simcpp20::event<SIM_UNIT> &event){
//////                    ready_queue.pop();
//////                })
////            }
////            co_await sim.timeout(1e-6);
////        }
////    }
////};
//
//class FirstInFirstOutOneByOne : public CollectiveScheduler {
//public:
//    std::unordered_map<std::pair<unsigned, unsigned>, std::deque<std::shared_ptr<Tensor>>, pair_hash> queue;
//    std::queue<std::deque<std::shared_ptr<Tensor>>> ready_queue;
//
//    void enqueue(const std::shared_ptr<Tensor> &tensor) override {
//        auto key = std::make_pair(tensor->job->id, tensor->tensor_id);
//        queue[key].push_back(tensor);
////        printf("queue size %zu %d %d\n", queue[key].size(), tensor->tensor_id, tensor->job->id);
//        if (queue[key].size() == tensor->job->num_workers_allocated) {
//            ready_queue.push(std::move(queue[key]));
//            queue.erase(key);
//        }
//    }
//
//    simcpp20::event<SIM_UNIT> collective_scheduler(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster) override {
//        while (!cluster.all_jobs_finished) {
//            if (!ready_queue.empty()) {
//                auto &tensors = ready_queue.front();
//                std::vector<simcpp20::event<SIM_UNIT>> allreduce_events;
//                for (auto &tensor : tensors) {
////                    printf("[%llu]\tinvoking allreduce within cs s%llu j%d m%d\n", sim.now(),
////                           tensor->size, tensor->job->id, tensor->machine->id);
//                    allreduce_events.push_back(
//                            tensor->machine->allreduce(sim, tensor->size, std::to_string(tensor->tensor_id),
//                                                       tensor->job));
//                }
//                auto begin = sim.now();
//                co_await sim.all_of(allreduce_events);
//                auto end = sim.now();
//                for (auto &tensor : tensors) {
//                    if (PRINT) {
//                        printf("[allreduce] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu\n",
//                               tensor->iter, tensor->job->id, tensor->machine->id, tensor->tensor_id, tensor->size,
//                               begin,
//                               end - begin, end);
//                    }
//                    tensor->iter++;
//                    tensor->lock.release();
//                }
//                ready_queue.pop();
//            }
//            co_await sim.timeout(1e6);
//        }
//    }
//};
//
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
//            return l->iter == r->iter ? l->tensor_id > r->tensor_id : l->iter > r->iter;
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
//simcpp20::event<SIM_UNIT>
//Switch_::send_receive(simcpp20::simulation<SIM_UNIT> &sim, uint64_t pkt_size, std::string all_ids,
//                      unsigned num_pkts_needed,
//                      unsigned mid) {
//    auto ptr_it = pkt_lock_map.find(mid);
//    if (ptr_it == pkt_lock_map.end()) {
//        pkt_lock_map.emplace(mid, std::make_shared<resource<SIM_UNIT>>(sim, 1));
//    }
//    auto pkt_lock = pkt_lock_map[mid];
////    if(pkt_lock==nullptr) pkt_lock = std::make_shared<resource<SIM_UNIT>>(sim, 1);
//    co_await pkt_lock->request(); // one packet at a time (per link)
//    auto transfer_time = pkt_transfer_time(pkt_size);
//    co_await sim.timeout(transfer_time); // worker to switch
////    printf("[%llu]\tID %d Job packet arrived\n", sim.now(), all_ids);
//    pkt_lock->release();
//    auto it = counter_map.find(all_ids);
//    if (it == counter_map.end()) {
//        counter_map.emplace(all_ids, std::make_shared<counter<SIM_UNIT>>(sim, num_pkts_needed));
//    }
//    counter_map.at(all_ids)->add_counter();
//    co_await counter_map.at(all_ids)->done(); // we have all_ids packet from all _workers
//    auto ptr_it2 = pkt_lock_map2.find(mid);
//    if (ptr_it2 == pkt_lock_map2.end()) {
//        pkt_lock_map2.emplace(mid, std::make_shared<resource<SIM_UNIT>>(sim, 1));
//    }
//    auto pkt_lock2 = pkt_lock_map2[mid];
//    co_await pkt_lock2->request(); // one packet at a time (per link)
//    counter_map.erase(all_ids);
//    co_await sim.timeout(transfer_time); // ToR to worker
//    pkt_lock2->release();
//}
//
//simcpp20::event<SIM_UNIT>
//Machine::allreduce(simcpp20::simulation<SIM_UNIT> &sim, uint64_t grad_size, std::string tensor_id,
//                   std::shared_ptr<Job> job) {
//    auto start = sim.now();
////    printf("[ALLREDUCE] jid %d mid %d tid %s size %llu start %llu\n",
////                           job->id, id, tensor_id.c_str(), grad_size, start);
////    if (job->id == JID)
////        printf("[1,%d]<stdout>:SYNTHETIC COLLECTIVE START %llu %llu\n", id, uint64_t(start * 1e9), grad_size);
////    co_await lock.at(id).at(job->id)->request(); // allreduce goes one by one (non-preemptive)
//
//    const static uint64_t pkt_size = 1024 * 1024; // number of elements
////    printf("[%.3f]\tMachine %d Job %d invoke allreduce on %llu\n", sim.now(), id, job->id, grad_size);
//    std::vector<simcpp20::event<SIM_UNIT>> pkt_events;
//    unsigned i = 0;
//    uint64_t c = 0;
//    for (; i < grad_size / pkt_size; ++i) {
//        std::stringstream s;
//        std::string all_ids;
//        s << job->id << "," << tensor_id << "," << i;
//        s >> all_ids; // job_id, tensor_id, pkt_id
//        printf("[%llu]\tMachine %d Job %d tensor %d send packet %d\n", sim.now(), id, job->id, grad_size, i);
//        pkt_events.push_back(tor->send_receive(sim, pkt_size, all_ids, job->num_workers_allocated, id));
//        co_await sim.timeout(0);
//        c += pkt_size;
//    }
//    if (grad_size % pkt_size) {
//        std::stringstream s;
//        std::string all_ids;
//        s << job->id << "," << tensor_id << "," << i;
//        s >> all_ids; // job_id, tensor_id, pkt_id
//        printf("[%llu]\tMachine %d Job %d tensor %d send packet %d\n", sim.now(), id, job->id, grad_size, i);
//        pkt_events.push_back(tor->send_receive(sim, grad_size % pkt_size, all_ids, job->num_workers_allocated, id));
//        c += grad_size % pkt_size;
//    }
//    assert(c == grad_size);
//    co_await sim.all_of(pkt_events);
////    printf("[%.3f]\tMachine %d done allreduce for Job %d on %d\n", sim.now(), id, job->id, grad_size);
////    auto duration = sim.now() - start;
////    if (job->id == JID) {
////        printf("[1,%d]<stdout>:SYNTHETIC COLLECTIVE PROFILE %llu %llu 1 %llu 0\n", id, uint64_t(start * 1e9),
////               uint64_t(duration * 1e9), grad_size);
////    }
////    lock.at(id).at(job->id)->release();
//}
//
//simcpp20::event<SIM_UNIT>
//Machine::execute_job(simcpp20::simulation<SIM_UNIT> &sim, std::shared_ptr<Job> job, const unsigned gpus_required,
//                     CollectiveScheduler *cs) {
//    jobs.push_back(job);
//    printf("[%llu]\tmachine %d free gpus_required %d, job %d asking %d\n", sim.now(), id, gpu, job->id, gpus_required);
//    gpu -= gpus_required;
//    assert(gpu >= 0);
//    job->start_time = sim.now();
//    cluster->check_if_all_jobs_started();
//
//    std::vector<std::shared_ptr<Tensor>> tensors;
//    unsigned tensor_id = 0;
//    for (auto grad_size: job->model) {
//        tensors.push_back(
//                std::make_shared<Tensor>(Tensor{grad_size, 0, this, job, tensor_id++, 0, 0, resource(sim, 1)}));
//    }
//
//    auto &jid_allreduce_lock = allreduce_lock[id];
//    jid_allreduce_lock.erase(job->id);
//    jid_allreduce_lock.emplace(job->id, std::make_shared<resource<SIM_UNIT>>(sim, 1));
//
//    for (unsigned iter = 0; iter < job->n_iter; ++iter) {
////        if (job->id == JID) printf("[1,%d]<stdout>:SYNTHETIC ITERATION START PROFILE %llu\n", id, sim.now());
//
//        for (auto &tensor: tensors) {
//            co_await tensor->lock.request(); // block if last step's allreduce is not completed yet
//            tensor->allreduced_size = 0;
//            auto fptime = forward_pass_time(tensor->size);
//            auto forward_begin = sim.now();
//            co_await sim.timeout(fptime);
//            if (PRINT) {
//                printf("[forward] iter %d jid %d mid %d tid %d size %llu start %llu duration %llu end %llu\n", iter,
//                       job->id, id, tensor->tensor_id, tensor->size, forward_begin, fptime, sim.now());
//            }
//            tensor->lock.release();
//        }
//        //    printf("[%llu]\tMachine %d job %d forward pass all done\n", sim.now(), id, job->id);
//
//        std::ranges::reverse_view reversed{tensors};
//        unsigned tensor_id = 0;
//        for (auto &tensor: reversed) {
//            auto bptime = backward_pass_time(tensor->size);
//            auto backward_begin = sim.now();
//            co_await sim.timeout(bptime);
//            if (PRINT) {
//                printf("[backward] iter %d jid %d mid %d tid %d size %d start %llu duration %llu end %llu\n", iter,
//                       job->id, id, tensor->tensor_id, tensor->size, backward_begin, bptime, sim.now());
//            }
//            co_await tensor->lock.request(); // release after allreduce is done so next fp can proceed
//            printf("got lock for iter %d jid %d mid %d tid %d size %d\n", tensor->iter,
//                   job->id, id, tensor->tensor_id, tensor->size);
//            cs->enqueue(tensor);
////            auto chunk_size = CHUNK_SIZE;
////            if (chunk_size) {
////                unsigned chunk_id = 0;
////                for (unsigned i = 0; i < tensor.size / chunk_size; ++i) {
////                    cs->enqueue(chunk_size, this, tensor_id, chunk_id++, job);
////                }
////                if (tensor.size % chunk_size) {
////                    cs->enqueue(tensor.size % chunk_size, this, tensor_id, chunk_id++, job);
////                }
////                tensor_id++;
////            } else {
////                cs->enqueue(tensor.size, this, tensor_id++, 0, job);
////            }
////            co_await sim.timeout(0);
////            allreduce_events.push_back(allreduce(sim, grad_size, tensor_id++, job)); // allreduce doesn't block
//        }
////        co_await sim.all_of(allreduce_events);
////        printf("[%llu]\tmachine %d backward loop over\n", sim.now(), id);
////        if(job->id==JID) printf("[1,%d]<stdout>:SYNTHETIC ITERATION END PROFILE %llu\n", id, sim.now());
//    }
//    for (auto &tensor: tensors) {
//        printf("waiting start for iter %d jid %d mid %d tid %d size %lu\n", tensor->iter,
//               job->id, id, tensor->tensor_id, tensor->size);
//        co_await tensor->lock.request(); // wait until final allreduces are done
//        printf("waiting done for iter %d jid %d mid %d tid %d size %lu\n", tensor->iter,
//               job->id, id, tensor->tensor_id, tensor->size);
//    }
//    job->finish_time = sim.now();
//    gpu += gpus_required;
//    printf("[%llu]\tmachine %d finishes job %d and has %d gpus_required now\n", sim.now(), id, job->id, gpu);
//    assert(gpu <= gpu_capacity);
//    cluster->check_if_all_jobs_finished();
//}
//
////void Machine::finish_job(Job job) {
////    gpus_required += job.gpus_required;
////    assert(gpus_required <= gpu_capacity);
////    cluster.check_if_all_jobs_finished();
////}
//
//class SchedulingAlgo {
//public:
//    virtual std::shared_ptr<Job> choose_job_to_execute_in(Cluster &) = 0;
//
//    virtual ~SchedulingAlgo() = default;
//};
//
//class PlacementAlgo {
//public:
//    virtual std::unordered_map<unsigned, unsigned> place_job_in(Cluster &, std::shared_ptr<Job>) = 0;
//
//    virtual ~PlacementAlgo() = default;
//};
//
//class FirstComeFirstServed : public SchedulingAlgo {
//public:
//    std::shared_ptr<Job> choose_job_to_execute_in(Cluster &cluster) override {
////        std::cout << "choose from " << cluster.jobs.size() << " ptr: " << &cluster << std::endl;
//        for (auto &job: cluster.jobs) {
////            std::cout << "j " << job.start_time << std::endl;
//            if (job->start_time == ULLONG_MAX) {
//                return job;
//            }
//        }
//        return nullptr;
//    }
//};
//
//class RandomPlacement : public PlacementAlgo {
//public:
//    std::random_device rd{};
//    std::mt19937 gen{rd()};
//
//    std::unordered_map<unsigned, unsigned> place_job_in(Cluster &cluster, std::shared_ptr<Job> job) override {
//        std::vector<unsigned> candidates;
//        std::unordered_map<unsigned, unsigned> counter;
//        for (auto &machine:cluster.machines) {
//            for (int i = 0; i < machine->gpu; i++) candidates.push_back(machine->id);
//        }
//        if (job->gpus_required > candidates.size()) return counter;
//        std::vector<unsigned> selected;
//        std::ranges::sample(candidates, std::back_inserter(selected), job->gpus_required, gen);
//        for (auto machine_id:selected) {
//            if (counter.find(machine_id) == counter.end()) {
//                counter[machine_id] = 1;
//            } else {
//                counter[machine_id] += 1;
//            }
//        }
//        return counter;
//    }
//};
//
//simcpp20::event<SIM_UNIT>
//broker(simcpp20::simulation<SIM_UNIT> &sim, std::vector<std::shared_ptr<Job>> &jobs, Cluster &cluster) {
//    for (auto &job:jobs) {
//        co_await sim.timeout(job->submit_time - sim.now());
//        job->submitted_time = sim.now();
//        printf("[%llu]\tjob %u arrived\n", sim.now(), job->id);
//        cluster.jobs.push_back(job);
//    }
//    cluster.all_jobs_submitted = true;
//}
//
//simcpp20::event<SIM_UNIT>
//cluster_scheduler(simcpp20::simulation<SIM_UNIT> &sim, Cluster &cluster, SchedulingAlgo *s, PlacementAlgo *p,
//                  CollectiveScheduler *cs) {
//    while (!cluster.all_jobs_started) {
//        std::shared_ptr<Job> job = s->choose_job_to_execute_in(cluster);
//        if (job != nullptr) {
//            printf("[%llu]\tjob %d which requires %d gpus is chosen\n", sim.now(), job->id, job->gpus_required);
//            auto run_config = p->place_job_in(cluster, job);
//            if (run_config.empty()) {
//                printf("[%llu]\tplacement failed for task %d requiring %d GPUs\n", sim.now(), job->id, job->gpus_required);
//            } else {
//                printf("[%llu]\tjob %d placement: ", sim.now(), job->id);
//                job->num_workers_allocated = run_config.size();
//                for (const auto &pair: run_config) {
//                    printf("mid %d -> %d gpus_required ", pair.first, pair.second);
//                    cluster.machine_map[pair.first]->execute_job(sim, job, pair.second, cs);
//                }
//                printf("\n");
//            }
//            co_await sim.timeout(0);
//            continue; // There could be multiple jobs with the same submission timestamp
//        }
//        co_await sim.timeout(1e10);
//    }
//}
//
//constexpr unsigned int str2int(const char *str, int h = 0) {
//    return !str[h] ? 5381 : (str2int(str, h + 1) * 33) ^ str[h];
//}
//
//class TestEventSource : public EventSource {
//public:
//    TestEventSource(EventList &eventlist, const string &name) : EventSource(eventlist, name){};
//    void doNextEvent() override {
//        std::cout << "test event source " << eventlist().now() << std::endl;
//    }
//};

int main() {
    simcpp20::simulation<SIM_UNIT> sim;
    EventList event_list(sim);
//    Logfile logfile("test.log", event_list);

    auto cluster = make_shared<Cluster>(event_list);

    // expect that Job submit time is sorted
    auto jobs = std::vector<std::shared_ptr<Job>>{
            std::make_shared<Job>(cluster, 1e12, event_list),
            std::make_shared<Job>(cluster, 2e12, event_list, std::vector<uint64_t>{1000})
    };

    JobScheduler job_scheduler(event_list, cluster, jobs, "fcfs", "random");
    job_scheduler.start_scheduler();

//    broker(sim, jobs, cluster);
//    cluster_scheduler(sim, cluster, scheduling_algo, placement_algo, cs);
//    cs->collective_scheduler(sim, cluster);
    sim.run_until(4e12);
//    while (event_list.doNextEvent()) {
//    }
    std::cout << "done" << std::endl;
//    delete cs;
    return 0;
}
