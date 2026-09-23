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
        src/app.c
        src/opts.c
        src/canvas.c
        src/color.c
        src/rng.c
        src/entity.c
        src/help.c
        src/settings.c
        src/scene/scene.c
        src/scene/effects.c
        src/scene/creatures.c
        src/scene/environment.c
        src/scene/kaiju.c
        src/scene/fishhook.c
        src/scene/feed.c
)

if (EMSCRIPTEN)
    list(APPEND UNDERTHEC_SOURCES src/web/web_main.c)
else ()
    list(APPEND UNDERTHEC_SOURCES
            src/main.c
            src/term/term_common.c
            src/teletext/tt_render.c
            src/teletext/tt_t42.c
            src/teletext/tt_ts.c
            src/teletext/tt_net.c
    )
    if (WIN32)
        list(APPEND UNDERTHEC_SOURCES src/term/term_windows.c)
    else ()
        list(APPEND UNDERTHEC_SOURCES src/term/term_posix.c)
    endif ()
endif ()

add_executable(underthec ${UNDERTHEC_SOURCES})
target_include_directories(underthec PRIVATE src)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(underthec PRIVATE -Wall -Wextra)
endif ()

if (EMSCRIPTEN)
    if (UNDERTHEC_STATIC)
        message(STATUS "UNDERTHEC_STATIC has no effect on Emscripten")
    endif ()
    set_target_properties(underthec PROPERTIES SUFFIX ".js")
    target_link_options(underthec PRIVATE
            -sMODULARIZE=1
            -sEXPORT_NAME=underthec
            -sENVIRONMENT=web
            -sALLOW_MEMORY_GROWTH=1
            -sEXPORTED_FUNCTIONS=_web_opt,_web_init,_web_fps,_web_resize,_web_key,_web_feed,_web_frame,_web_width,_web_height
            -sEXPORTED_RUNTIME_METHODS=HEAPU32,ccall
    )
    add_custom_command(TARGET underthec POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/web/index.html $<TARGET_FILE_DIR:underthec>
    )
else ()
    if (UNDERTHEC_STATIC AND NOT APPLE)
        target_link_options(underthec PRIVATE -static)
    endif ()
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")
endif ()

if (NOT EMSCRIPTEN)
    install(TARGETS underthec RUNTIME DESTINATION bin)
    if (NOT WIN32)
        install(FILES man/underthec.1 DESTINATION share/man/man1)
    endif ()
endif ()
