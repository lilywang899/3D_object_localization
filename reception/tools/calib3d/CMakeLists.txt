cmake_minimum_required(VERSION 3.26)
project(calib3d)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)
# -------------- OpenCV --------------
find_package(OpenCV REQUIRED)
message(STATUS "OpenCV library status:")
message(STATUS "    config: ${OpenCV_DIR}")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

add_executable(calib3d "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

include_directories(
        calib3d
        ${OpenCV_INCLUDE_DIRS}
        /home/gabriel_wang/tools/opencv-4.11.0/modules/calib3d/include
        /home/gabriel_wang/tools/opencv-4.11.0/modules/objdetect/include
)

target_link_libraries(calib3d
        ${OpenCV_LIBS}
        -lpthread
)
set_property(TARGET calib3d PROPERTY CXX_STANDARD 17)




