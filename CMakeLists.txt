# latest available version on Ubuntu
cmake_minimum_required(VERSION 3.22.1)
project(vlcrm VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Werror=return-type)
if(CMAKE_C_COMPILER_ID MATCHES Clang)
    add_compile_options(-Wno-unreachable-code-generic-assoc)
endif()
if(MINGW)
    add_compile_options(-Wno-implicit-function-declaration)
endif()

if(NOT VLC_HEADERS)    
    if(UNIX AND NOT APPLE AND EXISTS /usr/include/vlc/plugins)
        set(VLC_HEADERS_DEFAULT /usr/include/vlc/plugins)
    else()
        set(VLC_SOURCE_URL https://download.videolan.org/vlc/last/vlc-3.0.21.tar.xz)
        message("Downloading VLC sources from ${VLC_SOURCE_URL}")
        include(FetchContent)
        FetchContent_Declare(vlc URL ${VLC_SOURCE_URL})
        FetchContent_MakeAvailable(vlc)
        FetchContent_GetProperties(vlc SOURCE_DIR VLC_SOURCE_DIR)
        set(VLC_HEADERS_DEFAULT ${VLC_SOURCE_DIR}/include)
    endif()
endif()
set(VLC_HEADERS ${VLC_HEADERS_DEFAULT} CACHE PATH "Path to libvlccore headers directory")

if(NOT VLC_LIB)
    if(APPLE)
        set(VLC_LIB_DEFAULT /Applications/VLC.app/Contents/MacOS/lib)
    elseif(UNIX)
        set(VLC_LIB_DEFAULT /usr/lib)
    elseif(WIN32)
        # WSL path to default VLC installation
        set(VLC_LIB_DEFAULT "/mnt/c/Program Files/VideoLAN/VLC")
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
