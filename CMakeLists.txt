cmake_minimum_required(VERSION 3.29.3)
project(vlcrm VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(WIN32)
  add_compile_definitions(
      NOMINMAX # Removes legacy windef.h min and max macro
  )
endif()

if(MSVC)
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS # Disable CRT secure warning to allow cross-platform stdlib usage
    )
    add_compile_options(
        /W4 # Enable informational warnings (1-4 level)
        /WX # Treat warnings as errors
        /utf-8 # Enable UTF-8 support
        /Zc:wchar_t # wchar_t as builtin type
        /Zc:forScope # Removes possibility to use out of scope loop variables
        # Removes unreferenced data or functions that are COMDATs, or that only have internal linkage. 
        # Greatly reduces binary size
        /Zc:inline
    )
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type -Wno-unreachable-code-generic-assoc)
endif()

if(NOT VLC_HEADERS)    
    if(UNIX AND NOT APPLE AND EXISTS /usr/include/vlc/plugins)
        set(VLC_HEADERS_DEFAULT /usr/include/vlc/plugins)
    else()
        include(FetchContent)
        FetchContent_Declare(vlc URL https://mirror.yandex.ru/mirrors/ftp.videolan.org/vlc/3.0.20/vlc-3.0.20.tar.xz)
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
        set(VLC_LIB_DEFAULT "C:/Program Files/VideoLAN/VLC")
    endif()
endif()
set(VLC_LIB ${VLC_LIB_DEFAULT} CACHE PATH "Path to libvlccore library directory")

if(NOT VLC_PLUGINS)
    if(APPLE)
        set(VLC_PLUGINS_DEFAULT /Applications/VLC.app/Contents/MacOS/plugins)
    elseif(UNIX)
        set(VLC_PLUGINS_DEFAULT /usr/lib/vlc/plugins/misc)
    elseif(WIN32)
        set(VLC_PLUGINS_DEFAULT "C:/Program Files/VideoLAN/VLC/plugins/misc")
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
