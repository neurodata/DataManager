project(DataManagerTests)
find_package(GTest REQUIRED)

set(BLOCK_MANAGER_LIBRARIES BlockManager)
set(DATA_ARRAY_LIBRARIES DataArray)

include_directories(${Folly_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})

add_executable(BlockManagerTestBin BlockManagerTests.cpp)
set_target_properties(BlockManagerTestBin PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testbin/)

set(BLOCK_MANAGER_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/BlockManager ${CMAKE_SOURCE_DIR}/DataArray)

target_include_directories(BlockManagerTestBin PRIVATE ${BLOCK_MANAGER_INCLUDE_DIRS})

target_link_libraries(BlockManagerTestBin ${BLOCK_MANAGER_LIBRARIES} GTest::GTest GTest::Main ${Glog_LIBRARIES} ${Gflags_LIBRARIES} ${Boost_LIBRARIES} ${Folly_LIBRARIES})

add_test(NAME BlockManagerTest COMMAND BlockManagerTestBin)