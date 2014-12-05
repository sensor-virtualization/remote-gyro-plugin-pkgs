cmake_minimum_required(VERSION 2.6)
project(gyro_sim_processor CXX)

add_definitions(-DUSE_ONLY_ONE_MODULE)

ADD_DEFINITIONS("-DTARGET")
MESSAGE("add -DTARGET")

add_definitions(-Wall -O3 -omit-frame-pointer)
#add_definitions(-Wall -g -D_DEBUG)
add_definitions(-Iinclude)

add_library(${PROJECT_NAME} SHARED
	src/gyro_sim_processor.cpp
)

target_link_libraries(${PROJECT_NAME} ${rpkgs_LDFLAGS} ${GLES_LDFLAGS} "-lm")

install(TARGETS ${PROJECT_NAME} DESTINATION lib/sensor_framework)
