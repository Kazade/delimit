
set(
    DELIMIT_SOURCES
    main.cpp
    application.cpp
    window.cpp
    frame.cpp
    buffer.cpp
    search.cpp
    utils.cpp
    coverage/coverage.cpp
    linter/linter.cpp
)

link_directories(${GTKMM_LIBRARY_DIRS} ${GTKSOURCEVIEWMM_LIBRARY_DIRS})
include_directories(${GTKMM_INCLUDE_DIRS} ${GTKSOURCEVIEWMM_INCLUDE_DIRS})

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
