project(ExportShader LANGUAGES CXX)

SET(FBX_TARGET_SOURCE
    main.cxx
    ../Common/Common.h
    ../Common/Common.cxx
    ../Common/GeometryUtility.h
    ../Common/GeometryUtility.cxx
)
ADD_EXECUTABLE(
   ${PROJECT_NAME}
   ${FBX_TARGET_SOURCE}
)
target_link_libraries(${PROJECT_NAME} PRIVATE fbxsdk)
fbx_target_finalize(${PROJECT_NAME})
