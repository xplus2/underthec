cmake_minimum_required(VERSION 3.16)
project(underthec C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

file(READ ${CMAKE_SOURCE_DIR}/src/version.h UNDERTHEC_VERSION_H)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS src/version.h)
string(REGEX MATCH "TOOL_VERSION \"([0-9]+)\\.([0-9]+)\\.([0-9]+)\"" _ "${UNDERTHEC_VERSION_H}")
set(UNDERTHEC_VERSION_STRING "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
foreach (part MAJOR MINOR PATCH)
    string(REGEX MATCH "TOOL_VERSION_${part} ([0-9]+)" _ "${UNDERTHEC_VERSION_H}")
    list(APPEND UNDERTHEC_VERSION_PARTS "${CMAKE_MATCH_1}")
endforeach ()
string(REPLACE ";" "." UNDERTHEC_VERSION_PARTS "${UNDERTHEC_VERSION_PARTS}")
if (NOT UNDERTHEC_VERSION_STRING STREQUAL UNDERTHEC_VERSION_PARTS)
    message(FATAL_ERROR "src/version.h: TOOL_VERSION ${UNDERTHEC_VERSION_STRING} != MAJOR.MINOR.PATCH ${UNDERTHEC_VERSION_PARTS}")
endif ()

option(UNDERTHEC_STATIC "link statically where the target platform allows it" ON)

set(UNDERTHEC_XSCREENSAVER_BINDIR "libexec/xscreensaver" CACHE PATH "install dir for the xscreensaver binary")
set(UNDERTHEC_XSCREENSAVER_CONFIGDIR "libexec/xscreensaver" CACHE PATH "install dir for the xscreensaver .xml config-spec")

set(UNDERTHEC_CORE_SOURCES
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
set(UNDERTHEC_SOURCES ${UNDERTHEC_CORE_SOURCES})

if (EMSCRIPTEN)
    list(APPEND UNDERTHEC_SOURCES src/config.c src/web/web_main.c)
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
        enable_language(RC)
        list(APPEND UNDERTHEC_SOURCES src/term/term_windows.c src/win/underthec.rc)
        set_source_files_properties(src/win/underthec.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/src/win/underthec.ico)
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
            -sEXPORTED_FUNCTIONS=_web_opt,_web_init,_web_fps,_web_resize,_web_key,_web_click,_web_frame,_web_width,_web_height
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

if (WIN32 AND NOT EMSCRIPTEN)
    enable_language(RC)
    add_executable(underthec_scr WIN32
            ${UNDERTHEC_CORE_SOURCES}
            src/config.c
            src/scr/scr_main.c
            src/scr/scr_cfg.c
            src/scr/scr.rc
    )
    set_source_files_properties(src/scr/scr.rc PROPERTIES OBJECT_DEPENDS "${CMAKE_SOURCE_DIR}/src/scr/scr.manifest;${CMAKE_SOURCE_DIR}/src/win/scr.ico")
    target_include_directories(underthec_scr PRIVATE src)
    set_target_properties(underthec_scr PROPERTIES OUTPUT_NAME underthec SUFFIX ".scr")
    target_link_libraries(underthec_scr PRIVATE gdi32 advapi32)
    if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(underthec_scr PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra>)
    endif ()
    if (UNDERTHEC_STATIC)
        target_link_options(underthec_scr PRIVATE -static)
    endif ()
    install(TARGETS underthec_scr RUNTIME DESTINATION bin)
endif ()

if (NOT WIN32 AND NOT EMSCRIPTEN)
    find_package(X11)
    if (X11_FOUND)
        add_executable(underthec_xscr
                ${UNDERTHEC_CORE_SOURCES}
                src/config.c
                src/xscr/xscr_main.c
                src/xscr/xscr_font.c
        )
        target_include_directories(underthec_xscr PRIVATE src ${X11_INCLUDE_DIR})
        target_link_libraries(underthec_xscr PRIVATE ${X11_LIBRARIES})
        if (X11_Xft_FOUND)
            find_package(Freetype)
            if (FREETYPE_FOUND)
                target_compile_definitions(underthec_xscr PRIVATE HAVE_XFT)
                target_include_directories(underthec_xscr PRIVATE ${X11_Xft_INCLUDE_PATH} ${FREETYPE_INCLUDE_DIRS})
                target_link_libraries(underthec_xscr PRIVATE ${X11_Xft_LIB} ${FREETYPE_LIBRARIES})
            else ()
                message(STATUS "Xft found but freetype2 development headers are missing, building without Xft")
            endif ()
        endif ()
        if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(underthec_xscr PRIVATE -Wall -Wextra)
        endif ()
        if (UNDERTHEC_STATIC)
            message(STATUS "UNDERTHEC_STATIC has no effect on the xscreensaver build (X11/Xft aren't available static)")
        endif ()
        install(PROGRAMS $<TARGET_FILE:underthec_xscr> DESTINATION ${UNDERTHEC_XSCREENSAVER_BINDIR} RENAME underthec)
        install(FILES src/xscr/underthec.xml DESTINATION ${UNDERTHEC_XSCREENSAVER_CONFIGDIR})
    else ()
        message(STATUS "X11 development headers not found, skipping the xscreensaver target")
    endif ()
endif ()

if (NOT EMSCRIPTEN)
    install(TARGETS underthec RUNTIME DESTINATION bin)
endif ()
