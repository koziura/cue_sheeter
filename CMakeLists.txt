cmake_minimum_required(VERSION 3.5)

project(cue_sheeter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR OFF)
set(CMAKE_DEBUG_POSTFIX "_d")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

set(THIRD_PARTY_DIR "${PROJECT_SOURCE_DIR}/third_party")

include(third_party/dependency.cmake)

if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd6387")
endif()

add_definitions(
	-D_CRT_SECURE_NO_WARNINGS
	${THIRD_DEFINES}
)

link_directories(
	${THIRD_LINK_DIRS}
	${DEP_LINK_DIRS}
)

include_directories(
	src
	${THIRD_INCLUDE_DIRS}
)

set(THIRDPARTY_SOURSES
	${THIRD_SOURCE_DIRS}
)
source_group(TREE ${CMAKE_SOURCE_DIR} PREFIX "/" FILES ${THIRDPARTY_SOURSES})

aux_source_directory(src TOOL_SOURCES)
file(GLOB TOOL_HPP "src/*.h")

set(PROJECT_SOURCES
	${TOOL_SOURCES}
	${TOOL_HPP}
	${THIRDPARTY_SOURSES}
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
add_dependencies(${PROJECT_NAME} ${DEP_PROJECTS})

target_link_libraries(${PROJECT_NAME} ${DEP_LIBRARIES})

set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
