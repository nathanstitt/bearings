cmake_minimum_required(VERSION 3.22)
project(bearings_core)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BEARINGS_SOURCES
    src/BearingsCore.cpp
    src/Config.cpp
    src/Schedule.cpp
    src/Location.cpp
    src/LocationStore.cpp
    src/Geofence.cpp
    src/GeofenceStore.cpp
    src/Logger.cpp
    vendor/sqlite3/sqlite3.c
)

set(BEARINGS_COMPILE_DEFS
    SQLITE_THREADSAFE=2
    SQLITE_DEFAULT_MEMSTATUS=0
    SQLITE_DEFAULT_WAL_SYNCHRONOUS=1
    SQLITE_LIKE_DOESNT_MATCH_BLOBS
    SQLITE_OMIT_DEPRECATED
    SQLITE_OMIT_SHARED_CACHE
)

if(ANDROID)
    # Android: build shared library with JNI bindings
    add_library(bearings_core SHARED
        ${BEARINGS_SOURCES}
        ../android/src/main/cpp/bearings_jni.cpp
    )
    target_link_libraries(bearings_core log)
else()
    # iOS / other: static library
    add_library(bearings_core STATIC ${BEARINGS_SOURCES})
endif()

target_include_directories(bearings_core
    PUBLIC include
    PRIVATE vendor/sqlite3
    PRIVATE vendor
)

target_compile_definitions(bearings_core PRIVATE ${BEARINGS_COMPILE_DEFS})

# Tests (only when building standalone, not as part of Android/iOS)
if(NOT ANDROID AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    option(BEARINGS_BUILD_TESTS "Build C++ tests" ON)
    if(BEARINGS_BUILD_TESTS)
        enable_testing()
        add_executable(bearings_tests test/test_core.cpp)
        target_link_libraries(bearings_tests bearings_core)
        target_include_directories(bearings_tests PRIVATE test vendor)
        add_test(NAME core COMMAND bearings_tests)
    endif()
endif()
