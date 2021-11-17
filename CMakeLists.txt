cmake_minimum_required(VERSION 3.19)
project(CloudSimCpp)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(fschuetz04_simcpp20
        GIT_REPOSITORY https://github.com/fschuetz04/simcpp20
        GIT_TAG 160f1fc6c1aad900661adbc27d21b5e209b69c44) # replace with latest revision

FetchContent_MakeAvailable(fschuetz04_simcpp20)


set(HTSIM_SRCS)
list(APPEND HTSIM_SRCS
        #        "htsim/cbr.cpp"
        #        "htsim/cbrpacket.cpp"
        #        "htsim/clock.cpp"
        #        "htsim/compositeprioqueue.cpp"
        #        "htsim/compositequeue.cpp"
        "htsim/config.cpp"
        #        "htsim/cpqueue.cpp"
        #        "htsim/dctcp.cpp"
        #        "htsim/dctcp_transfer.cpp"
        #        "htsim/ecnqueue.cpp"
        #        "htsim/eth_pause_packet.cpp"
        "htsim/eventlist.cpp"
        #        "htsim/exoqueue.cpp"
        #        "htsim/fairpullqueue.cpp"
        "htsim/logfile.cpp"
        "htsim/loggers.cpp"
        #        "htsim/mtcp.cpp"
        #        "htsim/ndp.cpp"
        #        "htsim/ndp_transfer.cpp"
        #        "htsim/ndppacket.cpp"
        "htsim/network.cpp"
        "htsim/parse_output.cpp"
        "htsim/pipe.cpp"
        #        "htsim/prioqueue.cpp"
        #        "htsim/qcn.cpp"
        "htsim/queue.cpp"
        #        "htsim/queue_lossless.cpp"
        #        "htsim/queue_lossless_input.cpp"
        #        "htsim/queue_lossless_output.cpp"
        "htsim/randomqueue.cpp"
        "htsim/route.cpp"
        #        "htsim/sent_packets.cpp"
        "htsim/switch_htsim.cpp"
        "htsim/tcp.cpp"
        "htsim/tcp_periodic.cpp"
        "htsim/tcp_transfer.cpp"
        "htsim/tcppacket.cpp"
        #        "htsim/datacenter/bcube_topology.cpp"
        #        "htsim/datacenter/camcubetopology.cpp"
        #        "htsim/datacenter/connection_matrix.cpp"
        #        "htsim/datacenter/fat_tree_topology.cpp"
        #        "htsim/datacenter/firstfit.cpp"
        #        "htsim/datacenter/incast.cpp"
        #        "htsim/datacenter/mtcp_subflow_control.cpp"
        #        "htsim/datacenter/multihomed_fat_tree_topology.cpp"
        #        "htsim/datacenter/oversubscribed_fat_tree_topology.cpp"
        #        "htsim/datacenter/shortflows.cpp"
        #        "htsim/datacenter/star_topology.cpp"
        #        "htsim/datacenter/vl2_topology.cpp"
        )

set(SRCS)
list(APPEND SRCS
        "src/common.cpp"
        "src/cluster.cpp"
        "src/CppProgressBar.cpp"
        "src/topology/hierarchical_topology.cpp"
        "src/job.cpp"
        "src/job_scheduler.cpp"
        "src/job_submitter.cpp"
        "src/packet.cpp"
        "src/switch.cpp"
        "src/worker.cpp"
        "src/collective_scheduler.cpp"
        "src/job_placement/random.cpp"
        "src/job_placement/yarn.cpp"
        "src/job_scheduling/first_come_first_served.cpp"
        "src/job_scheduling/fit_first.cpp"
        "src/collective_scheduling/first_in_first_out_one_by_one.cpp"
        "src/collective_scheduling/ready_and_go.cpp"
        "src/collective_scheduling/bytescheduler.cpp"
        )

include_directories(htsim)
include_directories(htsim/datacenter)
include_directories(${fschuetz04_simcpp20_SOURCE_DIR}/include)
include_directories(src)

add_library(htsim ${HTSIM_SRCS})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

## for profiling
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -fno-omit-frame-pointer")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -fno-omit-frame-pointer")

#add_executable(CloudSimCpp main.cpp ${SRCS})
add_executable(SwitchML switchml_main.cpp ${SRCS})
#target_link_libraries(CloudSimCpp PUBLIC htsim fschuetz04::simcpp20)
# Threads::Threads for csvio
target_link_libraries(SwitchML PUBLIC htsim fschuetz04::simcpp20 Threads::Threads)