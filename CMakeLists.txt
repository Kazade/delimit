cmake_minimum_required(VERSION 2.6)

project(Delimit)

find_package(PkgConfig)

pkg_check_modules(GTKMM gtkmm-3.0)
pkg_check_modules(GTKSOURCEVIEWMM gtksourceviewmm-3.0)
pkg_check_modules(SQLITE sqlite3)
pkg_check_modules(PCRE libpcre)

link_libraries(
    ${SQLITE_LIBRARIES}
    ${PCRE_LIBRARIES}
)

include_directories(
    ${SQLITE_INCLUDE_DIRS}
)


add_definitions("-Wall -Wextra")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/ui DESTINATION ${CMAKE_BINARY_DIR}/delimit)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/schemes DESTINATION ${CMAKE_BINARY_DIR}/delimit)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/delimit.svg DESTINATION ${CMAKE_BINARY_DIR}/delimit)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/settings.json DESTINATION ${CMAKE_BINARY_DIR}/delimit)


link_directories(${GTKMM_LIBRARY_DIRS} ${GTKSOURCEVIEWMM_LIBRARY_DIRS})
include_directories(${GTKMM_INCLUDE_DIRS} ${GTKSOURCEVIEWMM_INCLUDE_DIRS})

add_subdirectory(src)

FIND_PACKAGE(KAZTEST)

IF(KAZTEST_FOUND)
    ADD_SUBDIRECTORY(tests)
ENDIF()


