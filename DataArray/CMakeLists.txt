find_package(Glog REQUIRED)
find_package(TIFF REQUIRED)
find_package(Boost COMPONENTS filesystem system REQUIRED QUIET)

set(DATA_ARRAY_LIBS ${Glog_LIBRARIES} ${Boost_LIBRARIES} ${TIFF_LIBRARIES})
set(DATA_ARRAY_INCLUDE_DIRS ${Glog_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${TIFF_INCLUDE_DIRS})

set(DATA_ARRAY_SOURCES TiffArray.cpp)

if(BLOSC_FOUND)
    list(APPEND DATA_ARRAY_SOURCES BloscArray.cpp)
    list(APPEND DATA_ARRAY_INCLUDE_DIRS "${BLOSC_INCLUDE_DIRS}")
    list(APPEND DATA_ARRAY_LIBS "${BLOSC_LIBRARIES}")
endif()

add_library(DataArray ${DATA_ARRAY_SOURCES})

target_include_directories(DataArray PRIVATE ${DATA_ARRAY_INCLUDE_DIRS})
target_link_libraries(DataArray ${DATA_ARRAY_LIBS})

set_target_properties(DataArray PROPERTIES PUBLIC_HEADER "DataArray.h;BloscArray.h;TiffArray.h")
INSTALL(TARGETS DataArray ARCHIVE DESTINATION ${DATA_MANAGER_LIB_DIR} PUBLIC_HEADER DESTINATION ${DATA_MANAGER_INC_DIR}/DataArray)