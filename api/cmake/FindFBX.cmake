# FindFBX.cmake
#
# Locate FBX SDK
# This module defines:
# FBX_FOUND, if false, do not try to link to FBX
# FBX_INCLUDE_DIR, where to find the headers
# FBX_LIBRARY, where to find the library
#

# Set the FBX SDK path to local directory
set(FBX_SDK_ROOT "${CMAKE_SOURCE_DIR}/../third_party/FbxSdk/2020.3.7")

if(NOT EXISTS ${FBX_SDK_ROOT})
    message(FATAL_ERROR "FBX SDK not found at ${FBX_SDK_ROOT}")
endif()

# Find include directory
find_path(FBX_INCLUDE_DIR fbxsdk.h
    PATHS
    ${FBX_SDK_ROOT}/include
    NO_DEFAULT_PATH
)

# Determine architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(FBX_ARCHITECTURE "x64")
else()
    set(FBX_ARCHITECTURE "x86")
endif()

# Find FBX library
find_library(FBX_LIBRARY
    NAMES libfbxsdk.lib
    PATHS
    "${FBX_SDK_ROOT}/lib/${FBX_ARCHITECTURE}/release"
    NO_DEFAULT_PATH
)

# Find XML library
find_library(FBX_XML_LIBRARY
    NAMES libxml2.lib
    PATHS
    "${FBX_SDK_ROOT}/lib/${FBX_ARCHITECTURE}/release"
    NO_DEFAULT_PATH
)

# Find ZLIB library
find_library(FBX_ZLIB_LIBRARY
    NAMES zlib.lib
    PATHS
    "${FBX_SDK_ROOT}/lib/${FBX_ARCHITECTURE}/release"
    NO_DEFAULT_PATH
)

if(NOT FBX_LIBRARY)
    message(FATAL_ERROR "Could not find FBX release library at ${FBX_SDK_ROOT}/lib/${FBX_ARCHITECTURE}/release")
endif()

if(NOT FBX_XML_LIBRARY)
    message(STATUS "Could not find FBX XML library at ${FBX_SDK_ROOT}/lib/${FBX_ARCHITECTURE}/release")
endif()

if(NOT FBX_ZLIB_LIBRARY)
    message(STATUS "Could not find FBX ZLIB library at ${FBX_SDK_ROOT}/lib/${FBX_ARCHITECTURE}/release")
endif()

# Handle the QUIETLY and REQUIRED arguments and set FBX_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FBX DEFAULT_MSG
    FBX_LIBRARY FBX_XML_LIBRARY FBX_ZLIB_LIBRARY FBX_INCLUDE_DIR)

if(FBX_FOUND)
    set(FBX_LIBRARIES ${FBX_LIBRARY} ${FBX_XML_LIBRARY} ${FBX_ZLIB_LIBRARY})
    set(FBX_INCLUDE_DIRS ${FBX_INCLUDE_DIR})
endif()

mark_as_advanced(FBX_INCLUDE_DIR FBX_LIBRARY FBX_XML_LIBRARY FBX_ZLIB_LIBRARY)
