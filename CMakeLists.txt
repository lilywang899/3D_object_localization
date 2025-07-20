cmake_minimum_required(VERSION 3.26)
project(perception)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)
# -------------- OpenCV --------------
set(OpenCV_DIR "/private/l753wang/build")
find_package(OpenCV REQUIRED)
message(STATUS "OpenCV library status:")
message(STATUS "    config: ${OpenCV_DIR}")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

# -------------- libtorch --------------
#set(LIBTORCH_DEVICE "cpu")
set(PYTORCH_VERSION "2.6.0")
#set(LIBTORCH_URL "https://download.pytorch.org/libtorch/${LIBTORCH_DEVICE}/libtorch-shared-with-deps-${PYTORCH_VERSION}%2B${LIBTORCH_DEVICE}.zip")
#include(FetchContent)
#FetchContent_Declare(
#        libtorch
#        PREFIX libtorch
#        DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/libtorch
#        SOURCE_DIR ${CMAKE_SOURCE_DIR}/libtorch
#        URL ${LIBTORCH_URL}
#)

#FetchContent_MakeAvailable(libtorch)
list(APPEND CMAKE_PREFIX_PATH "/private/l753wang/libtorch")
set(Torch_DIR "/private/l753wang/libtorch/share/cmake/Torch")

#find_package(Torch REQUIRED PATHS "${CMAKE_SOURCE_DIR}/libtorch")
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
message(STATUS "libtorch status:")
message(STATUS "    version: ${PYTORCH_VERSION}")
message(STATUS "    libraries: ${TORCH_LIBRARIES}")
message(STATUS "    include path: ${TORCH_INCLUDE_DIRS}")


add_executable(perception "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/UdpClient.cpp")

include_directories(
        perception
        ${OpenCV_INCLUDE_DIRS}
        ${TORCH_INCLUDE_DIRS}
#        ${CMAKE_CURRENT_SOURCE_DIR}/libtorch/include
#        ${CMAKE_CURRENT_SOURCE_DIR}/libtorch/include/torch/csrc/api/include
)

target_link_libraries(perception
   ${TORCH_LIBRARIES}
   ${OpenCV_LIBS}
   -lpthread
)
set_property(TARGET perception PROPERTY CXX_STANDARD 17)

add_executable(udpserver "${CMAKE_CURRENT_SOURCE_DIR}/tools/UdpServer.cpp")


