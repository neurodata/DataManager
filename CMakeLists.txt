cmake_minimum_required (VERSION 2.8)
project (DataManager)
set (DataManager_VERSION_MAJOR 0)
set (DataManager_VERSION_MINOR 0)
string(TIMESTAMP DataManager_VERSION_BUILDDATE "%Y%m%d")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

find_package(Glog REQUIRED)
find_package(gflags REQUIRED)
find_package(Folly REQUIRED)
find_package(Boost COMPONENTS filesystem system REQUIRED QUIET )

include_directories(${Folly_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})

configure_file (
    "${CMAKE_SOURCE_DIR}/NeuroDataManager.h.in"
    "${CMAKE_BINARY_DIR}/NeuroDataManager.h"
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# add the binary dir to the search path for include files
# so we find DataManagerHandler.h
include_directories("${CMAKE_BINARY_DIR}")
include_directories("${CMAKE_SOURCE_DIR}/DataManager")

#add_subdirectory(DataManager)
add_subdirectory(BlockManager)
add_subdirectory(DataArray)

#set(DATA_MANAGER_LIBRARIES DataManager)
set(BLOCK_MANAGER_LIBRARIES BlockManager)
set(DATA_ARRAY_LIBRARIES DataArray)

add_executable(ingest Ingest.cpp)

target_link_libraries(ingest ${BLOCK_MANAGER_LIBRARIES} ${DATA_ARRAY_LIBRARIES} ${Glog_LIBRARIES} ${Gflags_LIBRARIES} ${Folly_LIBRARIES} ${Boost_LIBRARIES})
#target_link_libraries(ingest ${DATA_MANAGER_LIBRARIES} ${Glog_LIBRARIES} ${Gflags_LIBRARIES} ${Folly_LIBRARIES} ${Boost_LIBRARIES})
