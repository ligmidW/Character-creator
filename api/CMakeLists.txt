cmake_minimum_required(VERSION 3.10)
project(CharacterFactory)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_definitions(-DWIN32 -D_WINDOWS -D_USRDLL -D_WINDLL -DNT_PLUGIN -DCharacterFactory_EXPORTS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_compile_options(/utf-8)
endif()

set(MAYA_VERSION 2022 CACHE STRING "Maya version")
find_package(Maya REQUIRED)

find_package(FBX REQUIRED)

add_definitions(
    -DFBXSDK_SHARED
    -DFBX_ENABLED
)

set(SOURCE_FILES
    src/Commands.cpp
    src/FbxHandle.cpp
    src/AutoRigCreate.cpp
)

set(HEADER_FILES
    include/Commands.h
    include/FbxHandle.h
    include/AutoRigCreate.h
)

include_directories(${MAYA_INCLUDE_DIR})
include_directories(${FBX_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})

target_link_directories(${PROJECT_NAME} PRIVATE ${MAYA_LIBRARY_DIR})
target_link_libraries(${PROJECT_NAME} 
    ${MAYA_LIBRARIES}
    ${FBX_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".mll"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FBX_SDK_ROOT}/lib/${FBX_ARCHITECTURE}/release/libfbxsdk.dll"
        "${CMAKE_BINARY_DIR}/Release"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FBX_SDK_ROOT}/lib/${FBX_ARCHITECTURE}/release/libfbxsdk.dll"
        "$ENV{MAYA_LOCATION}/bin"
)
