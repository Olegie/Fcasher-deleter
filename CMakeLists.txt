cmake_minimum_required(VERSION 3.21)

project(Fcasher
    VERSION 1.0.0
    DESCRIPTION "Windows-first cache inspection and cleanup utility"
    LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Fcasher currently supports Windows only.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FCASHER_BUILD_TESTS "Build unit tests" ON)

add_library(fcasher_core STATIC
    src/core/cleanup_engine.c
    src/core/file_record.c
    src/core/path_filters.c
    src/core/scan_engine.c
    src/core/scan_result.c
)

target_include_directories(fcasher_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(fcasher_core
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
)

add_library(fcasher_app STATIC
    src/app/category_registry.cpp
    src/app/cli.cpp
    src/app/command_dispatcher.cpp
    src/app/report_formatter.cpp
    src/app/safety_policy.cpp
    src/platform/windows_paths.cpp
    src/services/export_service.cpp
    src/services/report_service.cpp
)

target_include_directories(fcasher_app
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(fcasher_app
    PUBLIC
        fcasher_core
)

target_compile_definitions(fcasher_app
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
)

add_executable(fcasher
    src/main.cpp
)

target_link_libraries(fcasher
    PRIVATE
        fcasher_app
)

target_compile_definitions(fcasher
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
)

enable_testing()

if(FCASHER_BUILD_TESTS)
    add_executable(fcasher_tests
        tests/test_main.cpp
        tests/test_cli.cpp
        tests/test_filters.cpp
        tests/test_scan.cpp
        tests/test_safety.cpp
    )

    target_include_directories(fcasher_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_link_libraries(fcasher_tests
        PRIVATE
            fcasher_app
    )

    target_compile_definitions(fcasher_tests
        PRIVATE
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX
    )

    add_test(NAME fcasher_tests COMMAND fcasher_tests)
endif()
