project(trashcan LANGUAGES C)
cmake_minimum_required(VERSION 3.10)

add_library(trashcan trashcan.c)

if(APPLE)
	find_library(COCOA COCOA)
	find_library(COREFOUNDATION CoreFoundation)
	include_directories(${COCOA} ${COREFOUNDATION})
	target_link_libraries(trashcan ${COCOA} ${COREFOUNDATION})
endif()
