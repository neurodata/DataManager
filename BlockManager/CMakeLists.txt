find_package(Glog REQUIRED)
find_package(Folly REQUIRED)
find_package(Boost COMPONENTS filesystem system REQUIRED QUIET )
find_package(JPEG REQUIRED)

set(BLOCK_MANAGER_LIBS ${Glog_LIBRARIES} ${Boost_LIBRARIES} ${Folly_LIBRARIES} ${JPEG_LIBRARIES})
set(BLOCK_MANAGER_INCLUDE_DIRS ${CMAKE_SOURCE_DIR} ${Glog_INCLUDE_DIR} ${Folly_INCLUDE_DIRS} ${Boost_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})

set(BLOCK_MANAGER_SOURCES Manifest.cpp BlockManager.cpp Blocks/Block.cpp Blocks/FilesystemBlock.cpp Datastore/FilesystemBlockStore.cpp)

add_library(BlockManager ${BLOCK_MANAGER_SOURCES})

target_include_directories(BlockManager PRIVATE ${BLOCK_MANAGER_INCLUDE_DIRS})
target_link_libraries(BlockManager ${BLOCK_MANAGER_LIBS} compressed_segmentation)
