project(Normals LANGUAGES CXX)

SET(FBX_TARGET_SOURCE
    main.cxx
    ../Common/Common.h
    ../Common/Common.cxx
)
ADD_EXECUTABLE(
   ${PROJECT_NAME}
   ${FBX_TARGET_SOURCE}
)
ADD_CUSTOM_COMMAND(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Normals.fbx" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Normals.fbx"
)
target_link_libraries(${PROJECT_NAME} PRIVATE fbxsdk)
fbx_target_finalize(${PROJECT_NAME})
