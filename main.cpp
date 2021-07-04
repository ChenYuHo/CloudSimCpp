#include <cstdio>
#include <iostream>
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

    explicit Job(double t) : submit_time(t), id(unsigned(t)) {}

    simcpp20::event<> run(simcpp20::simulation<> &sim, std::unordered_map<unsigned, unsigned> run_config) {
        start_time = sim.now();
        co_await sim.timeout(10);
        finish_time = sim.now();
    }
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
    std::vector<Job> jobs;
    Cluster &cluster;
    Switch &tor;

    explicit Machine(Cluster &cluster, Switch &tor) : id(get_id()), cluster(cluster), tor(tor) {}

    explicit Machine(config conf, Cluster &cluster, Switch &tor) : id(get_id()), gpu(conf.g_per_m), cluster(cluster),
                                                                   tor(tor) {}

    int running_jobs() {
        int n = 0;
        for (Job &job: jobs)
            // started but not finished
            if (job.start_time >= 0 && job.finish_time < 0) n += 1;
        return n;
    }

    void finish_job(Job job);

    void start_job(Job job);
};

class Switch {
private:
    static unsigned get_id() {
        static unsigned ID = 0;
        return ID++;
    }

public:
    unsigned id;
    std::vector<Machine> machines;

    Switch() : id(get_id()) {}

    int running_jobs() {
        int n = 0;
        for (Machine &m: machines)
            n += m.running_jobs();
        return n;
    }
};

class Cluster {
public:
    std::vector<Machine> machines;
    std::vector<Switch> tors;
    std::vector<Job> jobs;
    bool all_jobs_submitted{};
    bool all_jobs_started{};
    bool all_jobs_finished{};

    void add_job(Job job) {
        jobs.push_back(job);
    }

    void setup(config conf) {
        for (int i = 0; i < conf.n_tor; i++) {
            Switch s = Switch();
            for (int j = 0; j < conf.m_per_tor; j++) {
                Machine m = Machine(*this, s);
                machines.push_back(m);
                s.machines.push_back(m);
            }
            tors.push_back(s);
        }
    }

    void check_if_all_jobs_started() {
        if (!all_jobs_submitted) return;
        for (Job &j:jobs) {
            if (j.start_time < 0) return; // at least one job not started yet
        }
        all_jobs_started = true;
    }

    void check_if_all_jobs_finished() {
        if (!all_jobs_started) return;
        for (Job &j:jobs) {
            if (j.finish_time < 0) return; // at least one job not finished yet
        }
        all_jobs_finished = true;
    }

};

void Machine::start_job(Job job) {
    jobs.push_back(job);
    gpu -= job.gpu;
    assert(gpu >= 0);
    cluster.check_if_all_jobs_started();
}

void Machine::finish_job(Job job) {
    gpu += job.gpu;
    assert(gpu <= gpu_capacity);
    cluster.check_if_all_jobs_finished();
}

class SchedulingAlgo {
public:
    virtual Job *choose_job_to_execute_in(Cluster &) = 0;
    virtual ~SchedulingAlgo() = default;
};

class PlacementAlgo {
public:
    virtual std::unordered_map<unsigned, unsigned> place_job_in(Cluster &, Job *) = 0;
    virtual ~PlacementAlgo() = default;
};

class FirstComeFirstServed : public SchedulingAlgo {
public:
    Job *choose_job_to_execute_in(Cluster &cluster) override {
//        std::cout << "choose from " << cluster.jobs.size() << " ptr: " << &cluster << std::endl;
        for (auto &job:cluster.jobs) {
//            std::cout << "j " << job.start_time << std::endl;
            if (job.start_time < 0) {
                return &job;
            }
        }
        return nullptr;
    }
};

class RandomPlacement : public PlacementAlgo {
public:
    std::random_device rd{};
    std::mt19937 gen{rd()};

    std::unordered_map<unsigned, unsigned> place_job_in(Cluster &cluster, Job *job) override {
        std::vector<unsigned> candidates;
        std::unordered_map<unsigned, unsigned> counter;
        for (auto &machine:cluster.machines) {
            for (int i = 0; i < machine.gpu; i++) candidates.push_back(machine.id);
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

simcpp20::event<> broker(simcpp20::simulation<> &sim, std::vector<Job> &jobs, Cluster &cluster) {
    for (auto &job:jobs) {
        co_await sim.timeout(job.submit_time - sim.now());
        job.submitted_time = sim.now();
        printf("[%.0f]\tjob %d arrived\n", sim.now(), job.id);
//        cluster.add_job(job);
        cluster.jobs.push_back(job);
//        std::cout << "b " << cluster.jobs.size() << " ptr " << &cluster << std::endl;
    }
    cluster.all_jobs_submitted = true;
}

simcpp20::event<> scheduler(simcpp20::simulation<> &sim, Cluster &cluster, SchedulingAlgo *s, PlacementAlgo *p) {
    while (!cluster.all_jobs_started) {
        Job *job = s->choose_job_to_execute_in(cluster);
//        std::cout << sim.now() << " " << (job == nullptr) << std::endl;
        if (job != nullptr) {
            printf("[%.0f]\tjob %d which requires %d gpus is chosen\n", sim.now(), job->id, job->gpu);
            auto run_config = p->place_job_in(cluster, job);
            printf("[%.0f]\tjob %d placement: ", sim.now(), job->id);
            if (run_config.empty()) {
                printf("[%.0f]\tplacement failed for task %d requiring %d GPUs\n", sim.now(), job->id, job->gpu);
            } else {
                for (const auto &pair: run_config) {
                    printf("mid %d -> %d gpu, ", pair.first, pair.second);
                }
                printf("\n");
                job->run(sim, run_config);
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

    std::vector<Job> jobs = std::vector<Job>{Job(1.), Job(4.), Job(9.)};
    Cluster cluster = Cluster();
    cluster.setup(conf);
    PlacementAlgo *placement_algo;
    SchedulingAlgo *scheduling_algo;
    placement_algo = new RandomPlacement();
    scheduling_algo = new FirstComeFirstServed();


    broker(sim, jobs, cluster);
    scheduler(sim, cluster, scheduling_algo, placement_algo);
    sim.run_until(100);
    delete placement_algo;
    delete scheduling_algo;
    return 0;
}
