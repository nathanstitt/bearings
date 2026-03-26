cmake_minimum_required(VERSION 3.22)
project(geomony_core)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GEOMONY_SOURCES
    src/GeomonyCore.cpp
    src/Config.cpp
    src/Schedule.cpp
    src/Location.cpp
    src/LocationStore.cpp
    src/Geofence.cpp
    src/GeofenceStore.cpp
    src/GeofenceEventRecord.cpp
    src/GeofenceEventStore.cpp
    src/Logger.cpp
    vendor/geomony_sqlite3/sqlite3.c
)

set(GEOMONY_COMPILE_DEFS
    SQLITE_THREADSAFE=2
    SQLITE_DEFAULT_MEMSTATUS=0
    SQLITE_DEFAULT_WAL_SYNCHRONOUS=1
    SQLITE_LIKE_DOESNT_MATCH_BLOBS
    SQLITE_OMIT_DEPRECATED
    SQLITE_OMIT_SHARED_CACHE
)

if(ANDROID)
    # Android: build shared library with JNI bindings
    add_library(geomony_core SHARED
        ${GEOMONY_SOURCES}
        ../android/src/main/cpp/geomony_jni.cpp
    )
    target_link_libraries(geomony_core log)
else()
    # iOS / other: static library
    add_library(geomony_core STATIC ${GEOMONY_SOURCES})
endif()

target_include_directories(geomony_core
    PUBLIC include
    PRIVATE vendor
)

target_compile_definitions(geomony_core PRIVATE ${GEOMONY_COMPILE_DEFS})

# Tests (only when building standalone, not as part of Android/iOS)
if(NOT ANDROID AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    option(GEOMONY_BUILD_TESTS "Build C++ tests" ON)
    if(GEOMONY_BUILD_TESTS)
        enable_testing()
        add_executable(geomony_tests test/test_core.cpp)
        target_link_libraries(geomony_tests geomony_core)
        target_include_directories(geomony_tests PRIVATE test vendor)
        add_test(NAME core COMMAND geomony_tests)
    endif()
endif()
