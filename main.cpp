#include <cstdio>
#include <iostream>
#include <utility>
#include <vector>
#include <unordered_map>
#include <ranges>
#include <random>
#include <algorithm>
#include "fschuetz04/simcpp20.hpp"

struct config {
    int m_per_tor;
    int n_tor;
    int g_per_m;
};

class Cluster;

class Switch;

class Job {
public:
    unsigned id{};
    int gpu{3};
    double submit_time;
    double submitted_time{};
    double start_time{-1.};
    double finish_time{-1.};
    unsigned n_iter{5};
    std::vector<unsigned> model{
            100};//25393, 23232, 64, 307200, 192, 663552, 384, 884736, 256, 589824, 256, 37748736, 4096,
    //16777216, 4096, 4096000, 1000}; // default to AlexNet size

    explicit Job(double t) : submit_time(t), id(unsigned(t)) {
        printf("Job %d constructor invoked\n", unsigned(t));
    }

//    simcpp20::event<> run(simcpp20::simulation<> &sim, const std::unordered_map<unsigned, unsigned> &run_config) {
//        start_time = sim.now();
//        co_await sim.timeout(10);
//        finish_time = sim.now();
//    }
};

class Machine {
private:
    static unsigned get_id() {
        static unsigned ID = 0;
        return ID++;
    }

public:
    unsigned id;
    int gpu{4};
    int gpu_capacity{};
    std::vector<std::shared_ptr<Job>> jobs;
    Cluster *cluster;
    std::shared_ptr<Switch> tor;

    explicit Machine(Cluster *cluster, std::shared_ptr<Switch> tor) : id(get_id()), cluster(cluster),
                                                                      tor(std::move(tor)) {
        printf("Machine %d constructor invoked\n", id);
    }

    explicit Machine(config conf, Cluster *cluster, Switch *tor) : id(get_id()), gpu(conf.g_per_m), cluster(cluster),
                                                                   tor(tor) {
        printf("Machine %d constructor invoked\n", id);
    }

    static double forward_pass_time(const unsigned &size) {
        return 1;
    }



//    int running_jobs() {
//        int n = 0;
//        for (Job* job: jobs)
//            // started but not finished
//            if (job->start_time >= 0 && job->finish_time < 0) n += 1;
//        return n;
//    }

//    void finish_job(Job job);

//    void start_job(Job job);

//    simcpp20::event<> run_job(simcpp20::simulation<> &sim, const std::shared_ptr<Job> &job) const {
//        job->start_time = sim.now();
//        printf("[%.0f]\tMachine %d running job %d\n", sim.now(), id, job->id);
//        for (unsigned i = 0; i < job->n_iter; ++i) {
//            for (const auto &grad_size: job->model) {
//                printf("[%.0f]\tMachine %d job %d forward pass for %d started\n", sim.now(), id, job->id, grad_size);
//                co_await sim.timeout(1);
////                printf("[%.0f]\tMachine %d job %d forward pass for %d done\n", sim.now(), id, job->id, grad_size);
//            }
//        }
//    }

    void execute_job(std::shared_ptr<Job> job);
};

class Switch {
private:
    static unsigned get_id() {
        static unsigned ID = 0;
        return ID++;
    }

public:
    unsigned id;
    std::vector<std::shared_ptr<Machine>> machines;

    Switch() : id(get_id()) {
        printf("Switch %d constructor invoked\n", id);
    }

//    int running_jobs() {
//        int n = 0;
//        for (const std::shared_ptr<Machine>& m: machines)
//            n += m->running_jobs();
//        return n;
//    }
};

class Cluster {
public:
    std::vector<std::shared_ptr<Machine>> machines;
    std::unordered_map<unsigned, std::shared_ptr<Machine>> machine_map;
    std::vector<std::shared_ptr<Switch>> tors;
    std::vector<std::shared_ptr<Job>> jobs;
    bool all_jobs_submitted{};
    bool all_jobs_started{};
    bool all_jobs_finished{};

    void add_job(const std::shared_ptr<Job> &job) {
        jobs.push_back(job);
    }

    void setup(config conf) {
        for (int i = 0; i < conf.n_tor; i++) {
            std::shared_ptr<Switch> s = std::make_shared<Switch>();
            for (int j = 0; j < conf.m_per_tor; j++) {
                std::shared_ptr<Machine> m = std::make_shared<Machine>(this, s);
                machines.push_back(m);
                machine_map[m->id] = machines.back();
                s->machines.push_back(m);
            }
            tors.push_back(s);
        }
    }

    void check_if_all_jobs_started() {
        if (!all_jobs_submitted) return;
        for (const std::shared_ptr<Job> &j:jobs) {
            if (j->start_time < 0) return; // at least one job not started yet
        }
        all_jobs_started = true;
    }

    void check_if_all_jobs_finished() {
        if (!all_jobs_started) return;
        for (const std::shared_ptr<Job> &j:jobs) {
            if (j->finish_time < 0) return; // at least one job not finished yet
        }
        all_jobs_finished = true;
    }

};

void Machine::execute_job(std::shared_ptr<Job> job) {
    jobs.push_back(job);
    gpu -= job->gpu;
    assert(gpu >= 0);
    cluster->check_if_all_jobs_started();
}

//void Machine::finish_job(Job job) {
//    gpu += job.gpu;
//    assert(gpu <= gpu_capacity);
//    cluster.check_if_all_jobs_finished();
//}

class SchedulingAlgo {
public:
    virtual std::shared_ptr<Job> choose_job_to_execute_in(Cluster &) = 0;

    virtual ~SchedulingAlgo() = default;
};

class PlacementAlgo {
public:
    virtual std::unordered_map<unsigned, unsigned> place_job_in(Cluster &, std::shared_ptr<Job>) = 0;

    virtual ~PlacementAlgo() = default;
};

class FirstComeFirstServed : public SchedulingAlgo {
public:
    std::shared_ptr<Job> choose_job_to_execute_in(Cluster &cluster) override {
//        std::cout << "choose from " << cluster.jobs.size() << " ptr: " << &cluster << std::endl;
        for (auto &job: cluster.jobs) {
//            std::cout << "j " << job.start_time << std::endl;
            if (job->start_time < 0) {
                return job;
            }
        }
        return nullptr;
    }
};

class RandomPlacement : public PlacementAlgo {
public:
    std::random_device rd{};
    std::mt19937 gen{rd()};

    std::unordered_map<unsigned, unsigned> place_job_in(Cluster &cluster, std::shared_ptr<Job> job) override {
        std::vector<unsigned> candidates;
        std::unordered_map<unsigned, unsigned> counter;
        for (auto &machine:cluster.machines) {
            for (int i = 0; i < machine->gpu; i++) candidates.push_back(machine->id);
        }
        if (job->gpu > candidates.size()) return counter;
        std::vector<unsigned> selected;
        std::ranges::sample(candidates, std::back_inserter(selected), job->gpu, gen);
        for (auto machine_id:selected) {
            if (counter.find(machine_id) == counter.end()) {
                counter[machine_id] = 1;
            } else {
                counter[machine_id] += 1;
            }
        }
        return counter;
    }
};

simcpp20::event<> broker(simcpp20::simulation<> &sim, std::vector<std::shared_ptr<Job>> &jobs, Cluster &cluster) {
    for (auto &job:jobs) {
        co_await sim.timeout(job->submit_time - sim.now());
        job->submitted_time = sim.now();
        printf("[%.0f]\tjob %d arrived\n", sim.now(), job->id);
//        cluster.add_job(job);
        cluster.jobs.push_back(job);
//        std::cout << "b " << cluster.jobs.size() << " ptr " << &cluster << std::endl;
    }
    cluster.all_jobs_submitted = true;
}

simcpp20::event<> scheduler(simcpp20::simulation<> &sim, Cluster &cluster, SchedulingAlgo *s, PlacementAlgo *p) {
    while (!cluster.all_jobs_started) {
        std::shared_ptr<Job> job = s->choose_job_to_execute_in(cluster);
//        std::cout << sim.now() << " " << (job == nullptr) << std::endl;
        if (job != nullptr) {
            printf("[%.0f]\tjob %d which requires %d gpus is chosen\n", sim.now(), job->id, job->gpu);
            auto run_config = p->place_job_in(cluster, job);
            printf("[%.0f]\tjob %d placement: ", sim.now(), job->id);
            if (run_config.empty()) {
                printf("[%.0f]\tplacement failed for task %d requiring %d GPUs\n", sim.now(), job->id, job->gpu);
            } else {
                for (const auto &pair: run_config) {
                    printf("mid %d -> %d gpu ", pair.first, pair.second);
                    cluster.machine_map[pair.first]->execute_job(job);
                }
                printf("\n");
//                job->run(sim, run_config);
            }
        }
        co_await sim.timeout(1);

    }
}


int main() {
    simcpp20::simulation<> sim;

    config conf{
            .m_per_tor = 4,
            .n_tor = 2,
            .g_per_m = 4,
    };

    std::vector<std::shared_ptr<Job>> jobs = std::vector<std::shared_ptr<Job>>{std::make_shared<Job>(1.),
                                                                               std::make_shared<Job>(4.),
                                                                               std::make_shared<Job>(9.)};
    Cluster cluster = Cluster();
    cluster.setup(conf);
    PlacementAlgo *placement_algo;
    SchedulingAlgo *scheduling_algo;
    placement_algo = new RandomPlacement();
    scheduling_algo = new FirstComeFirstServed();


    broker(sim, jobs, cluster);
    scheduler(sim, cluster, scheduling_algo, placement_algo);
    sim.run();
    delete placement_algo;
    delete scheduling_algo;
    return 0;
}
