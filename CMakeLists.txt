cmake_minimum_required(VERSION 3.16)
project(underthec C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

option(UNDERTHEC_STATIC "link statically where the target platform allows it" ON)

set(UNDERTHEC_SOURCES
        src/main.c
        src/canvas.c
        src/color.c
        src/rng.c
        src/entity.c
        src/scene.c
)

if (WIN32)
    list(APPEND UNDERTHEC_SOURCES src/term/term_windows.c)
else ()
    list(APPEND UNDERTHEC_SOURCES src/term/term_posix.c)
endif ()

add_executable(underthec ${UNDERTHEC_SOURCES})
target_include_directories(underthec PRIVATE src)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(underthec PRIVATE -Wall -Wextra)
endif ()

if (UNDERTHEC_STATIC AND NOT APPLE)
    target_link_options(underthec PRIVATE -static)
endif ()

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")

install(TARGETS underthec RUNTIME DESTINATION bin)
if (NOT WIN32)
    install(FILES man/underthec.1 DESTINATION share/man/man1)
endif ()
