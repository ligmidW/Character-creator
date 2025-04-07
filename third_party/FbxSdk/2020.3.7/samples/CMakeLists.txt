# ---------------------------------------------------------------------------------------------------------------------------------------
# Cmake build files for FBX SDK samples
# See CMake documentation @ http://www.cmake.org/HTML/Documentation.html
#

cmake_minimum_required(VERSION 3.17)

set(FBXSDK_SAMPLES TRUE)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW) # MSVC runtime library flags are selected by an abstraction.
endif(POLICY CMP0091)

project(Samples)

if(NOT COMMAND fbx_target_finalize)
    # Architecture detection
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        if("${CMAKE_C_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64")
            SET(FBX_ARM 1)
            set(FBX_ARCH arm64) # 64 bits
        else()
            set(FBX_ARCH x64) # 64 bits
        endif()
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        if("${CMAKE_C_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM")
            SET(FBX_ARM 1)
            set(FBX_ARCH arm) # 32 bits
        else()
            set(FBX_ARCH x86) # 32 bits
        endif()
    endif()

    if(NOT FBX_CONFIGURATION)
        if(WIN32)
            set(FBX_CONFIGURATION vc${MSVC_TOOLSET_VERSION}_${FBX_ARCH}_dll)
        else()
            if(APPLE)
                set(FBX_CONFIGURATION clang_macos_dylib)
            else()
                set(FBX_CONFIGURATION gcc_${FBX_ARCH}_so_lib)
            endif()
        endif()
    endif()

    if(NOT FBX_BUILD_ROOT)
        set(FBX_BUILD_ROOT ${CMAKE_SOURCE_DIR}/../build/${FBX_CONFIGURATION})
    endif()
    IF(NOT EXISTS "${FBX_BUILD_ROOT}")
        FILE(MAKE_DIRECTORY "${FBX_BUILD_ROOT}")
    endif()

    INCLUDE("fbxsdk.cmake")
endif()

if(FBX_SHARED)
    add_compile_definitions(FBXSDK_SHARED)
endif()

# ---------------------------------------------------------------------------------------------------------------------------------------
# Public Samples
# ---------------------------------------------------------------------------------------------------------------------------------------
add_subdirectory(Animation)
add_subdirectory(Audio)
add_subdirectory(Camera)
add_subdirectory(ConvertScene)
add_subdirectory(ExportDocument)
add_subdirectory(ExportScene01)
add_subdirectory(ExportScene02)
add_subdirectory(ExportScene03)
add_subdirectory(ExportScene04)
add_subdirectory(ExportScene05)
add_subdirectory(ExportShader)
add_subdirectory(ExternalDocsRefs)
add_subdirectory(FileStream)
add_subdirectory(ImportScene)
add_subdirectory(Instances)
add_subdirectory(Layers)
add_subdirectory(Normals)
add_subdirectory(Pivot)
add_subdirectory(ProceduralTexture)
add_subdirectory(ShapeAttributes)
add_subdirectory(StereoCamera)
add_subdirectory(SwitchBinding)
add_subdirectory(Transformations)
add_subdirectory(UserProperties)
add_subdirectory(UVSample)
if(NOT FBX_ARM AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore") # No ARM or Windows Store version of Glut and Glew
    add_subdirectory(ViewScene)
endif()

# ---------------------------------------------------------------------------------------------------------------------------------------
# UI Examples are not supported on Windows Store (UNICODE)
if(WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    add_subdirectory("UI Examples/CubeCreator")
    add_subdirectory("UI Examples/SceneTreeView")
    add_subdirectory("UI Examples/ImportExport")
endif()

# ---------------------------------------------------------------------------------------------------------------------------------------
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

# ---------------------------------------------------------------------------------------------------------------------------------------
# Private Samples
# ---------------------------------------------------------------------------------------------------------------------------------------
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/_Private)
    add_subdirectory(_Private)
endif()

SET_PROPERTY(TARGET Animation PROPERTY FOLDER                       "Samples/Public")
SET_PROPERTY(TARGET Audio PROPERTY FOLDER                           "Samples/Public")
SET_PROPERTY(TARGET Camera PROPERTY FOLDER                          "Samples/Public")
SET_PROPERTY(TARGET ConvertScene PROPERTY FOLDER                    "Samples/Public")
SET_PROPERTY(TARGET ExportDocument PROPERTY FOLDER                  "Samples/Public")
SET_PROPERTY(TARGET ExportScene01 PROPERTY FOLDER                   "Samples/Public")
SET_PROPERTY(TARGET ExportScene02 PROPERTY FOLDER                   "Samples/Public")
SET_PROPERTY(TARGET ExportScene03 PROPERTY FOLDER                   "Samples/Public")
SET_PROPERTY(TARGET ExportScene04 PROPERTY FOLDER                   "Samples/Public")
SET_PROPERTY(TARGET ExportScene05 PROPERTY FOLDER                   "Samples/Public")
SET_PROPERTY(TARGET ExportShader PROPERTY FOLDER                    "Samples/Public")
SET_PROPERTY(TARGET ExternalDocsRefs PROPERTY FOLDER                "Samples/Public")
SET_PROPERTY(TARGET FileStream PROPERTY FOLDER                      "Samples/Public")
SET_PROPERTY(TARGET ImportScene PROPERTY FOLDER                     "Samples/Public")
SET_PROPERTY(TARGET Instances PROPERTY FOLDER                       "Samples/Public")
SET_PROPERTY(TARGET Layers PROPERTY FOLDER                          "Samples/Public")
SET_PROPERTY(TARGET Normals PROPERTY FOLDER                         "Samples/Public")
SET_PROPERTY(TARGET Pivot PROPERTY FOLDER                           "Samples/Public")
SET_PROPERTY(TARGET ProceduralTexture PROPERTY FOLDER               "Samples/Public")
SET_PROPERTY(TARGET ShapeAttributes PROPERTY FOLDER                 "Samples/Public")
SET_PROPERTY(TARGET StereoCamera PROPERTY FOLDER                    "Samples/Public")
SET_PROPERTY(TARGET SwitchBinding PROPERTY FOLDER                   "Samples/Public")
SET_PROPERTY(TARGET Transformations PROPERTY FOLDER                 "Samples/Public")
SET_PROPERTY(TARGET UserProperties PROPERTY FOLDER                  "Samples/Public")
SET_PROPERTY(TARGET UVSample PROPERTY FOLDER                        "Samples/Public")
if(NOT FBX_ARM AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore") # No ARM or Windows Store version of Glut and Glew
    SET_PROPERTY(TARGET ViewScene PROPERTY FOLDER                       "Samples/Public")
endif()
IF(WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    SET_PROPERTY(TARGET UI_Example_CubeCreator PROPERTY FOLDER      "Samples/Public")
    SET_PROPERTY(TARGET UI_Example_SceneTreeView PROPERTY FOLDER    "Samples/Public")
    SET_PROPERTY(TARGET UI_Example_ImportExport PROPERTY FOLDER     "Samples/Public")
ENDIF()
