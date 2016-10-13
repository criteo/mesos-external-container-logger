# Based on https://github.com/mesosphere/serenity/blob/master/CMakeLists.txt
cmake_minimum_required(VERSION 3.0.0)

IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

project(mesos-external-container-logger
    VERSION 0.1.1
    LANGUAGES CXX C)
enable_testing()

# Command line options.
set(WITH_MESOS "/usr" CACHE STRING "Mesos installation directory")
set(MESOS_INSTALLATION_DIR ${WITH_MESOS})
message("Mesos installation directory set to: " ${MESOS_INSTALLATION_DIR})

option(INTEGRATION_TESTS "Enable compilation of integration tests." OFF)

# Compiler things.
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    message(STATUS "Error: ${CMAKE_CXX_COMPILER} has no C++11 support")
endif()

# Some libs needs to be compiled position indepenedent.
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

# Add gtest.
add_subdirectory (${MESOS_SRC_DIR}/build/3rdparty/gmock-1.7.0)
#include_directories(${gmock_SOURCE_DIR} include)
#include_directories(${gtest_SOURCE_DIR} include)

link_directories(
    ${MESOS_INSTALLATION_DIR}/lib/
    ${MESOS_INSTALLATION_DIR}/lib/mesos/3rdparty/lib)

include_directories(
    ./
    ${MESOS_SRC_DIR}/build/3rdparty/gmock-1.7.0/gtest/include
    ${MESOS_SRC_DIR}/build/3rdparty/gmock-1.7.0/include
    ${MESOS_SRC_DIR}/build/3rdparty/include/zookeeper
    ${MESOS_SRC_DIR}/build/3rdparty/include
    ${MESOS_SRC_DIR}/build/src
    ${MESOS_SRC_DIR}/src
    ${MESOS_INSTALLATION_DIR}/include/
    ${MESOS_INSTALLATION_DIR}/include/mesos
    ${MESOS_INSTALLATION_DIR}/lib/mesos/3rdparty/include
    ${CMAKE_BINARY_DIR}/
    $(CMAKE_INCLUDE_PATH)
)

add_definitions(
    -DSOURCE_DIR="${MESOS_SRC_DIR}/src"
    -DBUILD_DIR="${MESOS_SRC_DIR}/build"
    -DPKGMODULEDIR="nowhere"
    -DLIBDIR="nowhere"
    -DPKGLIBEXECDIR="nowhere"
    -DTESTLIBEXECDIR="nowhere"
    -DSBINDIR="nowhere"
    -DPKGDATADIR="nowhere"
    -DVERSION="1.0.1"
)

set(EXTERNAL_CONTAINER_LOGGER_SOURCES
    src/lib_externallogger.cpp
    src/lib_externallogger.hpp
)

# Module.
add_library(external_container_logger-${PROJECT_VERSION}
    SHARED
    ${EXTERNAL_CONTAINER_LOGGER_SOURCES})
target_link_libraries(external_container_logger-${PROJECT_VERSION})

set(WITH_SOURCE_MESOS "" CACHE STRING "Mesos source directory")
set(MESOS_SOURCE_DIR ${WITH_SOURCE_MESOS})
