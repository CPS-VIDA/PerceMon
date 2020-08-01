add_library(mot17_helpers mot17_helpers.cc)
target_link_libraries(mot17_helpers PUBLIC PerceMon::PerceMon CLI11::CLI11
                                           spdlog::spdlog)
target_include_directories(mot17_helpers PUBLIC ${CMAKE_CURRENT_LISTS_DIR})

add_executable(mot17_example mot17_example.cc)
target_link_libraries(mot17_example PRIVATE mot17_helpers)
