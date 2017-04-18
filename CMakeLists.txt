cmake_minimum_required(VERSION 2.8)
project(teldqm)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(LCIO REQUIRED)
find_package(EUTelescope REQUIRED)
find_package(ROOT REQUIRED)
find_package(Boost REQUIRED program_options)

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

include_directories(${LCIO_INCLUDE_DIRS} ${ROOT_INCLUDE_DIRS} ${EUTelescope_INCLUDE_DIR})
link_directories(${ROOT_LIBRARY_PATH})
add_executable(datamerger main.cpp datastructures.cpp johannesexporter.cpp ${CMAKE_BINARY_DIR}/dict.cpp)
add_executable(correlator correlator.cpp datastructures.cpp ${CMAKE_BINARY_DIR}/dict.cpp)
add_executable(shifter shifter.cpp datastructures.cpp ${CMAKE_BINARY_DIR}/dict.cpp)
#add_executable(datamerger main.cpp exporter.cpp)
#add_executable(correlator correlator.cpp)
target_link_libraries(datamerger ${LCIO_LIBRARIES} ${Boost_LIBRARIES} ${ROOT_LIBRARIES} ${EUTelescope_LIBRARIES})
target_link_libraries(correlator ${ROOT_LIBRARIES})
target_link_libraries(shifter ${ROOT_LIBRARIES})

install(TARGETS datamerger correlator shifter
	RUNTIME DESTINATION bin
	)
install(FILES ${SCRIPT_FILES} DESTINATION bin
	PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)