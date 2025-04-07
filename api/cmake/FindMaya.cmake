# Find Maya SDK
#
# Variables that will be defined:
# MAYA_FOUND          Defined if Maya has been found
# MAYA_INCLUDE_DIR    Path to Maya's include directory
# MAYA_LIBRARIES      Maya libraries
# MAYA_LIBRARY_DIR    Path to Maya's library directory

if(NOT DEFINED MAYA_VERSION)
    set(MAYA_VERSION 2024 CACHE STRING "Maya version")
endif()

# OS specific environment setup
set(MAYA_INSTALL_BASE_DEFAULT "C:/Program Files/Autodesk")
set(MAYA_INSTALL_DIR_DEFAULT ${MAYA_INSTALL_BASE_DEFAULT}/Maya${MAYA_VERSION})
set(MAYA_INCLUDE_DIR ${MAYA_INSTALL_DIR_DEFAULT}/include)
set(MAYA_LIBRARY_DIR ${MAYA_INSTALL_DIR_DEFAULT}/lib)

# Include directory
find_path(MAYA_INCLUDE_DIR
    maya/MFn.h
    PATHS
        ${MAYA_INCLUDE_DIR}
    DOC "Maya include path"
)

# Maya libraries
set(MAYA_LIBRARIES
    OpenMaya
    OpenMayaAnim
    OpenMayaFX
    OpenMayaRender
    OpenMayaUI
    Foundation
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Maya
    REQUIRED_VARS MAYA_INCLUDE_DIR MAYA_LIBRARY_DIR
    VERSION_VAR MAYA_VERSION
)

mark_as_advanced(
    MAYA_INCLUDE_DIR
    MAYA_LIBRARY_DIR
)
