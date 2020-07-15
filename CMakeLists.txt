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

# Options
option(SIGNALTL_TEST "Build signaltl test suite?" ${SIGNALTL_MASTER_PROJECT})
option(SIGNALTL_EXAMPLES "Build the examples?" ${SIGNALTL_MASTER_PROJECT})
option(SIGNALTL_INSTALL "Install the library and header files?"
       ${SIGNALTL_MASTER_PROJECT})
option(SIGNALTL_PYTHON_BINDINGS "Build the Python extension?" ON)

set(_SIGNALTL_BUILD_THE_TESTS
    OFF
    CACHE INTERNAL "Easy option to build the tests")
if((SIGNALTL_MASTER_PROJECT OR SIGNALTL_TEST) AND BUILD_TESTING)
  set(_SIGNALTL_BUILD_THE_TESTS
      ON
      CACHE INTERNAL "Easy option to build the tests")
endif()

set(CMAKE_CXX_EXTENSIONS ON)

# Third party libraries and dependencies
include(ThirdPartyLibs)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party)

# Add library and module
set(STQL_SOURCES src/ast.cc)

add_library(stql ${STQL_SOURCES})
add_library(stql::stql ALIAS stql)
target_include_directories(
  stql PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
              $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)
target_compile_features(stql PUBLIC cxx_std_17)
target_link_libraries(stql PRIVATE mono_wedge::mono_wedge)
