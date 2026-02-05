add_executable(pmdmini-gui
  main.cpp
  app.cpp app.h
  config.cpp config.h
  logger.cpp logger.h
  playlist.cpp playlist.h
  player.cpp player.h
  ring_buffer.h
  scanner.cpp scanner.h
  ui.cpp ui.h
  ${TINYFILEDIALOGS_SOURCE_DIR}/tinyfiledialogs.c
)

if(MSVC)
  set_source_files_properties(
    ${TINYFILEDIALOGS_SOURCE_DIR}/tinyfiledialogs.c
    PROPERTIES
      COMPILE_OPTIONS "/w"
  )
else()
  set_source_files_properties(
    ${TINYFILEDIALOGS_SOURCE_DIR}/tinyfiledialogs.c
    PROPERTIES
      COMPILE_OPTIONS "-w"
  )
endif()

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_sources(pmdmini-gui PRIVATE ${IMGUI_SOURCES})

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)

target_include_directories(pmdmini-gui PRIVATE
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  ${SDL2_INCLUDE_DIRS}
  ${pmdmini_SOURCE_DIR}/src
  ${TINYFILEDIALOGS_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(pmdmini-gui PRIVATE
  ${SDL2_LIBRARIES}
  OpenGL::GL
  nlohmann_json::nlohmann_json
  pmdmini
)

pmdmini_gui_set_warnings(pmdmini-gui)
