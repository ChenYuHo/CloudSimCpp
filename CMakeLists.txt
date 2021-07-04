cmake_minimum_required(VERSION 3.14)

project(CloudSimCpp)

include(FetchContent)

FetchContent_Declare(fschuetz04_simcpp20
    GIT_REPOSITORY https://github.com/fschuetz04/simcpp20
    GIT_TAG        160f1fc6c1aad900661adbc27d21b5e209b69c44) # replace with latest revision

FetchContent_MakeAvailable(fschuetz04_simcpp20)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE fschuetz04::simcpp20)
