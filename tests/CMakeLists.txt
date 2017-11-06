project(DataManagerTests)
find_package(GTest REQUIRED)

set(BLOCK_MANAGER_LIBRARIES BlockManager)
set(DATA_ARRAY_LIBRARIES DataArray)

include_directories(${Folly_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})

set(BLOCK_MANAGER_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/BlockManager)
set(DATA_ARRAY_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/DataArray)

# BlockManager Tests
add_executable(BlockManagerTestBin BlockManagerTests.cpp)
set_target_properties(BlockManagerTestBin PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testbin/)

target_include_directories(BlockManagerTestBin PRIVATE ${BLOCK_MANAGER_INCLUDE_DIR} ${DATA_ARRAY_INCLUDE_DIR})

target_link_libraries(BlockManagerTestBin ${BLOCK_MANAGER_LIBRARIES} GTest::GTest GTest::Main ${Glog_LIBRARIES} ${Gflags_LIBRARIES} ${Boost_LIBRARIES} ${Folly_LIBRARIES})

add_test(NAME BlockManagerTest COMMAND BlockManagerTestBin)

# DataArray Tests
add_executable(DataArrayTestBin DataArrayTests.cpp)
set_target_properties(DataArrayTestBin PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/testbin/)

target_include_directories(DataArrayTestBin PRIVATE ${DATA_ARRAY_INCLUDE_DIR})

target_link_libraries(DataArrayTestBin ${DATA_ARRAY_LIBRARIES} GTest::GTest GTest::Main ${Glog_LIBRARIES} ${Gflags_LIBRARIES} ${Boost_LIBRARIES} ${Folly_LIBRARIES})

add_test(NAME DataArrayTest COMMAND DataArrayTestBin)
