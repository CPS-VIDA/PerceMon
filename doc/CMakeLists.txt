if(NOT PERCEMON_DOCS)
  return()
endif()

find_package(Doxygen REQUIRED dot)
set(CMAKE_FOLDER "doc")

set(DOC_INPUTS ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/doc/index.md)
set(DOXYGEN_PROJECT_NAME "PerceMon")
set(DOXYGEN_PROJECT_NUMBER "${PERCEMON_FULL_VERSION}")
set(DOXYGEN_PROJECT_BRIEF "")
set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set(DOXYGEN_QUIET YES)
# set(DOXYGEN_EXTRACT_ALL YES)

set(DOXYGEN_GENERATE_XML YES)
set(DOXYGEN_GENERATE_MAN NO)
set(DOXYGEN_GENERATE_HTML YES)
set(DOXYGEN_GENERATE_TREEVIEW YES)
set(DOXYGEN_HTML_EXTRA_STYLESHEET ${CMAKE_CURRENT_LIST_DIR}/doxygen-awesome.css)

doxygen_add_docs(docs ${DOC_INPUTS} COMMENT "Generate docs for libPerceMon")

if(NOT GIT_FOUND)
  message(
    STATUS "Git was not found; the 'gh-pages.{clean,copy,update}' targets "
           "will be unavailable.")
  return()
endif()

add_custom_target(
  percemon-gh-pages.clean
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/clean-gh-pages.cmake
  COMMAND ${CMAKE_COMMAND} -E remove_directory search
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/gh-pages
  COMMENT "Cleaning up doc/gh-pages"
  VERBATIM)
set_target_properties(percemon-gh-pages.clean PROPERTIES FOLDER ${CMAKE_FOLDER})

add_custom_target(
  percemon-gh-pages.copy
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/html
          ${CMAKE_CURRENT_LIST_DIR}/gh-pages
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/gh-pages
  COMMENT
    "Copying the documentation from ${CMAKE_CURRENT_BINARY_DIR}/html to doc/gh-pages"
  DEPENDS docs percemon-gh-pages.clean
  VERBATIM)
set_target_properties(percemon-gh-pages.copy PROPERTIES FOLDER ${CMAKE_FOLDER})

execute_process(
  COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_SOURCE_DIR} rev-parse --short HEAD
  OUTPUT_VARIABLE PERCEMON_GIT_SHORT_SHA
  OUTPUT_STRIP_TRAILING_WHITESPACE)

add_custom_target(
  percemon-gh-pages.update
  COMMAND ${GIT_EXECUTABLE} add --all .
  COMMAND ${GIT_EXECUTABLE} commit -m "Update to ${PERCEMON_GIT_SHORT_SHA}"
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/gh-pages
  COMMENT "Updating the gh-pages branch with freshly built documentation"
  DEPENDS percemon-gh-pages.copy
  VERBATIM)
set_target_properties(percemon-gh-pages.update PROPERTIES FOLDER
                                                          ${CMAKE_FOLDER})

# Add an install target to install the docs
include(GNUInstallDirs)

install(DIRECTORY ${DOXYGEN_OUTPUT_DIRECTORY}/html
        DESTINATION ${CMAKE_INSTALL_DOCDIR})
