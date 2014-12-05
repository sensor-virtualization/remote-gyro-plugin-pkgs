cmake_minimum_required(VERSION 2.6)
project(gyro_sim CXX)


ADD_DEFINITIONS("-DTARGET")
MESSAGE("add -DTARGET")


#add_definitions(-D_ESTIMATE_PERFORMANCE)
#add_definitions(-D_DEFAULT_FONT_NAME="Vera")
add_definitions(-Wall -O3 -omit-frame-pointer)
#add_definitions(-Wall -g -D_DEBUG)
add_definitions(-Iinclude)

add_library(${PROJECT_NAME} SHARED
	src/cgyro_sim.cpp
)

target_link_libraries(${PROJECT_NAME} ${rpkgs_LDFLAGS} ${GLES_LDFLAGS})

install(TARGETS ${PROJECT_NAME} DESTINATION lib/sensor_framework)
