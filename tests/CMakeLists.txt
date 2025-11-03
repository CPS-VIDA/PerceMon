message(STATUS "Building Tests in ${CMAKE_CURRENT_LIST_DIR}")
find_package(Catch2)
if(NOT Catch2_FOUND)
  return()
endif()

include(Catch)

set(TEST_SRCS test_boolean_evaluation.cc test_datastream.cc test_monitoring.cc
              test_spatial.cc test_stql.cc)

add_executable(percemon_tests ${TEST_SRCS})
target_include_directories(percemon_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(percemon_tests PUBLIC libPerceMon::PerceMon
                                            Catch2::Catch2WithMain)
if(MSVC)
  target_compile_options(percemon_tests PRIVATE /W4 /WX /utf-8)
else()
  target_compile_options(percemon_tests PRIVATE -pedantic -Wall -Wextra -Werror)
endif()

if(PERCEMON_COVERAGE)
  add_coverage(percemon_tests)
endif()
catch_discover_tests(percemon_tests)
