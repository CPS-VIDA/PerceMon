add_library(percemon_examples INTERFACE)
target_link_libraries(percemon_examples INTERFACE PerceMon::PerceMon)
target_include_directories(percemon_examples
                           INTERFACE ${CMAKE_CURRENT_LISTS_DIR})

add_executable(test_permutations test_permutations.cc)
target_link_libraries(test_permutations PRIVATE fmt::fmt
                                                cppitertools::cppitertools)

add_subdirectory(mot17)
