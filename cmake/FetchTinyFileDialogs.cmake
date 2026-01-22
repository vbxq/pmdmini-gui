include(FetchContent)

if (POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)
endif()

FetchContent_Declare(
  tinyfiledialogs
  GIT_REPOSITORY https://github.com/native-toolkit/tinyfiledialogs.git
  GIT_TAG 1cb330e682e6fd44439417399dc830d0a3ad5722
)

FetchContent_Populate(tinyfiledialogs)
set(TINYFILEDIALOGS_SOURCE_DIR ${tinyfiledialogs_SOURCE_DIR})
