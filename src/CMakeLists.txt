# Add library and module
set(PERCEMON_SOURCES ast.cc monitoring/default_monitor.cc monitoring/horizon.cc)

add_library(PerceMon ${PERCEMON_SOURCES})
add_library(PerceMon::PerceMon ALIAS PerceMon)
target_include_directories(
  PerceMon PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_compile_features(PerceMon PUBLIC cxx_std_17)
target_link_libraries(PerceMon PRIVATE mono_wedge::mono_wedge)
target_link_libraries(PerceMon PUBLIC fmt::fmt)
if(MSVC)
  target_compile_options(PerceMon PRIVATE /W4 /WX /utf-8)
else()
  target_compile_options(PerceMon PRIVATE -pedantic -Wall -Wextra -Werror)
endif()

if(PERCEMON_COVERAGE)
  add_coverage(PerceMon)
endif()
