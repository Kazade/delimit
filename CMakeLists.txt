cmake_minimum_required(VERSION 2.6)

project(Delimit)

find_package(PkgConfig)

pkg_check_modules(GTKMM gtkmm-3.0)
pkg_check_modules(GTKSOURCEVIEWMM gtksourceviewmm-3.0)
pkg_check_modules(SQLITE sqlite3)
pkg_check_modules(PCRE libpcre)

FIND_PACKAGE(KAZBASE REQUIRED)

link_libraries(
    ${KAZBASE_LIBRARIES}
    ${SQLITE_LIBRARIES}
    ${PCRE_LIBRARIES}
)

include_directories(
    ${KAZBASE_INCLUDE_DIRS}
    ${SQLITE_INCLUDE_DIRS}
)


add_definitions("-Wall -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -pthread")

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


#Packaging
set(CPACK_RPM_COMPONENT_INSTALL ON)

SET(CPACK_GENERATOR "RPM")
#CPACK_RPM_PACKAGE_NAME("delimit")
#CPACK_RPM_PACKAGE_VERSION("1.0.0")
#CPACK_RPM_PACKAGE_SUMMARY("An opinionated code editor")
#CPACK_RPM_PACKAGE_RELEASE("1")
#CPACK_RPM_PACKAGE_LICENSE("The Bill & Ted License")
#CPACK_RPM_PACKAGE_GROUP("Development/Tools")
#CPACK_RPM_PACKAGE_VENDOR("Kazade Software")
#CPACK_RPM_PACKAGE_DESCRIPTION("An simple code editor designed to be integrated, rather than a sea of plugins")
#CPACK_RPM_PACKAGE_REQUIRES("gtkmm30 >= 3.6")

include(CPack)
