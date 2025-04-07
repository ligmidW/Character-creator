================================================================================

                              FBX SDK SAMPLES README

Welcome to the FBX SDK samples readme! This document explains how to call CMake
to generate the appropriate build files.

Sincerely,
the Autodesk FBX team

================================================================================

To generate the build files you need to call CMake with the desired generator
and the following options:

FBX_SHARED     :  set at command line with -DFBX_SHARED=1 to enable the sample to 
                  link with the FBX SDK dynamic library
               
FBX_STATIC_RTL :  set at command line with -DFBX_STATIC_RTL=1 to use the static 
                  MSVCRT (/MT). By default will use the dynamic MSVCRT (/MD).
                  This option is only meaningful on the Windows platform.

NOTE:
 - omitting options will implicitly set the FBX_SHARED=1
 - On Windows platform, if you are using Visual Studio compiler, it is advised to run CMake
   from the appropriate 'Tools Command Prompt' console to make sure CMake find and use the 
   correct compiler & environment.
 - Although you can run cmake in the sample directory, to avoid mixing build files and source files
   in the same folder, we strongly suggest you create a separate build folder and run cmake from there.

Examples:
==========

1) Building the ViewScene sample (and the default settings):

    1. cd samples
    2. mkdir build
    3. cd build
    4. cmake -G "Ninja Multi-Config" ..
    5. ninja ViewScene

If you want to re-generate the solution with different settings, it is preferable that you first delete the content of
the build folder to avoid cmake cache incompatibilities.

2) Building the ViewScene sample on Windows using Ninja /MT

    cmake -G "Ninja Multi-Config" -DFBX_STATIC_RTL=1 ..
    
3) Building the ViewScene sample on Windows using Ninja shared DLL

    cmake -G "Ninja Multi-Config" -DFBX_SHARED=1 ..
    
4) Wrong configuration, will display a warning and generates a DLL version

    cmake -G "Ninja Multi-Config" -DFBX_SHARED=1 -DFBX_STATIC_RTL=1 ..
    Both FBX_SHARED and FBX_STATIC_RTL have been defined. They are mutually exclusive, considering FBX_SHARED only.

