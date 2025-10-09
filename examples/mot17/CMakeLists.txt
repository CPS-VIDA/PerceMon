find_package(CLI11)

add_library(mot17_helpers mot17_helpers.cc)
target_link_libraries(mot17_helpers PUBLIC PerceMon::PerceMon CLI11::CLI11
                                           fmt::fmt)
target_include_directories(mot17_helpers PUBLIC ${CMAKE_CURRENT_LISTS_DIR})
target_compile_features(mot17_helpers PUBLIC cxx_std_17)
# Add <filesystem> support
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # using GCC
  target_compile_options(mot17_helpers PUBLIC -lstdc++fs)
  target_link_libraries(mot17_helpers PUBLIC stdc++fs)
endif()

add_executable(mot17_example mot17_example.cc)
target_link_libraries(mot17_example PRIVATE mot17_helpers)

if(PERCEMON_COVERAGE) # THis is just to satisfy the linker
  add_coverage(mot17_example)
endif()
