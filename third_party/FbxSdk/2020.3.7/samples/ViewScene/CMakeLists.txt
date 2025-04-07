project(ViewScene LANGUAGES CXX)

IF(CMAKE_OSX_SYSROOT MATCHES "Platforms/iPhone*")
    MESSAGE(FATAL_ERROR "on iOS we don't have OpenGL stuff so we cannot compile ViewScene")
    RETURN()
ENDIF()

SET(FBX_TARGET_SOURCE
    DrawScene.h
    GetPosition.h
    GlFunctions.h
    SetCamera.h
    SceneCache.h
    SceneContext.h
    DrawText.h
    targa.h
    DrawScene.cxx
    GetPosition.cxx
    GlFunctions.cxx
    SetCamera.cxx
    SceneCache.cxx
    SceneContext.cxx
    DrawText.cxx
    main.cxx
    targa.cxx
    ../Common/Common.h
    ../Common/Common.cxx
)

ADD_EXECUTABLE(
   ${PROJECT_NAME}
   ${FBX_TARGET_SOURCE}
)

IF(WIN32)
    SET(FBX_TARGET_LIBS glut32.lib glew32.lib)
ELSE(WIN32)
    IF(APPLE)
        # if compiling of iOS skip linking with OpenGL & GLUT. We don't have these versions!!!
        IF(NOT CMAKE_OSX_SYSROOT MATCHES "Platforms/iPhone*")
            # no need for the explicit glut dependency (-lglut) but we keep the path to the framework
            # for includes and automatic library linking
            SET(FBX_TARGET_LIBS GLEW "-framework OpenGL -framework GLUT")
        ENDIF(NOT CMAKE_OSX_SYSROOT MATCHES "Platforms/iPhone*")
    ELSE(APPLE)
        SET(FBX_TARGET_LIBS glut GLEW GLU GL X11)
    ENDIF(APPLE)
ENDIF(WIN32)

target_link_libraries(${PROJECT_NAME} PRIVATE ${FBX_TARGET_LIBS} fbxsdk)

if(WIN32)
    if(FBX_ARCH STREQUAL "x64")
        set(OS win/x64)
    else()
        set(OS win/x86)
    endif()
elseif(APPLE)
    set(OS macos)
else()
    if(FBX_ARCH STREQUAL "x64")
        set(OS linux/x64)
    else()
        set(OS linux/x86)
    endif()
endif()

target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS})
fbx_target_finalize(${PROJECT_NAME})

IF(WIN32)
    ADD_CUSTOM_COMMAND(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}/glew32.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/glew32.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/libs/${OS}/glut32.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/glut32.dll"
    )
endif()

ADD_CUSTOM_COMMAND(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/humanoid.fbx" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/humanoid.fbx"
)
