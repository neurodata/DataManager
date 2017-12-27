set(NEUROGLANCER_MESH_SOURCES mesh_objects.cc voxel_mesh_generator.cc)

add_library(neuroglancer_mesh STATIC ${NEUROGLANCER_MESH_SOURCES})
INSTALL(TARGETS neuroglancer_mesh ARCHIVE DESTINATION ${DATA_MANAGER_LIB_DIR})