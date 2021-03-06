message(STATUS "Building Tests in ${CMAKE_CURRENT_LIST_DIR}")
include(Catch)

set(TEST_SRCS test_ast.cc test_percemon.cc)

add_executable(percemon_tests ${TEST_SRCS})
target_include_directories(percemon_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(percemon_tests PUBLIC PerceMon::PerceMon Catch2::Catch2)
if(MSVC)
  target_compile_options(percemon_tests PRIVATE /W4 /WX /utf-8)
else()
  target_compile_options(percemon_tests PRIVATE -pedantic -Wall -Wextra -Werror)
endif()

if(PERCEMON_COVERAGE)
  add_coverage(percemon_tests)
endif()
catch_discover_tests(percemon_tests)
