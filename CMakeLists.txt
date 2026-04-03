cmake_minimum_required(VERSION 3.18)

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
option(FCASHER_LEGACY_WIN32_BASELINE
    "Restrict Win32 API usage to a Windows 2000/XP-era baseline. Runtime compatibility still depends on the chosen compiler runtime."
    ON)

function(fcasher_apply_common_settings target)
    target_include_directories(${target}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_compile_definitions(${target}
        PRIVATE
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX
    )

    if(FCASHER_LEGACY_WIN32_BASELINE)
        target_compile_definitions(${target}
            PRIVATE
                WINVER=0x0500
                _WIN32_WINNT=0x0500
        )
    endif()

    if(MSVC)
        target_compile_options(${target}
            PRIVATE
                /W4
        )
    else()
        target_compile_options(${target}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
        )
    endif()
endfunction()

add_library(fcasher_core STATIC
    src/core/cleanup_engine.c
    src/core/file_record.c
    src/core/path_filters.c
    src/core/scan_engine.c
    src/core/scan_result.c
)

fcasher_apply_common_settings(fcasher_core)

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

target_link_libraries(fcasher_app
    PUBLIC
        fcasher_core
)

fcasher_apply_common_settings(fcasher_app)

add_executable(fcasher_cli
    src/main.cpp
)

target_link_libraries(fcasher_cli
    PRIVATE
        fcasher_app
)

fcasher_apply_common_settings(fcasher_cli)

add_executable(fcasher WIN32
    src/gui/main_window.cpp
    src/gui/session_controller.cpp
    src/gui/winmain.cpp
)

target_link_libraries(fcasher
    PRIVATE
        fcasher_app
        comdlg32
        comctl32
        shell32
)

fcasher_apply_common_settings(fcasher)

if(FCASHER_BUILD_TESTS)
    enable_testing()

    add_executable(fcasher_tests
        tests/test_main.cpp
        tests/test_cli.cpp
        tests/test_filters.cpp
        tests/test_registry.cpp
        tests/test_scan.cpp
        tests/test_safety.cpp
    )

    target_include_directories(fcasher_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)

    target_link_libraries(fcasher_tests
        PRIVATE
            fcasher_app
    )

    fcasher_apply_common_settings(fcasher_tests)

    add_test(NAME fcasher_tests COMMAND fcasher_tests)
endif()
