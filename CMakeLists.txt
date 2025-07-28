cmake_minimum_required(VERSION 3.5)
project(Euteran C)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(PkgConfig REQUIRED)

if(WIN32)
    message(STATUS "Compilando no Windows")
    pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

elseif(UNIX AND NOT APPLE)
    message(STATUS "Compilando no Linux")
    pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
    pkg_check_modules(XCB REQUIRED xcb)
endif()

pkg_check_modules(GTK REQUIRED gtk4)
pkg_check_modules(ADWAITA REQUIRED libadwaita-1)
pkg_check_modules(MPG123 REQUIRED libmpg123)
pkg_check_modules(JSONGLIB REQUIRED json-glib-1.0)

include_directories(
    ${GTK_INCLUDE_DIRS}
    ${ADWAITA_INCLUDE_DIRS}
    ${MPG123_INCLUDE_DIRS}
    ${JSONGLIB_INCLUDE_DIRS}
    src
)

if(WIN32)
    include_directories(${PORTAUDIO_INCLUDE_DIRS})
elseif(UNIX AND NOT APPLE)
    include_directories(${PIPEWIRE_INCLUDE_DIRS} ${XCB_INCLUDE_DIRS})
endif()

if(UNIX AND NOT APPLE)
    set(SOURCES 
        main.c
        src/eut_audiolinux.c
        src/eut_audioinput.c
        src/eut_utils.c
        src/eut_foldercb.c
        src/eut_musiclistfunc.c
        src/eut_logs.c
        src/eut_audiodevices.c
        src/eut_async.c
        src/eut_dialogs.c
        src/eut_subwindow.c
        src/eut_widgetsfunctions.c
        src/eut_main_object.c
        src/eut_settings.c
        src/wav/dr_wav.c
        src/ogg/stb_vorbis.c
    )
elseif(WIN32)
    set(SOURCES
        main.c
        src/eut_utils.c
        src/ogg/stb_vorbis.c
    )
endif()


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

add_executable(Euteran ${SOURCES})

target_link_libraries(Euteran
    ${GTK_LIBRARIES}
    ${ADWAITA_LIBRARIES}
    ${MPG123_LIBRARIES}
    ${JSONGLIB_LIBRARIES}
    m
)

if(WIN32)
    target_link_libraries(Euteran ${PORTAUDIO_LIBRARIES})
    target_compile_options(Euteran PRIVATE ${PORTAUDIO_CFLAGS})

elseif(UNIX AND NOT APPLE)
    target_link_libraries(Euteran ${PIPEWIRE_LIBRARIES} ${XCB_LIBRARIES})
    target_compile_options(Euteran PRIVATE ${PIPEWIRE_CFLAGS} ${XCB_CFLAGS})
endif()

target_compile_options(Euteran PRIVATE ${GTK_CFLAGS} ${ADWAITA_CFLAGS} ${MPG123_CFLAGS})

add_custom_target(copy_style ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Style
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Style
)

add_dependencies(Euteran copy_style)

