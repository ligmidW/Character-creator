IF(NOT WIN32)
    MESSAGE(SEND_ERROR "This sample can only compile on Windows platform.")
    RETURN()
ENDIF()

project(UI_Example_ImportExport LANGUAGES CXX)
SET(FBX_TARGET_SOURCE
    "../Common/ImportExport.cxx"
    "../Common/ImportExport.h"
    "../SceneTreeView/UI.ico"
    "../SceneTreeView/UI.rc"
    "resource.h"
    "stdafx.h"
    "UI.cxx"
    "UI.h"
)
ADD_EXECUTABLE(
    ${PROJECT_NAME}
    WIN32    #sets the application subsystem to Windows Application instead of Console
    ${FBX_TARGET_SOURCE}
)
target_link_libraries(${PROJECT_NAME} PRIVATE fbxsdk)
fbx_target_finalize(${PROJECT_NAME})
