
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

FILE(GLOB_RECURSE TEST_FILES *.h)

ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    COMMAND ${KAZTEST_EXECUTABLE} --output ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ${TEST_FILES}
    DEPENDS ${TEST_FILES}
)

set(
    DELIMIT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/autocomplete/parsers/python.cpp
    ${CMAKE_SOURCE_DIR}/src/autocomplete/parsers/plain.cpp
    ${CMAKE_SOURCE_DIR}/src/autocomplete/base.cpp
    ${CMAKE_SOURCE_DIR}/src/project_info.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/rank.cpp
)

ADD_EXECUTABLE(tests ${TEST_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ${DELIMIT_SOURCES})
target_link_libraries(tests ${GTKMM_LIBRARIES} ${GTKSOURCEVIEWMM_LIBRARIES})

ADD_TEST(suite tests)
