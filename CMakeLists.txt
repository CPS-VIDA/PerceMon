cmake_minimum_required(VERSION 3.15)

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
option(PERCEMON_COVERAGE "Generate coverage.xml for test suite?" OFF)
option(PERCEMON_EXAMPLES "Build the examples?" OFF)
option(PERCEMON_DOCS "Build the docs?" OFF)
option(BUILD_PYTHON_BINDINGS "Build the percemon Python bindings" ON)

# Configure CMake to perform an optimized release build by default unless
# another build type is specified. Without this addition, binding code may run
# slowly and produce large binaries.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "MinSizeRel" "RelWithDebInfo")
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

find_package(fmt REQUIRED)
find_package(cppitertools REQUIRED)

# Sources and actual library Add library and module
set(PERCEMON_SOURCES
    csrc/ast.cc csrc/topo.cc csrc/monitoring/default_monitor.cc
    csrc/monitoring/horizon.cc csrc/fmt.cc)

add_library(PerceMon ${PERCEMON_SOURCES})
add_library(PerceMon::PerceMon ALIAS PerceMon)
target_include_directories(
  PerceMon PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_include_directories(PerceMon PRIVATE ${PROJECT_SOURCE_DIR}/csrc)
target_compile_features(PerceMon PUBLIC cxx_std_17)
target_link_libraries(PerceMon PUBLIC fmt::fmt cppitertools::cppitertools)
set_target_properties(PerceMon PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(BUILD_PYTHON_BINDINGS)
  if(CMAKE_VERSION VERSION_LESS 3.18)
    set(DEV_MODULE Development)
  else()
    set(DEV_MODULE Development.Module)
  endif()

  # Try to import all Python components potentially needed by nanobind
  find_package(
    Python 3.9 REQUIRED
    COMPONENTS Interpreter ${DEV_MODULE}
    OPTIONAL_COMPONENTS Development.SABIModule)

  # Detect the installed nanobind package and import it into CMake Uses python
  # to pull the path to accomodate pip/conda installs
  execute_process(
    COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE nanobind_ROOT)
  find_package(nanobind CONFIG REQUIRED)

  nanobind_add_module(percemon NB_STATIC STABLE_ABI python/main.cpp)
  target_link_libraries(percemon PUBLIC PerceMon::PerceMon)
endif()

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
  list(APPEND LCOV_REMOVE_PATTERNS "'${PROJECT_SOURCE_DIR}/tests/*'"
       "'${PROJECT_SOURCE_DIR}/examples/*'" "'/usr/'")
  coverage_evaluate()
endif()

install(
  TARGETS PerceMon
  EXPORT PerceMonTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(BUILD_PYTHON_BINDINGS)
  install(TARGETS percemon LIBRARY DESTINATION .)
  nanobind_add_stub(
    percemon_stubs
    MODULE percemon
    OUTPUT percemon.pyi
    PYTHON_PATH $<TARGET_FILE_DIR:percemon>
    DEPENDS percemon
    INSTALL_TIME)
endif()

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
