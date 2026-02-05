add_executable(pmdmini-gui-tests
  test_config.cpp
  test_ring_buffer.cpp
  test_scanner.cpp
  test_player_compile.cpp
  test_playlist.cpp
)

find_package(SDL2 REQUIRED)

target_include_directories(pmdmini-gui-tests PRIVATE
  ${SDL2_INCLUDE_DIRS}
  ${pmdmini_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(pmdmini-gui-tests PRIVATE Catch2::Catch2WithMain)
# Use SDL2::SDL2 directly instead of ${SDL2_LIBRARIES} to avoid SDL2main conflict with Catch2
target_link_libraries(pmdmini-gui-tests PRIVATE SDL2::SDL2 pmdmini)

target_sources(pmdmini-gui-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/logger.cpp
  ${CMAKE_SOURCE_DIR}/src/playlist.cpp
  ${CMAKE_SOURCE_DIR}/src/player.cpp
  ${CMAKE_SOURCE_DIR}/src/scanner.cpp
  ${CMAKE_SOURCE_DIR}/src/config.cpp
)

target_link_libraries(pmdmini-gui-tests PRIVATE nlohmann_json::nlohmann_json)

# Copy SDL2.dll next to test executable on Windows
if(WIN32)
  add_custom_command(TARGET pmdmini-gui-tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:SDL2::SDL2>
      $<TARGET_FILE_DIR:pmdmini-gui-tests>
  )
endif()

include(CTest)
include(Catch)
# Use DISCOVERY_MODE PRE_TEST on Windows to avoid DLL-not-found during build
# The test discovery will happen at ctest runtime instead of build time
catch_discover_tests(pmdmini-gui-tests DISCOVERY_MODE PRE_TEST)
