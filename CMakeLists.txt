cmake_minimum_required(VERSION 3.19)
project(CloudSimCpp)
set(CMAKE_CXX_STANDARD 20)

set(HTSIM_SRCS)
list(APPEND HTSIM_SRCS
        "NDP/sim/config.cpp"
        "NDP/sim/eventlist.cpp"
        "NDP/sim/network.cpp"
        "NDP/sim/pipe.cpp"
        "NDP/sim/queue.cpp"
        "NDP/sim/route.cpp"
        )

set(SRCS)
list(APPEND SRCS
        "src/common.cpp"
        "src/cluster.cpp"
        "src/CppProgressBar.cpp"
        "src/topology/hierarchical_topology.cpp"
        "src/topology/custom_topology.cpp"
        "src/job.cpp"
        "src/job_scheduler.cpp"
        "src/job_submitter.cpp"
        "src/packet.cpp"
        "src/simplepipe.cpp"
        "src/simplequeue.cpp"
        "src/switch.cpp"
        "src/worker.cpp"
        "src/collective_scheduler.cpp"
        "src/job_placement/random.cpp"
        "src/job_placement/custom.cpp"
        "src/job_placement/yarn.cpp"
        "src/job_scheduling/first_come_first_served.cpp"
        "src/job_scheduling/fit_first.cpp"
        "src/collective_scheduling/first_in_first_out_one_by_one.cpp"
        "src/collective_scheduling/deficit_round_robin.cpp"
        "src/collective_scheduling/ready_and_go.cpp"
        "src/collective_scheduling/bytescheduler.cpp"
        "src/collective_scheduling/sincronia.cpp"
        )

include_directories(src)
include_directories(NDP/sim)
include_directories(NDP/sim/datacenter)
include_directories(simcpp20/include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(glog 0.6.0 REQUIRED)
find_package(fmt REQUIRED)

add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
add_link_options(-fsanitize=address -fno-omit-frame-pointer)

add_subdirectory(simcpp20)
## for profiling
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -fno-omit-frame-pointer")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -fno-omit-frame-pointer")

add_executable(SwitchML switchml_main.cpp ${HTSIM_SRCS} ${SRCS})
add_executable(SwitchML_NOSIMPKT switchml_main.cpp ${HTSIM_SRCS} ${SRCS})
target_compile_definitions(SwitchML_NOSIMPKT PUBLIC NOSIMPKT)
# Threads::Threads for csvio
target_link_libraries(SwitchML PUBLIC glog::glog fmt::fmt fschuetz04_simcpp20 Threads::Threads)
target_link_libraries(SwitchML_NOSIMPKT glog::glog fmt::fmt fschuetz04_simcpp20 Threads::Threads)
