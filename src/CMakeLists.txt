# Windows application icon
if(WIN32)
  set(APP_ICON_RESOURCE "${CMAKE_SOURCE_DIR}/resources/app.rc")
else()
  set(APP_ICON_RESOURCE "")
endif()

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
  ${APP_ICON_RESOURCE}
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

# Static linking on Windows with MinGW
if(WIN32 AND MINGW)
  # Use static SDL2 and static MinGW runtime
  target_link_libraries(pmdmini-gui PRIVATE
    -static
    SDL2::SDL2main
    SDL2::SDL2-static
    OpenGL::GL
    nlohmann_json::nlohmann_json
    pmdmini
  )
  # Hide console window for GUI app
  target_link_options(pmdmini-gui PRIVATE -mwindows)
else()
  target_link_libraries(pmdmini-gui PRIVATE
    ${SDL2_LIBRARIES}
    OpenGL::GL
    nlohmann_json::nlohmann_json
    pmdmini
  )
endif()

pmdmini_gui_set_warnings(pmdmini-gui)
