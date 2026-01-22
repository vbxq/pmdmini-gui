include(FetchContent)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

if (PMDMINI_SOURCE_DIR)
  FetchContent_Declare(pmdmini SOURCE_DIR "${PMDMINI_SOURCE_DIR}")
else()
  FetchContent_Declare(
    pmdmini
    GIT_REPOSITORY https://github.com/gzaffin/pmdmini.git
    GIT_TAG 88406c57c124ed7034617e78f3700416df552d6f
  )
endif()

FetchContent_MakeAvailable(pmdmini)

if (TARGET pmdmini)
  target_compile_options(pmdmini PRIVATE -w)
endif()
