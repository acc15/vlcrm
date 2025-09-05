cmake_minimum_required(VERSION 3.22)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64") # For Universal Binary (Intel and Apple Silicon)
endif()

project(vlcrm LANGUAGES C)

add_subdirectory(lib/trashcan)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_SKIP_RPATH true)

add_compile_options(-Wall -Wextra -Werror=return-type)
if(CMAKE_C_COMPILER_ID MATCHES Clang)
    add_compile_options(-Wno-unreachable-code-generic-assoc)
endif()
if(MINGW)
    add_compile_options(-Wno-implicit-function-declaration)
endif()

set(VLC_VERSION 3.0.21)
set(VLC_NAME vlc-${VLC_VERSION})
set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps)

function(download_if_absent url path)
    if(NOT EXISTS "${path}")
        message("Downloading ${url} to ${path}")
        file(DOWNLOAD "${url}" "${path}")
    endif()
endfunction()

function(download_extract_if_absent url path dir patterns)
    if(NOT EXISTS "${dir}")
        download_if_absent("${url}" "${path}")
        file(ARCHIVE_EXTRACT INPUT "${path}" DESTINATION "${dir}" PATTERNS ${patterns})
    endif()
endfunction()

if(NOT VLC_HEADERS)
    find_path(VLC_HEADERS_DEFAULT NAMES vlc_common.h vlc_plugin.h PATH_SUFFIXES vlc/plugins NO_CACHE)
    if(NOT VLC_HEADERS_DEFAULT)
        download_extract_if_absent(
            "https://download.videolan.org/vlc/last/${VLC_NAME}.tar.xz" 
            "${DEPS_DIR}/download/${VLC_NAME}.tar.xz" 
            "${DEPS_DIR}/vlc-src"
            "${VLC_NAME}/include")
        set(VLC_HEADERS_DEFAULT "${DEPS_DIR}/vlc-src/${VLC_NAME}/include")
    endif()
endif()
set(VLC_HEADERS "${VLC_HEADERS_DEFAULT}" CACHE PATH "Path to libvlccore headers directory")

if(NOT VLC_LIB)
    set(VLC_LIB_HINTS "")
    if(APPLE AND EXISTS /Applications/VLC.app/Contents/MacOS/lib)
        execute_process(
            COMMAND lipo -archs /Applications/VLC.app/Contents/MacOS/lib/libvlccore.dylib 
            OUTPUT_VARIABLE HOST_VLC_ARCH
        )
        string(FIND "${HOST_VLC_ARCH}" "arm64" HOST_VLC_ARM64)
        string(FIND "${HOST_VLC_ARCH}" "x86_64" HOST_VLC_X64)
        if (NOT HOST_VLC_ARM64 EQUAL -1 AND NOT HOST_VLC_X64 EQUAL -1)
            list(APPEND VLC_LIB_HINTS /Applications/VLC.app/Contents/MacOS/lib)
        endif()
    endif()
    find_library(VLC_LIB_DEFAULT vlccore HINTS ${VLC_LIB_HINTS} NO_CACHE)
    if(NOT VLC_LIB_DEFAULT)
        if(APPLE)
            set(VLC_DOWNLOAD_PATH "${DEPS_DIR}/download/${VLC_NAME}-universal.dmg")
            set(VLC_UNPACK_PATH "${DEPS_DIR}/vlc-bin/macosx/universal")
            set(VLC_LIB_DEFAULT "${VLC_UNPACK_PATH}/libvlccore.dylib")
            if(NOT EXISTS "${VLC_LIB_DEFAULT}")
                download_if_absent(
                    "https://download.videolan.org/vlc/last/macosx/${VLC_NAME}-universal.dmg"
                    "${VLC_DOWNLOAD_PATH}"
                )
                execute_process(COMMAND hdiutil attach "${VLC_DOWNLOAD_PATH}" 
                    -readonly -nobrowse -noverify -noautofsck 
                    -mountpoint /Volumes/vlcrm_extract)
                file(GLOB VLC_COPY_PATHS /Volumes/vlcrm_extract/VLC.app/Contents/MacOS/lib/libvlccore.*)
                file(COPY ${VLC_COPY_PATHS} DESTINATION "${VLC_UNPACK_PATH}")
                execute_process(COMMAND hdiutil detach /Volumes/vlcrm_extract)
            endif()
        elseif(WIN32)
            if (CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64)
                set(VLC_PLATFORM win64)
            else()
                set(VLC_PLATFORM win32)
            endif()
            download_extract_if_absent(
                "https://download.videolan.org/vlc/last/${VLC_PLATFORM}/${VLC_NAME}-${VLC_PLATFORM}.zip" 
                "${DEPS_DIR}/download/${VLC_NAME}-${VLC_PLATFORM}.zip"
                "${DEPS_DIR}/vlc-bin/windows/${VLC_PLATFORM}"
                "${VLC_NAME}/libvlccore.dll"
            )
            set(VLC_LIB_DEFAULT "${DEPS_DIR}/vlc-bin/windows/${VLC_PLATFORM}/${VLC_NAME}/libvlccore.dll")
        endif()
    endif()
endif()
set(VLC_LIB ${VLC_LIB_DEFAULT} CACHE PATH "Path to libvlccore library")

if(NOT VLC_PLUGINS)
    if(APPLE)
        set(VLC_PLUGINS_DEFAULT /Applications/VLC.app/Contents/MacOS/plugins)
    elseif(UNIX)
        set(VLC_PLUGINS_DEFAULT /usr/lib/vlc/plugins/misc)
    elseif(WIN32)
        # WSL path to default VLC installation
        set(VLC_PLUGINS_DEFAULT "C:/Program Files/VideoLAN/VLC/plugins/misc")
    endif()
endif()
set(VLC_PLUGINS ${VLC_PLUGINS_DEFAULT} CACHE PATH "Path to plugin installation directory")

message("VLC_HEADERS=${VLC_HEADERS}, VLC_LIB=${VLC_LIB}, VLC_PLUGINS=${VLC_PLUGINS}")

include_directories(${VLC_HEADERS} lib/trashcan)

add_compile_definitions(MODULE_STRING="${PROJECT_NAME}" __PLUGIN__)
add_library(vlcrm_plugin SHARED main.c)
target_link_libraries(vlcrm_plugin "${VLC_LIB}" trashcan)

install(TARGETS vlcrm_plugin DESTINATION "${VLC_PLUGINS}")
