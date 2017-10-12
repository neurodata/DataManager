find_package(Glog REQUIRED)
find_package(Folly REQUIRED)
find_package(Boost COMPONENTS filesystem system REQUIRED QUIET )

set(SKELETON_LIBS ${Glog_LIBRARIES} ${Boost_LIBRARIES} ${Folly_LIBRARIES})
set(SKELETON_INCLUDE_DIRS ${CMAKE_SOURCE_DIR} ${Glog_INCLUDE_DIR} ${Folly_INCLUDE_DIRS} ${Boost_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})

set(SKELETON_SOURCES Skeleton.cpp SkeletonBuilder.cpp)

add_library(Skeleton ${SKELETON_SOURCES})

target_include_directories(Skeleton PRIVATE ${SKELETON_INCLUDE_DIRS})
target_link_libraries(Skeleton ${SKELETON_LIBS})
