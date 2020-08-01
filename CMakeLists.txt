cmake_minimum_required(VERSION 3.5.0)

file(
  STRINGS ${CMAKE_CURRENT_LIST_DIR}/VERSION PERCEMON_VERSION
  LIMIT_COUNT 1
  REGEX
    "^(0|[1-9][0-9]*)[.](0|[1-9][0-9]*)[.](0|[1-9][0-9]*)(-[.0-9A-Za-z-]+)?([+][.0-9A-Za-z-]+)?$"
)

project(
  PerceMon
  VERSION ${PERCEMON_VERSION}
  DESCRIPTION
    "A library for efficient online monitoring for Spatio-Temporal Quality Logic on streams of perception data."
  LANGUAGES CXX)

# Check if percemon is being used directly or via add_subdirectory
set(PERCEMON_MASTER_PROJECT OFF)
if(CMAKE_CURRENT_LIST_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(PERCEMON_MASTER_PROJECT ON)
endif()

include(CMakePrintHelpers)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CTest)

# Options
option(PERCEMON_TEST "Build percemon test suite?" ${PERCEMON_MASTER_PROJECT})
option(PERCEMON_COVERAGE "Generate coverage.xml for test suite?"
       ${PERCEMON_MASTER_PROJECT})
option(PERCEMON_EXAMPLES "Build the examples?" ${PERCEMON_MASTER_PROJECT})
option(PERCEMON_INSTALL "Install the library and header files?"
       ${PERCEMON_MASTER_PROJECT})

set(_PERCEMON_BUILD_THE_TESTS
    OFF
    CACHE INTERNAL "Easy option to build the tests")
if((PERCEMON_MASTER_PROJECT OR PERCEMON_TEST) AND BUILD_TESTING)
  set(_PERCEMON_BUILD_THE_TESTS
      ON
      CACHE INTERNAL "Easy option to build the tests")
endif()

if(PERCEMON_MASTER_PROJECT)
  # Force color in compiler output as it will be easier to debug...
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # using Clang
    add_compile_options(-fcolor-diagnostics)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
    add_compile_options(-fdiagnostics-color=always)
  endif()
endif()

if(PERCEMON_COVERAGE)
  set(ENABLE_COVERAGE
      ON
      CACHE BOOL "Enable coverage build." FORCE)
endif()

set(CMAKE_CXX_EXTENSIONS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Third party libraries and dependencies
include(ThirdPartyLibs)
add_subdirectory(third_party)

# Sources and actual library Add library and module
set(PERCEMON_SOURCES src/ast.cc src/monitoring/default_monitor.cc
                     src/monitoring/horizon.cc)

add_library(PerceMon ${PERCEMON_SOURCES})
add_library(PerceMon::PerceMon ALIAS PerceMon)
target_include_directories(
  PerceMon PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_include_directories(PerceMon PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_features(PerceMon PUBLIC cxx_std_17)
target_link_libraries(PerceMon PRIVATE mono_wedge::mono_wedge)
target_link_libraries(PerceMon PUBLIC fmt::fmt cppitertools::cppitertools)
if(MSVC)
  target_compile_options(PerceMon PRIVATE /W4 /WX /utf-8)
else()
  target_compile_options(PerceMon PRIVATE -pedantic -Wall -Wextra -Werror)
endif()

# if(PERCEMON_COVERAGE) add_coverage(PerceMon) endif()

if(_PERCEMON_BUILD_THE_TESTS)
  add_subdirectory(tests)
endif()

if(PERCEMON_EXAMPLES)
  add_subdirectory(examples)
endif()

if(PERCEMON_COVERAGE)
  coverage_evaluate()
endif()
