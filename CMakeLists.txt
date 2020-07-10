cmake_minimum_required(VERSION 3.5.0)
project(
  stql
  VERSION 0.1.0
  DESCRIPTION
    "A library for efficient online monitoring for Spatio-Temporal Quality Logic on streams of perception data."
  LANGUAGES CXX)

# Check if stql is being used directly or via add_subdirectory
set(STQL_MASTER_PROJECT OFF)
if(CMAKE_CURRENT_LIST_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(STQL_MASTER_PROJECT ON)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(CMakePrintHelpers)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

include(ThirdPartyLibs)

if(NOT MSVC)
  add_library(stql SHARED ${STQL_SOURCES})
else()
  # NOTE: This is some weird stuff for MSVC
  add_library(stql STATIC ${STQL_SOURCES})
endif()
add_library(stql::stql ALIAS stql)
target_include_directories(
  stql PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
              $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)
target_compile_features(stql PUBLIC cxx_std_17)
target_link_libraries(stql PRIVATE mono_wedge::mono_wedge)
