#ifndef CLOUDSIMCPP_JOB_H
#define CLOUDSIMCPP_JOB_H
#include <unordered_set>

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
    std::vector<uint64_t> weight_update_time{};
    explicit Job(simtime_picosec t, simcpp20::simulation<SIM_UNIT> &sim) : submit_time(t), id(get_id()) {}

    explicit Job(simtime_picosec t, simcpp20::simulation<SIM_UNIT> &sim,
                 std::vector<uint64_t> model) : submit_time(
            t), id(get_id()), model(std::move(model)) {}

    Job(simtime_picosec t, simcpp20::simulation<SIM_UNIT> &sim,
        std::vector<uint64_t> model, unsigned iter, unsigned gpu) : gpu(gpu), submit_time(t), id(get_id()),
            n_iter(iter), model(std::move(model)) {}

    Job(simtime_picosec t, simcpp20::simulation<SIM_UNIT> &sim,
        const std::string &m, unsigned iter, unsigned gpu) : gpu(gpu), submit_time(t), id(get_id()),
                                                             n_iter(iter) {
        // units are in number of elements, picoseconds. All applied PyTorch's default 25MB bucketing
        if (m == "alexnet") {
            model = vector<uint64_t>{330688, 39891840, 16781312, 4097000};
            forward_pass_time = vector<uint64_t>{6487422000, 5547996500, 482768500, 254498000};
            backward_pass_time = vector<uint64_t>{6780418500, 7897807000, 791112000, 538649500};
            weight_update_time = vector<uint64_t>{1167345, 140820177, 59238865, 14462613};
        } else if (m == "vgg11") {
            model = vector<uint64_t>{370816, 8849664, 102764544, 16781312, 4097000};
            forward_pass_time = vector<uint64_t>{45970975500, 36164820500, 4127456500, 485374000, 287803500};
            backward_pass_time = vector<uint64_t>{91740694000, 50867676500, 4109033500, 795572000, 538523000};
            weight_update_time = vector<uint64_t>{724217, 17283720, 200702948, 32774522, 8001592};
        } else if (m == "vgg16") {
            model = vector<uint64_t>{555328, 7079936, 7079424, 102764544, 16781312, 4097000};
            forward_pass_time = vector<uint64_t>{90816362000, 51354924000, 10024820000, 4156633000, 478786000, 287318500};
            backward_pass_time = vector<uint64_t>{200656557000, 81372176000, 15606738500, 4108847000, 796019000, 550236500};
            weight_update_time = vector<uint64_t>{1287480, 16414219, 16413032, 238250708, 38906021, 9498540};
        } else if (m == "vgg19") {
            model = vector<uint64_t>{555328, 7670016, 7079424, 107484160, 16781312, 4097000};
            forward_pass_time = vector<uint64_t>{90902865500, 63746658500, 16688510000, 10410244000, 484357500, 293821000};
            backward_pass_time = vector<uint64_t>{200714238500, 112808855000, 20643356500, 13684955500, 799120000, 568673000};
            weight_update_time = vector<uint64_t>{1386074, 19144017, 17669925, 268275652, 41885404, 10225929};
        } else if (m == "inception" || m == "inception3" || m == "inception4") { // trace is inceptionv3
            model = vector<uint64_t>{271200, 6700160, 7604224, 6815616, 2443368};
            forward_pass_time = vector<uint64_t>{39062640000, 69619003000, 23538588500, 10326569500, 2301948000};
            backward_pass_time = vector<uint64_t>{91082534500, 157184949500, 51105377500, 18621429000, 4108938000};
            weight_update_time = vector<uint64_t>{23843606, 589070697, 668555009, 599221461, 214818227};
        } else if (m == "googlenet") {
            model = vector<uint64_t>{266368, 6358536};
            forward_pass_time = vector<uint64_t>{14777889000, 38211556500};
            backward_pass_time = vector<uint64_t>{34875528000, 72078958000};
            weight_update_time = vector<uint64_t>{53534230, 1277928770};
        } else if (m == "resnet101") {
            model = vector<uint64_t>{405824, 6755584, 6703104, 6703104, 6703104, 7352832, 6822912, 3102696};
            forward_pass_time = vector<uint64_t>{36415028000, 45010404000, 24932418500, 25036425000, 24930242000, 11107929000, 4911117500, 2078128500};
            backward_pass_time = vector<uint64_t>{77078881500, 83454325500, 45548697000, 45538862500, 45566164000, 23223537000, 8892756000, 3708969500};
            weight_update_time = vector<uint64_t>{20383371, 339313527, 336677608, 336677608, 336677608, 369311574, 342695219, 155839484};
        } else if (m == "resnet152") {
            model = vector<uint64_t>{405824, 6758656, 6703104, 6703104, 6703104, 6703104, 6703104, 8534528, 7875584, 3102696};
            forward_pass_time = vector<uint64_t>{36436733000, 62309174000, 25085074500, 24963430500, 24960930000, 24952071500, 25011073000, 18011680500, 6339820500, 2082999000};
            backward_pass_time = vector<uint64_t>{77132477500, 113671785500, 45698397000, 45704558000, 45694686000, 45699517000, 45701750000, 35786120000, 11665839000, 3928979500};
            weight_update_time = vector<uint64_t>{21999307, 366379879, 363368461, 363368461, 363368461, 363368461, 363368461, 462648097, 426927412, 168194000};
        } else if (m == "bert") {
            model = vector<uint64_t>{31260672, 8927232, 7346176, 9445376, 8400896, 7346176, 9445376, 8400896, 7346176, 9445376, 8400896, 7346176, 9445376, 8400896, 7346176, 9445376, 8400896, 7346176, 9445376, 8400896, 7346176, 9445376, 8400896, 7346176, 9445376, 8400896, 7346176, 9445376, 8400896, 7346176, 9445376, 8400896, 7346176, 9445376, 8400896, 7346176, 9445376, 1053698};
            forward_pass_time = vector<uint64_t>{201679500, 1695811500, 613803500, 977431500, 1360976000, 592250500, 1002414000, 1333952500, 602293500, 966248500, 1338686000, 587513500, 999311000, 1333727000, 601031500, 968510000, 1340979000, 586090500, 987556500, 1495193000, 605150500, 972399500, 1354451000, 588239500, 993769500, 1332182000, 611251500, 984363000, 1368996000, 602952000, 996895000, 1357962500, 615189500, 989255000, 1370287500, 601601500, 999744500, 411394500};
            backward_pass_time = vector<uint64_t>{416228000, 229544768000, 6106568500, 8326861000, 14241184000, 6104166500, 8338906500, 14249240000, 6113811000, 8326589000, 14238364500, 6109754000, 8344041000, 14020027500, 2887953000, 7535392000, 15202323000, 10084485000, 6085473500, 15277926000, 10172296000, 7242516000, 15409178500, 8956063000, 7463055500, 15079349000, 9197268000, 7383737500, 1604860500, 810691500, 512126500, 1188866500, 809201000, 512058000, 1190091000, 825784000, 534169000, 4128649500};
            weight_update_time = vector<uint64_t>{17613563287, 5029973950, 4139141215, 5321917838, 4733414348, 4139141215, 5321917838, 4733414348, 4139141215, 5321917838, 4733414348, 4139141215, 5321917838, 4733414348, 4139141215, 5321917838, 4733414348, 4139141215, 5321917838, 4733414348, 4139141215, 5321917838, 4733414348, 4139141215, 5321917838, 4733414348, 4139141215, 5321917838, 4733414348, 4139141215, 5321917838, 4733414348, 4139141215, 5321917838, 4733414348, 4139141215, 5321917838, 593697295};
        } else { // resnet50
            model = vector<uint64_t>{405824, 6755584, 7417344, 7875584, 3102696};
            forward_pass_time = vector<uint64_t>{36421561000, 45085059500, 13925746500, 6352587000, 2089498000};
            backward_pass_time = vector<uint64_t>{77064011000, 83514474000, 28129353500, 11618227000, 3549431000};
            weight_update_time = vector<uint64_t>{20061435, 333954398, 366667730, 389320288, 153378150};
        }
    }
};
#endif //CLOUDSIMCPP_JOB_H
