cmake_minimum_required(VERSION 3.8)
find_package(realsense2 REQUIRED)

# Enable C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs  -fPIC -g -O0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread  -Wno-unused-local-typedefs  -fPIC -g -O0")

project(RealsenseDaemon)

add_executable(rs-daemon rs-daemon.cpp)
target_link_libraries(
        rs-daemon	
        ${realsense2_LIBRARY})

add_executable(udp_client tools/udp_client.cpp)
