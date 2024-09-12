cmake_minimum_required(VERSION 3.30.3)
project(vlcrm VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
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

function(download_extract_if_absent url path dir)
    if(NOT EXISTS "${dir}")
        download_if_absent("${url}" "${path}")
        file(ARCHIVE_EXTRACT INPUT "${path}" DESTINATION "${dir}")
    endif()
endfunction()

if(NOT VLC_HEADERS)
    if(EXISTS /usr/include/vlc/plugins)
        set(VLC_HEADERS_DEFAULT /usr/include/vlc/plugins)
    else()
        download_extract_if_absent(
            "https://download.videolan.org/vlc/last/${VLC_NAME}.tar.xz" 
            "${DEPS_DIR}/download/${VLC_NAME}.tar.xz" 
            "${DEPS_DIR}/vlc-src")
        set(VLC_HEADERS_DEFAULT "${DEPS_DIR}/vlc-src/${VLC_NAME}/include")
    endif()
endif()
set(VLC_HEADERS "${VLC_HEADERS_DEFAULT}" CACHE PATH "Path to libvlccore headers directory")

if(NOT VLC_LIB)
    if(APPLE)
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL CMAKE_HOST_SYSTEM_PROCESSOR AND EXISTS /Applications/VLC.app/Contents/MacOS/lib)
            set(VLC_LIB_DEFAULT /Applications/VLC.app/Contents/MacOS/lib)
        else()
            if (CMAKE_SYSTEM_PROCESSOR STREQUAL arm64)
                set(VLC_PLATFORM arm64)
            else()
                set(VLC_PLATFORM intel64)
            endif()
            set(VLC_DOWNLOAD_PATH ${DEPS_DIR}/download/${VLC_NAME}-${VLC_PLATFORM}.dmg)
            set(VLC_UNPACK_PATH ${DEPS_DIR}/vlc-bin/macosx/${VLC_PLATFORM})
            set(VLC_LIB_DEFAULT "${VLC_UNPACK_PATH}/lib")
            if(NOT EXISTS "${VLC_LIB_DEFAULT}")
                download_if_absent(
                    "https://download.videolan.org/vlc/last/macosx/${VLC_NAME}-${VLC_PLATFORM}.dmg" 
                    "${VLC_DOWNLOAD_PATH}")
                execute_process(COMMAND hdiutil attach "${VLC_DOWNLOAD_PATH}")
                file(GLOB VLC_COPY_PATHS "/Volumes/VLC media player/VLC.app/Contents/MacOS/*")
                file(COPY ${VLC_COPY_PATHS} DESTINATION "${VLC_UNPACK_PATH}")
                execute_process(COMMAND hdiutil detach "/Volumes/VLC media player")
            endif()
        endif()
    elseif(UNIX)
        set(VLC_LIB_DEFAULT /usr/lib)
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
        )
        set(VLC_LIB_DEFAULT "${DEPS_DIR}/vlc-bin/windows/${VLC_PLATFORM}/${VLC_NAME}")
    endif()
endif()
set(VLC_LIB ${VLC_LIB_DEFAULT} CACHE PATH "Path to libvlccore library directory")

if(NOT VLC_PLUGINS)
    if(APPLE)
        set(VLC_PLUGINS_DEFAULT /Applications/VLC.app/Contents/MacOS/plugins)
    elseif(UNIX)
        set(VLC_PLUGINS_DEFAULT /usr/lib/vlc/plugins/misc)
    elseif(WIN32)
        # WSL path to default VLC installation
        set(VLC_PLUGINS_DEFAULT "/mnt/c/Program Files/VideoLAN/VLC/plugins/misc")
    endif()
endif()
set(VLC_PLUGINS ${VLC_PLUGINS_DEFAULT} CACHE PATH "Path to plugin installation directory")

message("VLC_HEADERS=${VLC_HEADERS}, VLC_LIB=${VLC_LIB}, VLC_PLUGINS=${VLC_PLUGINS}")

include_directories(${VLC_HEADERS})
link_directories(${VLC_LIB})

add_compile_definitions(MODULE_STRING="${PROJECT_NAME}" __PLUGIN__)
add_library(vlcrm_plugin SHARED main.c)
target_link_libraries(vlcrm_plugin vlccore)

install(TARGETS vlcrm_plugin DESTINATION ${VLC_PLUGINS})
