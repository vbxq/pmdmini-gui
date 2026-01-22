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
target_link_libraries(pmdmini-gui-tests PRIVATE ${SDL2_LIBRARIES} pmdmini)

target_sources(pmdmini-gui-tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src/logger.cpp
  ${CMAKE_SOURCE_DIR}/src/playlist.cpp
  ${CMAKE_SOURCE_DIR}/src/player.cpp
  ${CMAKE_SOURCE_DIR}/src/scanner.cpp
  ${CMAKE_SOURCE_DIR}/src/config.cpp
)

target_link_libraries(pmdmini-gui-tests PRIVATE nlohmann_json::nlohmann_json)

include(CTest)
include(Catch)
catch_discover_tests(pmdmini-gui-tests)
