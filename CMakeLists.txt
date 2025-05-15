cmake_minimum_required(VERSION 3.5)
project(main C)

set(CMAKE_C_COMPILER "gcc")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(PkgConfig REQUIRED)

pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
pkg_check_modules(XCB REQUIRED xcb)
pkg_check_modules(GTK REQUIRED gtk4)
pkg_check_modules(GTK REQUIRED libadwaita-1)
pkg_check_modules(MPG123 REQUIRED libmpg123)  

include_directories(
    ${PIPEWIRE_INCLUDE_DIRS}
    ${XCB_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
    ${MPG123_INCLUDE_DIRS}  
    src
)

set(SOURCES 
    main.c
    src/audio.c
    src/utils.c
    src/ewindows.c
    src/e_widgets.c
    src/audio_devices.c
    src/widgets_devices.c
    src/ogg/stb_vorbis.c
)

add_executable(main ${SOURCES})

target_link_libraries(main
    ${PIPEWIRE_LIBRARIES}
    ${XCB_LIBRARIES}
    ${GTK_LIBRARIES}
    ${MPG123_LIBRARIES} 
    m  
)

target_compile_options(main PRIVATE ${PIPEWIRE_CFLAGS} ${XCB_CFLAGS} ${GTK_CFLAGS} ${MPG123_CFLAGS})

add_custom_target(copy_style ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Style
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Style
)

add_dependencies(main copy_style)

