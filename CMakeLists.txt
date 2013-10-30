cmake_minimum_required(VERSION 2.6)

project(Delimit)

find_package(PkgConfig)

pkg_check_modules(GTKMM gtkmm-3.0)
pkg_check_modules(GTKSOURCEVIEWMM gtksourceviewmm-3.0)

find_package(Boost COMPONENTS system date_time regex REQUIRED)
link_libraries(
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_REGEX_LIBRARY}
)

add_definitions("-Wall -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -pthread")

FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/ui DESTINATION ${CMAKE_BINARY_DIR}/delimit)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/schemes DESTINATION ${CMAKE_BINARY_DIR}/delimit)
FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/delimit.svg DESTINATION ${CMAKE_BINARY_DIR}/delimit)

add_subdirectory(src)
