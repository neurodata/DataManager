find_package(Glog REQUIRED)
find_package(Boost COMPONENTS filesystem system REQUIRED QUIET)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

set(MESH_LIBS ${Glog_LIBRARIES} ${Boost_LIBRARIES})
set(MESH_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include ${Glog_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})

set(MESH_SOURCES MeshExtractor.cpp TriangleMeshIO.cpp ExtractNeuroglancerMesh.cpp)

add_library(Mesh ${MESH_SOURCES})

target_include_directories(Mesh PRIVATE ${MESH_INCLUDE_DIRS})
target_link_libraries(Mesh ${MESH_LIBS} neuroglancer_mesh)

set_target_properties(Mesh PROPERTIES PUBLIC_HEADER "ExtractNeuroglancerMesh.h;MeshExtractor.h;TriangleMeshIO.h")
INSTALL(TARGETS Mesh ARCHIVE DESTINATION ${DATA_MANAGER_LIB_DIR} PUBLIC_HEADER DESTINATION ${DATA_MANAGER_INC_DIR}/Mesh)