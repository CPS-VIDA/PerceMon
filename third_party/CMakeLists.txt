add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/mono_wedge)

# fmtlib
find_package(fmt QUIET)
if(fmt_FOUND)
  message(STATUS "System installation of fmt found")
else()
  message(STATUS "Using bundled third_party/fmt.")
  git_submodule(PATH fmt WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
  # NOTE: Workaround when building SIGNALTL
  set(FMT_INSTALL
      ${PERCEMON_INSTALL}
      CACHE BOOL "")
  add_subdirectory(fmt)
  set_target_properties(fmt PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# cppitertools
find_package(cppitertools QUIET)
if(cppitertools_FOUND)
  message(STATUS "System installation of cppitertools found")
else()
  message(STATUS "Using bundled third_party/cppitertools.")
  git_submodule(PATH cppitertools WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
  add_subdirectory(cppitertools)
endif()

# Catch2
if(_PERCEMON_BUILD_THE_TESTS)
  find_package(Catch2 QUIET)
  if(Catch2_FOUND)
    message(STATUS "System installation of Catch2 found")
  else()
    message(STATUS "Using bundled third_party/Catch2.")
    git_submodule(PATH Catch2 WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    add_subdirectory(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Catch2/contrib")
  endif()
endif()

if(PERCEMON_COVERAGE)
  git_submodule(PATH CMake-codecov WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake-codecov/cmake")
  find_package(codecov REQUIRED)
endif()

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    PARENT_SCOPE)
