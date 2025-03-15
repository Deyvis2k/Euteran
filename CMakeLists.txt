cmake_minimum_required(VERSION 3.5)
project(main C)

# Definir compilador
set(CMAKE_C_COMPILER "gcc")

# Definir diretório de saída para o executável no diretório de build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Encontrar pacotes necessários
find_package(PkgConfig REQUIRED)

# Verificando o PipeWire 1.2.7
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
pkg_check_modules(XCB REQUIRED xcb)
pkg_check_modules(GTK REQUIRED gtk4)
pkg_check_modules(MPG123 REQUIRED libmpg123)  # Adicionando suporte ao mpg123

# Adicionar diretórios de includes das libs encontradas
include_directories(
    ${PIPEWIRE_INCLUDE_DIRS}
    ${XCB_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
    ${MPG123_INCLUDE_DIRS}  # Incluindo diretórios do mpg123
    src
)

# Definir fontes do projeto (main.c na raiz, outros em src/)
set(SOURCES 
    main.c
    src/audio.c
    src/utils.c
    src/ewindows.c
    src/e_widgets.c
)

# Criar executável
add_executable(main ${SOURCES})

# Vincular bibliotecas ao executável
target_link_libraries(main
    ${PIPEWIRE_LIBRARIES}
    ${XCB_LIBRARIES}
    ${GTK_LIBRARIES}
    ${MPG123_LIBRARIES}  # Vinculando a biblioteca mpg123
    m  # Biblioteca matemática padrão
)

# Definir flags de compilação
target_compile_options(main PRIVATE ${PIPEWIRE_CFLAGS} ${XCB_CFLAGS} ${GTK_CFLAGS} ${MPG123_CFLAGS})

