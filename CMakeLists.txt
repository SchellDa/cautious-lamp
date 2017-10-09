cmake_minimum_required(VERSION 2.8)
project(teldqm)

set(CMAKE_MODULE_PATH "$ENV{ROOTSYS}/etc/cmake" ${CMAKE_MODULE_PATH}  "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(ROOT REQUIRED)
find_package(LCIO REQUIRED)
find_package(EUTelescope REQUIRED)
find_package(Boost REQUIRED program_options)
find_package(Eigen3 REQUIRED)
find_package(Marlin 1.0 REQUIRED) # minimum required Marlin version

get_cmake_property(_variableNames VARIABLES)
foreach (_variableName ${_variableNames})
	    message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11 -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pedantic")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#if(DEFINED ENV{TB_LCIO})
#       set(LCIO_DATA_PATH $ENV{TB_LCIO})
#else()
#       message(WARNING "No default testbeam data path set via TB_LCIO environment variable")
#endif()
#
#configure_file(configure.in configure.h)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/dict.cpp ${CMAKE_BINARY_DIR}/dict.h
  COMMAND rootcint -f ${CMAKE_BINARY_DIR}/dict.cpp -c ${CMAKE_SOURCE_DIR}/datastructures.h ${CMAKE_SOURCE_DIR}/LinkDef.h
  DEPENDS ${CMAKE_SOURCE_DIR}/datastructures.h ${CMAKE_SOURCE_DIR}/LinkDef.h
)
# Find launcher Bash-Scripts
file(GLOB SCRIPT_FILES ${PROJECT_SOURCE_DIR}/scripts/*.sh)

include_directories(${LCIO_INCLUDE_DIRS} ${EUTELESCOPE_INCLUDE_DIRS})
include_directories(${ROOT_INCLUDE_DIRS} ${ROOT_INCLUDE_DIR})
include_directories(${EIGEN3_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIR})
include_directories(${Marlin_INCLUDE_DIRS})

link_directories(${ROOT_LIBRARY_PATH})
add_definitions(${Marlin_DEFINITIONS})
#add_executable(datamerger main.cpp datastructures.cpp johannesexporter.cpp ${CMAKE_BINARY_DIR}/dict.cpp)
add_executable(correlator correlator.cpp datastructures.cpp ${CMAKE_BINARY_DIR}/dict.cpp)
add_executable(shifter shifter.cpp datastructures.cpp ${CMAKE_BINARY_DIR}/dict.cpp)
add_executable(dts detailedtimeshifter.cpp datastructures.cpp ${CMAKE_BINARY_DIR}/dict.cpp)
add_library(MapsaDataMerger SHARED mapsadatamerger.cpp datastructures.cpp ${CMAKE_BINARY_DIR}/dict.cpp)
add_library(AlibavaDataMerger SHARED alibavadatamerger.cpp datastructures.cpp ${CMAKE_BINARY_DIR}/dict.cpp)
#add_executable(datamerger main.cpp exporter.cpp)
#add_executable(correlator correlator.cpp)
#target_link_libraries(datamerger ${LCIO_LIBRARIES} ${Boost_LIBRARIES} ${ROOT_LIBRARIES} ${EUTelescope_LIBRARIES})
target_link_libraries(correlator ${ROOT_LIBRARIES})
target_link_libraries(shifter ${ROOT_LIBRARIES})
target_link_libraries(dts ${ROOT_LIBRARIES})
target_link_libraries(MapsaDataMerger ${Marlin_LIBRARIES} ${LCIO_LIBRARIES} ${EUTelescope_LIBRARIES} ${ROOT_LIBRARIES})
target_link_libraries(AlibavaDataMerger ${Marlin_LIBRARIES} ${LCIO_LIBRARIES} ${EUTelescope_LIBRARIES} ${ROOT_LIBRARIES})


install(TARGETS correlator shifter dts 
	RUNTIME DESTINATION bin
	)
install(FILES ${SCRIPT_FILES} DESTINATION bin
	PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
