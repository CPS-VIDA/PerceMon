add_library(mot17_helpers mot17_helpers.cc)
target_link_libraries(mot17_helpers PUBLIC PerceMon::PerceMon CLI11::CLI11
                                           spdlog::spdlog)
target_include_directories(mot17_helpers PUBLIC ${CMAKE_CURRENT_LISTS_DIR})
target_compile_options(mot17_helpers PUBLIC -pedantic -Wall -Wextra -Werror)
target_compile_features(mot17_helpers PUBLIC cxx_std_17)
# Add <filesystem> support
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # using Clang
  target_compile_options(mot17_helpers PUBLIC -lc++fs)
  target_link_libraries(mot17_helpers PUBLIC c++fs)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # using GCC
  target_compile_options(mot17_helpers PUBLIC -lstdc++fs)
  target_link_libraries(mot17_helpers PUBLIC stdc++fs)
endif()

add_executable(mot17_example mot17_example.cc)
target_link_libraries(mot17_example PRIVATE mot17_helpers)
