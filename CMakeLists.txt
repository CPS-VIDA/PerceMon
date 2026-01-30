cmake_minimum_required(VERSION 3.29)

project(
  PerceMon
  VERSION 1.0.3
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
# Setup sccache if available
if(DEFINED ENV{SCCACHE_PATH})
  if(IS_EXECUTABLE "$ENV{SCCACHE_PATH}")
    set(SCCACHE_PROGRAM
        "$ENV{SCCACHE_PATH}"
        CACHE STRING "use provided sccache" FORCE)
  endif()
else()
  find_program(SCCACHE_PROGRAM sccache PATHS ENV SCCACHE_PATH)
endif()
if(SCCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
  message(STATUS "sccache found: ${SCCACHE_PROGRAM}")
else()
  message(STATUS "sccache not found")
endif()

# Force building of python bindings if using scikit-build
if(SKBUILD)
  set(BUILD_PYTHON_BINDINGS
      ON
      CACHE BOOL "Force build Python bindings" FORCE)
endif()
# Disable testing if we are running cibuildwheel
if(DEFINED ENV{CIBUILDWHEEL})
  set(BUILD_TESTING
      OFF
      CACHE BOOL "Force disable testing in cibuildwheel" FORCE)
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

add_library(
  libPerceMon
  csrc/datastream.cc csrc/monitoring/evaluation.cc
  csrc/monitoring/online_monitor.cc csrc/spatial.cc csrc/stql_requirements.cc)
add_library(libPerceMon::PerceMon ALIAS libPerceMon)
target_include_directories(
  libPerceMon PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                     $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_include_directories(libPerceMon PRIVATE ${PROJECT_SOURCE_DIR}/csrc)
target_compile_features(libPerceMon PUBLIC cxx_std_20)
set_target_properties(libPerceMon PROPERTIES POSITION_INDEPENDENT_CODE ON
                                             OUTPUT_NAME "PerceMon")

if(PERCEMON_MASTER_PROJECT)
  if(MSVC)
    target_compile_options(libPerceMon PRIVATE /W4 /WX /utf-8)
  else()
    target_compile_options(libPerceMon PRIVATE -pedantic -Wall -Wextra -Werror)
  endif()
endif()

if(BUILD_PYTHON_BINDINGS)
  if(CMAKE_VERSION VERSION_LESS 3.18)
    set(DEV_MODULE Development)
  else()
    set(DEV_MODULE Development.Module)
  endif()

  # Try to import all Python components potentially needed by nanobind
  find_package(
    Python 3.10 REQUIRED
    COMPONENTS Interpreter ${DEV_MODULE}
    OPTIONAL_COMPONENTS Development.SABIModule)

  # Detect the installed nanobind package and import it into CMake Uses python
  # to pull the path to accomodate pip/conda installs
  execute_process(
    COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE nanobind_ROOT)
  find_package(nanobind CONFIG REQUIRED)

  nanobind_add_module(
    percemon
    NB_STATIC
    STABLE_ABI
    python/percemon/bindings/datastream_bindings.cpp
    python/percemon/bindings/evaluation_bindings.cpp
    python/percemon/bindings/monitoring_bindings.cpp
    python/percemon/bindings/online_monitor_bindings.cpp
    python/percemon/bindings/spatial_bindings.cpp
    python/percemon/bindings/stql_bindings.cpp
    python/percemon/main.cpp)
  target_link_libraries(percemon PUBLIC libPerceMon::PerceMon)
endif()

if(PERCEMON_COVERAGE)
  add_coverage(libPerceMon)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

if(PERCEMON_DOCS)
  add_subdirectory(doc)
endif()

if(PERCEMON_COVERAGE)
  list(APPEND LCOV_REMOVE_PATTERNS "'${PROJECT_SOURCE_DIR}/tests/*'"
       "'${PROJECT_SOURCE_DIR}/examples/*'" "'/usr/'")
  coverage_evaluate()
endif()

install(
  TARGETS libPerceMon
  EXPORT PerceMonTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(BUILD_PYTHON_BINDINGS)
  install(TARGETS percemon LIBRARY DESTINATION .)
  if(SKBUILD)
    install(FILES python/percemon/__init__.pyi python/percemon/py.typed
            DESTINATION ${SKBUILD_PLATLIB_DIR}/percemon)
  endif()
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  PerceMonConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(
  EXPORT PerceMonTargets
  FILE PerceMonTargets.cmake
  NAMESPACE PerceMon::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PerceMon)

configure_package_config_file(
  cmake/PerceMonConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/PerceMonConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PerceMon)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/PerceMonConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/PerceMonConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PerceMon)
