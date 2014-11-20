
set(
    DELIMIT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/application.cpp
    ${CMAKE_SOURCE_DIR}/src/window.cpp
    ${CMAKE_SOURCE_DIR}/src/document_view.cpp
    ${CMAKE_SOURCE_DIR}/src/find_bar.cpp
    ${CMAKE_SOURCE_DIR}/src/awesome_bar.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/utils/indentation.cpp
    ${CMAKE_SOURCE_DIR}/src/gtk/open_files_list.cpp
    ${CMAKE_SOURCE_DIR}/src/coverage/coverage.cpp
    ${CMAKE_SOURCE_DIR}/src/linter/linter.cpp
    ${CMAKE_SOURCE_DIR}/src/autocomplete/base.cpp
    ${CMAKE_SOURCE_DIR}/src/autocomplete/datastore.cpp
    ${CMAKE_SOURCE_DIR}/src/autocomplete/indexer.cpp
    ${CMAKE_SOURCE_DIR}/src/autocomplete/provider.cpp
    ${CMAKE_SOURCE_DIR}/src/autocomplete/parsers/python.cpp
    ${CMAKE_SOURCE_DIR}/src/autocomplete/parsers/plain.cpp
    ${CMAKE_SOURCE_DIR}/src/directory_watcher.cpp
    ${CMAKE_SOURCE_DIR}/src/project_info.cpp
)

add_executable(delimit ${DELIMIT_SOURCES})

target_link_libraries(delimit ${GTKMM_LIBRARIES} ${GTKSOURCEVIEWMM_LIBRARIES})

FILE(GLOB data "${CMAKE_SOURCE_DIR}/data/*.*")
INSTALL(FILES ${data} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/delimit)

FILE(GLOB schemes "${CMAKE_SOURCE_DIR}/data/schemes/*.*")
INSTALL(FILES ${schemes} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/delimit/schemes)

FILE(GLOB ui "${CMAKE_SOURCE_DIR}/data/ui/*.*")
INSTALL(FILES ${ui} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/delimit/ui)

FILE(GLOB settings "${CMAKE_SOURCE_DIR}/data/settings/*.*")
INSTALL(FILES ${settings} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/delimit/settings)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
INSTALL(PROGRAMS ${EXECUTABLE_OUTPUT_PATH}/delimit DESTINATION bin)

install(PROGRAMS ${CMAKE_SOURCE_DIR}/data/delimit.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)

install(FILES ${CMAKE_SOURCE_DIR}/data/delimit.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps)
