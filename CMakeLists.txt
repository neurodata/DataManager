cmake_minimum_required (VERSION 2.8)
project (DataManager)
set (DataManager_VERSION_MAJOR 0)
set (DataManager_VERSION_MINOR 3)
set (DataManager_VERSION_PATCH 1)
string(TIMESTAMP DataManager_VERSION_BUILDDATE "%Y%m%d")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

option(USE_STATIC_LIBS "Link against static libraries.")
if(USE_STATIC_LIBS) 
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    
    set(Glog_USE_STATIC_LIBS ON)
    set(Gflags_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_LIBS ON)
    set(Folly_USE_STATIC_LIBS ON)
    
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    endif()
endif()

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

find_package(Glog REQUIRED)
if(USE_STATIC_LIBS)
    list(APPEND Glog_LIBRARIES lzma)
endif()

find_package(Gflags REQUIRED)
list(APPEND Gflags_LIBRARIES pthread)
find_package(ZLIB REQUIRED)

find_package(TIFF REQUIRED) 
if(USE_STATIC_LIBS AND ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    list(APPEND TIFF_LIBRARIES jbig jpeg)
endif()

find_package(Folly REQUIRED)
if(USE_STATIC_LIBS AND ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    list(APPEND Folly_LIBRARIES iberty)
endif()

find_package(Boost COMPONENTS filesystem system iostreams REQUIRED)
if(USE_STATIC_LIBS AND ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    list(APPEND Boost_LIBRARIES rt)
    list(INSERT Boost_LIBRARIES 0 pthread)
endif()

include_directories(${Folly_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})

find_package(BLOSC)
if(BLOSC_FOUND)
    add_definitions("-DHAVE_BLOSC")
else()
    message(STATUS "Failed to find Blosc compression library. Reading Blosc formatted files will not be supported.")
endif()

configure_file (
    "${CMAKE_SOURCE_DIR}/NeuroDataManager.h.in"
    "${CMAKE_BINARY_DIR}/NeuroDataManager.h"
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# add the binary dir to the search path for include files
# so we find DataManagerHandler.h
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("${CMAKE_BINARY_DIR}")
include_directories("${CMAKE_SOURCE_DIR}/DataManager")

add_subdirectory(BlockManager)
add_subdirectory(DataArray)
add_subdirectory(Mesh)
add_subdirectory(third_party/CompressedSegmentation)
add_subdirectory(third_party/NeuroglancerMesh)
add_subdirectory(Skeleton)

if(ENABLE_TESTS) 
    enable_testing()
    add_subdirectory(tests)
endif(ENABLE_TESTS)

set(BLOCK_MANAGER_LIBRARIES BlockManager)
set(DATA_ARRAY_LIBRARIES DataArray)
set(MESH_LIBRARIES MeshLib)

add_executable(ndm NeuroDataManager.cpp)
target_link_libraries(ndm ${BLOCK_MANAGER_LIBRARIES} ${DATA_ARRAY_LIBRARIES} ${Boost_LIBRARIES} ${Folly_LIBRARIES} ${Gflags_LIBRARIES} ${Glog_LIBRARIES} ${ZLIB_LIBRARIES} ${TIFF_LIBRARIES})

set(SKELETON_LIBRARIES Skeleton)

add_executable(skeleton Skeleton.cpp)
target_link_libraries(skeleton ${SKELETON_LIBRARIES} ${Glog_LIBRARIES} ${Gflags_LIBRARIES} ${Boost_LIBRARIES} ${Folly_LIBRARIES} ${ZLIB_LIBRARIES})

set(MESH_LIBRARIES Mesh)

add_executable(mesh Mesh.cpp)
target_link_libraries(mesh ${MESH_LIBRARIES} ${BLOCK_MANAGER_LIBRARIES} ${DATA_ARRAY_LIBRARIES} ${Glog_LIBRARIES} ${Gflags_LIBRARIES} ${Boost_LIBRARIES} ${Folly_LIBRARIES} ${ZLIB_LIBRARIES})
