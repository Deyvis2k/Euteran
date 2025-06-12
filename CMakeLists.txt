cmake_minimum_required(VERSION 3.5)
project(main C)

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

include_directories(
    ${GTK_INCLUDE_DIRS}
    ${ADWAITA_INCLUDE_DIRS}
    ${MPG123_INCLUDE_DIRS}
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
        src/audio.c
        src/utils.c
        src/ewindows.c
        src/e_widgets.c
        src/audio_devices.c
        src/widgets_devices.c
        src/widget_properties.c
        src/ogg/stb_vorbis.c
    )
elseif(WIN32)
    set(SOURCES
        main.c
        src/utils.c
        src/ogg/stb_vorbis.c
    )
endif()

add_executable(main ${SOURCES})

target_link_libraries(main
    ${GTK_LIBRARIES}
    ${ADWAITA_LIBRARIES}
    ${MPG123_LIBRARIES}
    m
)

if(WIN32)
    target_link_libraries(main ${PORTAUDIO_LIBRARIES})
    target_compile_options(main PRIVATE ${PORTAUDIO_CFLAGS})

elseif(UNIX AND NOT APPLE)
    target_link_libraries(main ${PIPEWIRE_LIBRARIES} ${XCB_LIBRARIES})
    target_compile_options(main PRIVATE ${PIPEWIRE_CFLAGS} ${XCB_CFLAGS})
endif()

target_compile_options(main PRIVATE ${GTK_CFLAGS} ${ADWAITA_CFLAGS} ${MPG123_CFLAGS})

add_custom_target(copy_style ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Style
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Style
)

add_dependencies(main copy_style)


