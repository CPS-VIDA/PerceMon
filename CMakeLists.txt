cmake_minimum_required(VERSION 3.10.0)

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
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Options
option(PERCEMON_TEST "Build percemon test suite?" OFF)
option(PERCEMON_COVERAGE "Generate coverage.xml for test suite?" OFF)
option(PERCEMON_EXAMPLES "Build the examples?" OFF)
option(PERCEMON_DOCS "Build the docs?" OFF)

set(_PERCEMON_BUILD_THE_TESTS
    OFF
    CACHE INTERNAL "Easy option to build the tests")
if(PERCEMON_MASTER_PROJECT
   AND PERCEMON_TEST
   AND BUILD_TESTING)
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
  find_package(codecov)
endif()

# set(CMAKE_CXX_EXTENSIONS ON)

find_package(fmt REQUIRED)
find_package(cppitertools REQUIRED)

# Sources and actual library Add library and module
set(PERCEMON_SOURCES src/ast.cc src/topo.cc src/monitoring/default_monitor.cc
                     src/monitoring/horizon.cc src/fmt.cc)

add_library(PerceMon ${PERCEMON_SOURCES})
add_library(PerceMon::PerceMon ALIAS PerceMon)
target_include_directories(
  PerceMon PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_include_directories(PerceMon PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_features(PerceMon PUBLIC cxx_std_17)
target_link_libraries(PerceMon PUBLIC fmt::fmt cppitertools::cppitertools)

if(PERCEMON_COVERAGE)
  add_coverage(PerceMon)
endif()

if(_PERCEMON_BUILD_THE_TESTS)
  add_subdirectory(tests)
endif()

if(PERCEMON_DOCS)
  add_subdirectory(doc)
endif()

if(PERCEMON_EXAMPLES)
  add_subdirectory(examples)
endif()

if(PERCEMON_COVERAGE)
  list(APPEND LCOV_REMOVE_PATTERNS
       "'${PROJECT_SOURCE_DIR}/tests/*'" "'${PROJECT_SOURCE_DIR}/examples/*'"
       "'/usr/'")
  coverage_evaluate()
endif()

include(GNUInstallDirs)

# Install catkin package.xml
install(FILES package.xml DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/PerceMon)

install(
  TARGETS PerceMon
  EXPORT PerceMonTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  PerceMonConfigVersion.cmake
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion)

install(
  EXPORT PerceMonTargets
  FILE PerceMonTargets.cmake
  NAMESPACE PerceMon::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PerceMon)

configure_file(cmake/PerceMonConfig.cmake.in PerceMonConfig.cmake @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/PerceMonConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/PerceMonConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PerceMon)
