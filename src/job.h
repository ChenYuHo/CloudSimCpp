#ifndef CLOUDSIMCPP_JOB_H
#define CLOUDSIMCPP_JOB_H

class Job {
private:
    static unsigned get_id() {
        static unsigned ID = 0;
        return ID++;
    }

public:
    unsigned id{};
    unsigned gpu{8};
    simtime_picosec submit_time{};
    simtime_picosec submitted_time{};
    simtime_picosec start_time{(std::numeric_limits<simtime_picosec>::max)()};
    simtime_picosec finish_time{(std::numeric_limits<simtime_picosec>::max)()};
    unsigned n_iter{3};
    int in_iter{-1};
    int fp_layer{-1};
    unsigned num_workers_allocated{};
    std::unordered_set<unsigned> wids_allocated{};
    unsigned tensor_id{0};
    unsigned master_mid{};
    std::vector<uint64_t> model{};
    std::vector<uint64_t> forward_pass_time{};
    std::vector<uint64_t> backward_pass_time{};
    explicit Job(simtime_picosec t, simcpp20::simulation<SIM_UNIT> &sim) : submit_time(t), id(get_id()) {
//        myprintf("Job %d constructor invoked\n", this->id);
    }

    explicit Job(simtime_picosec t, simcpp20::simulation<SIM_UNIT> &sim,
                 std::vector<uint64_t> model) : submit_time(
            t), id(get_id()), model(std::move(model)) {
//        myprintf("Job %d constructor invoked\n", this->id);
    }

    Job(simtime_picosec t, simcpp20::simulation<SIM_UNIT> &sim,
        std::vector<uint64_t> model, unsigned iter, unsigned gpu) : gpu(gpu), submit_time(t), id(get_id()),
            n_iter(iter), model(std::move(model)) {}

    Job(simtime_picosec t, simcpp20::simulation<SIM_UNIT> &sim,
        const std::string &m, unsigned iter, unsigned gpu) : gpu(gpu), submit_time(t), id(get_id()),
                                                             n_iter(iter) {
        // units are in number of elements, picoseconds
        if (m == "alexnet") {
            model = vector<uint64_t>{330688, 39891840, 16781312, 4097000};
            forward_pass_time = vector<uint64_t>{7142476500, 5935371500, 538843000, 290646000};
            backward_pass_time = vector<uint64_t>{7357764500, 8508749000, 866789500, 724873500};
        } else if (m == "vgg11") {
            model = vector<uint64_t>{370816, 4130048, 4719616, 102764544, 16781312, 4097000};
            forward_pass_time = vector<uint64_t>{40477420000, 29265161500, 6909629500, 4139546000, 563354500,
                                                 312222000};
            backward_pass_time = vector<uint64_t>{91795457000, 41330455000, 9609861500, 4113232000, 804690000,
                                                  714073000};
        } else if (m == "vgg16") {
            model = vector<uint64_t>{555328, 4720128, 4719616, 4719616, 102764544, 16781312, 4097000};
            forward_pass_time = vector<uint64_t>{85248979500, 41601837000, 13573160000, 6260709000, 4176132500,
                                                 559287500, 310557500};
            backward_pass_time = vector<uint64_t>{200699272500, 72797470500, 14607458000, 9612274000, 4112640000,
                                                  803203500, 739359500};
        } else if (m == "vgg19") {
            model = vector<uint64_t>{555328, 2950400, 4719616, 4719616, 4719616, 105124352, 16781312, 4097000};
            forward_pass_time = vector<uint64_t>{85205857500, 44195596500, 19585976500, 13565070000, 6254850000,
                                                 7296838000, 557258000, 309592500};
            backward_pass_time = vector<uint64_t>{200719184000, 76890454500, 35900909000, 14605192500, 12151102000,
                                                  7613635000, 803670500, 745728000};
        } else if (m == "inception" || m == "inception3" || m == "inception4") {
            model = vector<uint64_t>{271200, 2735232, 2792064, 2760704, 3006720, 3009664, 3018752, 2910592, 3329640};
            forward_pass_time = vector<uint64_t>{23529024000, 31928948500, 19309928500, 18639883500, 9524648500,
                                                 3121136500, 3998568000, 2683446500, 2604691500};
            backward_pass_time = vector<uint64_t>{50212443500, 56623332500, 31101889500, 29526303000, 19703666000,
                                                  4050867500, 5616823000, 3611453000, 4477577500};
        } else if (m == "googlenet") {
            model = vector<uint64_t>{266368, 2884992, 3473544};
            forward_pass_time = vector<uint64_t>{17054368500, 30183031500, 9014578500};
            backward_pass_time = vector<uint64_t>{34878800500, 58395501000, 14486796000};
        } else if (m == "resnet101") {
            model = vector<uint64_t>{405824, 2813696, 2824704, 2761216, 2824704, 2761216, 2824704, 2761216, 2824704,
                                     2761216, 4856832, 3150848, 3412992, 4462592, 3102696};
            forward_pass_time = vector<uint64_t>{38127371000, 31704019500, 9623361000, 11447747000, 9601884500,
                                                 11461110000, 9806017000, 11437089000, 9599704500, 11437225500,
                                                 11761981500, 3424134000, 2616983000, 3807266000, 2106830000};
            backward_pass_time = vector<uint64_t>{77140709000, 58692431000, 17412003000, 20695431500, 17409622000,
                                                  20667658500, 17403490000, 20671864500, 17406267500, 20677462500,
                                                  22602687500, 7796365500, 4669965500, 7004016000, 4170328000};
        } else if (m == "resnet152") {
            model = vector<uint64_t>{405824, 2880768, 2760704, 2761216, 2824704, 2761216, 2824704, 2761216, 2824704,
                                     2761216, 2824704, 2761216, 2824704, 2761216, 2824704, 2761216, 3739648, 3150848,
                                     3412992, 4462592, 3102696};
            forward_pass_time = vector<uint64_t>{37579590500, 48092134000, 10615202000, 11451695000, 9612745000,
                                                 11636295500, 9600159500, 11440254500, 9602236000, 11443515500,
                                                 9589970000, 11433956500, 9604805500, 11499351500, 9604383000,
                                                 11437671500, 7543339500, 3424651500, 2621402000, 3801351500,
                                                 2106359500};
            backward_pass_time = vector<uint64_t>{77066405000, 85837029000, 20191510000, 20656653500, 17410677500,
                                                  20670532500, 17410306500, 20670901500, 17404841500, 20667305000,
                                                  17399417000, 20666845500, 17407706500, 20663251000, 17413553500,
                                                  20669265000, 14961066500, 7795622500, 4682113000, 6993681000,
                                                  4446505000};
        } else { // resnet50
            model = vector<uint64_t>{405824, 2813696, 2824704, 3023360, 3409920, 3153920, 3409920, 3412992, 3102696};
            forward_pass_time = vector<uint64_t>{39000956500, 31620431500, 9609898000, 12353136500, 3670556000,
                                                 3646031000, 2373377500, 2602190500, 2103317000};
            backward_pass_time = vector<uint64_t>{77083596500, 58639153000, 17409537500, 22426742000, 7865404000,
                                                  8232344500, 4218669000, 4709677000, 3862900500};
        }
    }
};
#endif //CLOUDSIMCPP_JOB_H
