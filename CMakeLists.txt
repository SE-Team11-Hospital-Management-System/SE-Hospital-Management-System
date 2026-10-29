cmake_minimum_required(VERSION 3.25)
project(HMS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# HMS-NF-008: zero warnings with -Wall -Wextra. SAD 3.6: hardening flags.
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(NOT MINGW)
    add_compile_options(-fstack-protector-strong)
  endif()
endif()

option(HMS_SANITIZE "Build with AddressSanitizer (HMS-NF-009)" OFF)
option(HMS_COVERAGE "Build with line-coverage counters for gcovr (HMS-NF-008)" OFF)
if(HMS_COVERAGE AND NOT MSVC)
  add_compile_options(--coverage -O0)
  add_link_options(--coverage)
endif()
if(HMS_SANITIZE AND NOT MSVC)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

include(FetchContent)
# Web server and JSON libraries (header-only, downloaded once at configure time). SYSTEM = their warnings are not ours.
FetchContent_Declare(httplib URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.18.3.zip
                     DOWNLOAD_EXTRACT_TIMESTAMP TRUE SYSTEM)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
                     DOWNLOAD_EXTRACT_TIMESTAMP TRUE SYSTEM)
set(JSON_Install OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(httplib json)

# Every .cpp under src/ (except main.cpp) goes into the core library.
# New files are picked up automatically, so nobody has to edit this file when adding code.
file(GLOB_RECURSE HMS_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM HMS_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)
add_library(hms_core STATIC ${HMS_SOURCES})
target_include_directories(hms_core PUBLIC src)
target_link_libraries(hms_core PUBLIC httplib::httplib nlohmann_json::nlohmann_json)
if(WIN32)
  target_link_libraries(hms_core PUBLIC ws2_32)
endif()

add_executable(hms src/main.cpp)
target_link_libraries(hms PRIVATE hms_core)

# ---- tests (GoogleTest, downloaded once at configure time)
option(HMS_BUILD_TESTS "Build unit tests" ON)
if(HMS_BUILD_TESTS)
  enable_testing()
  FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
                       DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  file(GLOB HMS_TESTS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/*.cpp)
  add_executable(hms_tests ${HMS_TESTS})
  target_link_libraries(hms_tests PRIVATE hms_core GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(hms_tests DISCOVERY_TIMEOUT 60)
endif()
