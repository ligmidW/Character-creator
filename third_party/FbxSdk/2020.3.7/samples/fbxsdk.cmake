set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF (FBX_SHARED AND FBX_STATIC_RTL)
    SET(FBX_STATIC_RTL)
    MESSAGE("\nBoth FBX_SHARED and FBX_STATIC_RTL have been defined. They are mutually exclusive, considering FBX_SHARED only.")
ELSEIF(NOT FBX_SHARED AND NOT FBX_STATIC_RTL)
    SET(FBX_SHARED ON)
    MESSAGE("\nFBX_SHARED set by default.")
ENDIF()

set(FBX_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

IF(FBX_SHARED)
    SET(LIB_EXTENSION ${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(LIBRARY_TYPE SHARED)
ELSE()
    SET(LIB_EXTENSION ${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(LIBRARY_TYPE STATIC)
ENDIF()

add_library(fbxsdk${FBX_FILESDK_LIB_SUFFX} ${LIBRARY_TYPE} IMPORTED GLOBAL)
if(WIN32)
    set(distShortName "")

    if(NOT FBX_SHARED)
        IF(FBX_STATIC_RTL)
            SET(FBX_RTL_SUFFX "-mt")
        ELSE()
            SET(FBX_RTL_SUFFX "-md")
        ENDIF()
    endif()

    set_target_properties(fbxsdk${FBX_FILESDK_LIB_SUFFX} PROPERTIES
        IMPORTED_LOCATION ${FBX_ROOT}/lib/${distShortName}/${FBX_ARCH}/release/libfbxsdk${FBX_FILESDK_LIB_SUFFX}${FBX_RTL_SUFFX}${LIB_EXTENSION}
        IMPORTED_LOCATION_DEBUG ${FBX_ROOT}/lib/${distShortName}/${FBX_ARCH}/debug/libfbxsdk${FBX_FILESDK_LIB_SUFFX}${FBX_RTL_SUFFX}${LIB_EXTENSION}
        INTERFACE_INCLUDE_DIRECTORIES ${FBX_ROOT}/include
        IMPORTED_IMPLIB ${FBX_ROOT}/lib/${distShortName}/${FBX_ARCH}/release/libfbxsdk${FBX_FILESDK_LIB_SUFFX}${FBX_RTL_SUFFX}${CMAKE_STATIC_LIBRARY_SUFFIX}
        IMPORTED_IMPLIB_DEBUG ${FBX_ROOT}/lib/${distShortName}/${FBX_ARCH}/debug/libfbxsdk${FBX_FILESDK_LIB_SUFFX}${FBX_RTL_SUFFX}${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
    if(NOT FBX_SHARED)
        add_library(libxml2 STATIC IMPORTED GLOBAL)
        set_target_properties(libxml2 PROPERTIES
            IMPORTED_LOCATION ${FBX_ROOT}/lib/${distShortName}/${FBX_ARCH}/release/libxml2${FBX_RTL_SUFFX}${CMAKE_STATIC_LIBRARY_SUFFIX}
            IMPORTED_LOCATION_DEBUG ${FBX_ROOT}/lib/${distShortName}/${FBX_ARCH}/debug/libxml2${FBX_RTL_SUFFX}${CMAKE_STATIC_LIBRARY_SUFFIX}
            INTERFACE_INCLUDE_DIRECTORIES ${FBX_ROOT}/include/libxml2
            IMPORTED_IMPLIB ${FBX_ROOT}/lib/${distShortName}/${FBX_ARCH}/release/libxml2${FBX_RTL_SUFFX}${CMAKE_STATIC_LIBRARY_SUFFIX}
            IMPORTED_IMPLIB_DEBUG ${FBX_ROOT}/lib/${distShortName}/${FBX_ARCH}/debug/libxml2${FBX_RTL_SUFFX}${CMAKE_STATIC_LIBRARY_SUFFIX}
        )
        target_compile_definitions(libxml2 INTERFACE LIBXML_STATIC)
    
        add_library(zlib STATIC IMPORTED GLOBAL)
        set_target_properties(zlib PROPERTIES
            IMPORTED_LOCATION ${FBX_ROOT}/lib/${distShortName}/${FBX_ARCH}/release/zlib${FBX_RTL_SUFFX}${CMAKE_STATIC_LIBRARY_SUFFIX}
            IMPORTED_LOCATION_DEBUG ${FBX_ROOT}/lib/${distShortName}/${FBX_ARCH}/debug/zlib${FBX_RTL_SUFFX}${CMAKE_STATIC_LIBRARY_SUFFIX}
            IMPORTED_IMPLIB ${FBX_ROOT}/lib/${distShortName}/${FBX_ARCH}/release/zlib${FBX_RTL_SUFFX}${CMAKE_STATIC_LIBRARY_SUFFIX}
            IMPORTED_IMPLIB_DEBUG ${FBX_ROOT}/lib/${distShortName}/${FBX_ARCH}/debug/zlib${FBX_RTL_SUFFX}${CMAKE_STATIC_LIBRARY_SUFFIX}
        )
        target_link_libraries(fbxsdk${FBX_FILESDK_LIB_SUFFX} INTERFACE zlib)
        target_link_libraries(fbxsdk${FBX_FILESDK_LIB_SUFFX} INTERFACE libxml2)
    endif()
else()
    if(APPLE)
        # Change CMAKE_OSX_SYSROOT for ios builds
        if(FBX_CONFIGURATION MATCHES "ios")
            SET(CMAKE_OSX_SYSROOT CACHE STRING "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk")
            SET(FBX_MIN_OS 12.0)
            SET(CMAKE_OSX_ARCHITECTURES "arm64")
        elseif(FBX_CONFIGURATION MATCHES "iphone")
            SET(CMAKE_OSX_SYSROOT CACHE STRING "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk")
            SET(FBX_MIN_OS 12.0)
            SET(CMAKE_OSX_ARCHITECTURES "x86_64")
        elseif(FBX_CONFIGURATION MATCHES "_legacy_")
            SET(FBX_MIN_OS 10.13)
        else()
            if(FBX_CONFIGURATION MATCHES "emscripten")

            else()
                SET(CMAKE_OSX_ARCHITECTURES "x86_64;arm64") # Setup for Universal Binary builds; this way clients don't have to set any environment variables for cross-compilation
                SET(FBX_MIN_OS 10.15)
            endif()
        endif()
        if(FBX_CONFIGURATION MATCHES "_legacy_")
            set(distShortName "legacy")
        else()
            set(distShortName "clang")
        endif()
    else()
        # use, i.e. don't skip the full RPATH for the build tree
        set(CMAKE_SKIP_BUILD_RPATH TRUE)
    
        # when building, don't use the install RPATH already
        # (but later on when installing)
        set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    
        # the RPATH to be used when installing
        set(CMAKE_INSTALL_RPATH "\$ORIGIN:\$ORIGIN/lib")
    
        # don't add the automatically determined parts of the RPATH
        # which point to directories outside the build tree to the install RPATH
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
    
        set(distShortName "")
    endif()

    set_target_properties(fbxsdk${FBX_FILESDK_LIB_SUFFX} PROPERTIES
        IMPORTED_LOCATION ${FBX_ROOT}/lib/${distShortName}/release/libfbxsdk${FBX_FILESDK_LIB_SUFFX}${LIB_EXTENSION}
        IMPORTED_LOCATION_DEBUG ${FBX_ROOT}/lib/${distShortName}/debug/libfbxsdk${FBX_FILESDK_LIB_SUFFX}${LIB_EXTENSION}
        INTERFACE_INCLUDE_DIRECTORIES ${FBX_ROOT}/include
        IMPORTED_IMPLIB ${FBX_ROOT}/lib/${distShortName}/release/libfbxsdk${FBX_FILESDK_LIB_SUFFX}${CMAKE_STATIC_LIBRARY_SUFFIX}
        IMPORTED_IMPLIB_DEBUG ${FBX_ROOT}/lib/${distShortName}/debug/libfbxsdk${FBX_FILESDK_LIB_SUFFX}${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
    target_link_libraries(fbxsdk${FBX_FILESDK_LIB_SUFFX} INTERFACE dl pthread xml2 z)
    IF(APPLE)
        target_link_libraries(fbxsdk${FBX_FILESDK_LIB_SUFFX} INTERFACE "-framework System -framework CoreServices -framework CoreFoundation -framework SystemConfiguration" iconv)
    ENDIF(APPLE)
endif()

function(fbx_target_finalize target)

    target_compile_definitions(${target} PRIVATE $<IF:$<CONFIG:Debug>,_DEBUG,NDEBUG>)

    if(WIN32)
        # 0x0602 = Windows 8 as the minimum requirement (which is needed for Windows Store support).
        # See http://msdn.microsoft.com/en-us/library/aa383745(v=VS.85).aspx for the full list.
        add_compile_definitions(_WIN32_WINNT=0x0602)

        if(NOT CMAKE_GENERATOR MATCHES Visual* AND CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
            # When using Ninja generator, we need to make sure
            # that WINAPI_FAMILY=WINAPI_FAMILY_APP and UNICODE are defined
            add_compile_definitions(
                WINDOWS_STORE=1 
                WINAPI_FAMILY=WINAPI_FAMILY_APP 
                UNICODE _UNICODE)
        endif()

        set(PDBSTRIPPED /PDBSTRIPPED:$<TARGET_PDB_FILE_DIR:${target}>/$<TARGET_PDB_FILE_BASE_NAME:${target}>_stripped.pdb)
        # With RelWithDebInfo force to generate _stripped.pdb files regardless of SDK_DEBUG_FORMAT
        set(linkFlags_RelWithDebInfo $<IF:$<CONFIG:RelWithDebInfo>,${PDBSTRIPPED},>)
        IF("${DEBUG_INFORMATION_FORMAT}" STREQUAL "stripdb")
            set(linkFlags_RelWithDebInfo ${PDBSTRIPPED})
        endif()

        if(NOT FBX_SHARED)
            IF(FBX_STATIC_RTL)
                set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # /MT
            ELSE()
                set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL") # /MD
            ENDIF()
        endif()

        # Use OUTPUT_NAME if it was specified for COMPILE_PDB_NAME
		get_target_property(output_name ${target} OUTPUT_NAME)
        if(NOT output_name STREQUAL "output_name-NOTFOUND")
            set_property(TARGET ${target} PROPERTY COMPILE_PDB_NAME ${output_name})
        else()
            set_property(TARGET ${target} PROPERTY COMPILE_PDB_NAME ${target})
        endif()
        target_link_options(${target} PRIVATE 
            "${linkFlags_RelWithDebInfo}"
             $<$<CONFIG:Release>:/debug>
             /MANIFEST:NO
             /INCREMENTAL:NO 
        )
    else()
        # Extract debug information as .debug
        get_target_property(targetType ${target} TYPE)
        if(targetType STREQUAL "SHARED_LIBRARY")
            if(APPLE)
                if(NOT FBX_ARM)
                    find_program(DSYMUTIL_PROGRAM dsymutil)
                    if (DSYMUTIL_PROGRAM)
                        add_custom_command(TARGET ${target} POST_BUILD
                            COMMAND ${DSYMUTIL_PROGRAM} $<TARGET_FILE:${target}>
                        )
                    endif()
                endif()
            else()
                # https://sourceware.org/binutils/docs/binutils/objcopy.html
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${target}> $<TARGET_FILE:${target}>.debug
                    COMMAND ${CMAKE_OBJCOPY} --strip-debug $<TARGET_FILE:${target}>
                    COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:${target}>.debug $<TARGET_FILE:${target}>
                )
            endif()
        elseif(targetType STREQUAL "STATIC_LIBRARY")
            # I don't know why -fPIC is not by default for static libraries
            set_property(TARGET ${target} PROPERTY POSITION_INDEPENDENT_CODE ON)
        endif()
        set_property(TARGET ${target} PROPERTY CXX_VISIBILITY_PRESET hidden)
        set_property(TARGET ${target} PROPERTY CMAKE_VISIBILITY_INLINES_HIDDEN ON)
    endif()

    set_target_properties( ${target}
        PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY     "${FBX_BUILD_ROOT}"
            RUNTIME_OUTPUT_DIRECTORY     "${FBX_BUILD_ROOT}"
            ARCHIVE_OUTPUT_DIRECTORY     "${FBX_BUILD_ROOT}"
            COMPILE_PDB_OUTPUT_DIRECTORY "${FBX_BUILD_ROOT}"
    )

    if(FBX_SHARED)
        set(fbxsdk_dll "lib$<TARGET_FILE_BASE_NAME:fbxsdk${FBX_FILESDK_LIB_SUFFX}>${CMAKE_SHARED_LIBRARY_SUFFIX}")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different $<TARGET_LINKER_FILE_DIR:fbxsdk${FBX_FILESDK_LIB_SUFFX}>/${fbxsdk_dll} ${FBX_BUILD_ROOT}/$<CONFIG>/${fbxsdk_dll}
        )
    endif()

endfunction(fbx_target_finalize)
