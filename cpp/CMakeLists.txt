cmake_minimum_required(VERSION 3.22)
project(geotrack_core)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GEOTRACK_SOURCES
    src/GeotrackCore.cpp
    src/Config.cpp
    src/Schedule.cpp
    src/Location.cpp
    src/LocationStore.cpp
    src/Geofence.cpp
    src/GeofenceStore.cpp
    src/Logger.cpp
    vendor/sqlite3/sqlite3.c
)

set(GEOTRACK_COMPILE_DEFS
    SQLITE_THREADSAFE=2
    SQLITE_DEFAULT_MEMSTATUS=0
    SQLITE_DEFAULT_WAL_SYNCHRONOUS=1
    SQLITE_LIKE_DOESNT_MATCH_BLOBS
    SQLITE_OMIT_DEPRECATED
    SQLITE_OMIT_SHARED_CACHE
)

if(ANDROID)
    # Android: build shared library with JNI bindings
    add_library(geotrack_core SHARED
        ${GEOTRACK_SOURCES}
        ../android/src/main/cpp/geotrack_jni.cpp
    )
    target_link_libraries(geotrack_core log)
else()
    # iOS / other: static library
    add_library(geotrack_core STATIC ${GEOTRACK_SOURCES})
endif()

target_include_directories(geotrack_core
    PUBLIC include
    PRIVATE vendor/sqlite3
    PRIVATE vendor
)

target_compile_definitions(geotrack_core PRIVATE ${GEOTRACK_COMPILE_DEFS})
