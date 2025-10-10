add_library(percemon_examples INTERFACE)
target_link_libraries(percemon_examples INTERFACE PerceMon::PerceMon)
target_include_directories(percemon_examples
                           INTERFACE ${CMAKE_CURRENT_LISTS_DIR})

add_subdirectory(mot17)
