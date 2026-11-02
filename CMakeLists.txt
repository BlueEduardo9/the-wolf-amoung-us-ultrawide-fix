cmake_minimum_required(VERSION 3.25)
project(WolfUltrawideStarter VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(WOLF_BUILD_RUNTIME "Build the Windows diagnostic DLL and test host" ${WIN32})
include(CTest)

function(wolf_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    if(MINGW)
        target_link_options(${target} PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endfunction()

add_library(wolf_core STATIC src/core.cpp)
target_include_directories(wolf_core PUBLIC include)
wolf_warnings(wolf_core)

if(BUILD_TESTING)
    add_executable(wolf_core_tests tests/core_tests.cpp)
    target_link_libraries(wolf_core_tests PRIVATE wolf_core)
    wolf_warnings(wolf_core_tests)
    add_test(NAME core COMMAND wolf_core_tests)
endif()

if(WOLF_BUILD_RUNTIME)
    if(NOT WIN32)
        message(FATAL_ERROR "The runtime requires a Windows target. Use the prepared MinGW toolchain, or disable WOLF_BUILD_RUNTIME for native tests.")
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "The runtime supports Windows x86 and x64 targets only.")
    endif()
    enable_language(RC)
    add_library(WolfUltrawide SHARED
        src/windows/dllmain.cpp
        src/windows/runtime.cpp
        src/windows/game_integration.cpp
        src/windows/exports.def
        src/windows/version.rc)
    target_link_libraries(WolfUltrawide PRIVATE wolf_core)
    target_compile_definitions(WolfUltrawide PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    set_target_properties(WolfUltrawide PROPERTIES PREFIX "")
    wolf_warnings(WolfUltrawide)
    add_custom_command(TARGET WolfUltrawide POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config/WolfUltrawide.ini"
        "$<TARGET_FILE_DIR:WolfUltrawide>/WolfUltrawide.ini")

    add_executable(WolfUWHost tools/windows_host.cpp)
    target_include_directories(WolfUWHost PRIVATE include)
    target_compile_definitions(WolfUWHost PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
    if(MINGW)
        target_link_options(WolfUWHost PRIVATE -municode)
    endif()
    wolf_warnings(WolfUWHost)

    if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
        add_test(NAME windows_smoke COMMAND WolfUWHost
            --dll "$<TARGET_FILE:WolfUltrawide>"
            --config "${CMAKE_CURRENT_SOURCE_DIR}/config/WolfUltrawide.ini"
            --log "${CMAKE_CURRENT_BINARY_DIR}/windows-smoke.log")
    endif()
endif()
