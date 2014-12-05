cmake_minimum_required(VERSION 2.6)
project(gyro_sim_pkgs CXX)

# to install pkgconfig setup file.
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 1.0)

include(FindPkgConfig)
pkg_check_modules(rpkgs REQUIRED vconf sf_common)
add_definitions(${rpkgs_CFLAGS})

set(PROJECT_MAJOR_VERSION "0")
set(PROJECT_MINOR_VERSION "2")
set(PROJECT_RELEASE_VERSION "1")
set(CMAKE_VERBOSE_MAKEFILE OFF)

add_subdirectory(sensor)
add_subdirectory(processor)
