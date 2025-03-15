# tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.19)

set(TEST_SOURCE_FILES
    test_localbackend.cpp
    test_configmanager.cpp
)

add_executable(test_localbackend test_localbackend.cpp)
add_executable(test_configmanager test_configmanager.cpp)

foreach(test_target test_localbackend test_configmanager)
    target_include_directories(${test_target} PRIVATE
        ${CMAKE_SOURCE_DIR}/
    )

    target_link_libraries(${test_target} PRIVATE
        TimeBusterCore
        Qt6::Core
        Qt6::Test
        KF6::CalendarCore
        KF6::DAV
    )

    add_test(NAME ${test_target} COMMAND ${test_target})
endforeach()
