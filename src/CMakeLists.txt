
set(
    DELIMIT_SOURCES
    main.cpp
    application.cpp
    window.cpp
    frame.cpp
    buffer.cpp
    search.cpp
    base/unicode.cpp
    base/string.cpp
    base/glob.cpp
    base/regex.cpp
    base/os/path.cpp
    base/os/stat.cpp
    base/os/core.cpp
    base/fdo/base_directory.cpp
    base/logging.cpp
)

link_directories(${GTKMM_LIBRARY_DIRS} ${GTKSOURCEVIEWMM_LIBRARY_DIRS})
include_directories(${GTKMM_INCLUDE_DIRS} ${GTKSOURCEVIEWMM_INCLUDE_DIRS})

add_executable(delimit ${DELIMIT_SOURCES})

target_link_libraries(delimit ${GTKMM_LIBRARIES} ${GTKSOURCEVIEWMM_LIBRARIES})
