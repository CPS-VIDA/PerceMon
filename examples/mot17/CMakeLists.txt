add_library(mot17_examples INTERFACE)
target_link_libraries(mot17_examples INTERFACE Percemon::Percemon)
target_include_directories(mot17_examples INTERFACE ${CMAKE_CURRENT_LISTS_DIR})
