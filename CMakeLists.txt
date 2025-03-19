cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
if("${IDF_TARGET}" STREQUAL "linux")
    set(COMPONENTS main)
endif()
project(telepat-pill-dispenser)

idf_build_set_property(MINIMAL_BUILD ON)
