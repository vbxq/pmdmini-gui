cmake_minimum_required(VERSION 3.16)
project(pmdmini-gui LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PMDMINI_SOURCE_DIR "" CACHE PATH "Use local pmdmini source (offline builds)")

include(cmake/CompilerWarnings.cmake)
include(cmake/FetchImGui.cmake)
include(cmake/FetchPmdmini.cmake)
include(cmake/FetchCatch2.cmake)
include(cmake/FetchJson.cmake)
include(cmake/FetchTinyFileDialogs.cmake)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
