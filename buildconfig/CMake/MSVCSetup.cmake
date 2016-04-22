
##########################################################################
# Set the SYSTEM_PACKAGE_TARGET to RUNTIME as we only want to package
# dlls
###########################################################################
set (SYSTEM_PACKAGE_TARGET RUNTIME)

###########################################################################
# Compiler options.
###########################################################################
add_definitions ( -D_WINDOWS -DMS_VISUAL_STUDIO )
add_definitions ( -D_USE_MATH_DEFINES -DNOMINMAX )
add_definitions ( -DGSL_DLL -DJSON_DLL )
add_definitions ( -DPOCO_NO_UNWINDOWS )
add_definitions ( -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS )

##########################################################################
# Additional compiler flags
##########################################################################
# /MP     - Compile .cpp files in parallel
# /w34296 - Treat warning C4396, about comparison on unsigned and zero,
#           as a level 3 warning
# /w34389 - Treat warning C4389, about equality comparison on unsigned
#           and signed, as a level 3 warning
# /Zc:wchar_t- - Do not treat wchar_t as a builtin type. Required for Qt to
#           work with wstring
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /w34296 /w34389 /Zc:wchar_t-" )

# As discussed here: http://code.google.com/p/googletest/issues/detail?id=412
# gtest requires changing the _VARAIDIC_MAX value for VS2012 as it defaults to 5
add_definitions ( -D_variadic_max=10 )

# Set PCH heap limit, the default does not work when running msbuild from the commandline for some reason
# Any other value lower or higher seems to work but not the default. It it is fine without this when compiling
# in the GUI though...
set ( VISUALSTUDIO_COMPILERHEAPLIMIT 160 )
# It make or may not already be set so override if it is (assumes if in CXX also in C)
if ( CMAKE_CXX_FLAGS MATCHES "(/Zm)([0-9]+)" )
 string ( REGEX REPLACE "(/Zm)([0-9]+)" "\\1${VISUALSTUDIO_COMPILERHEAPLIMIT}" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} )
 string ( REGEX REPLACE "(/Zm)([0-9]+)" "\\1${VISUALSTUDIO_COMPILERHEAPLIMIT}" CMAKE_C_FLAGS ${CMAKE_C_FLAGS} )
else()
set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zm${VISUALSTUDIO_COMPILERHEAPLIMIT}" )
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zm${VISUALSTUDIO_COMPILERHEAPLIMIT}" )
endif()

###########################################################################
# On Windows we want to bundle Python. The necessary libraries are in
# THIRD_PARTY_DIR/lib/python2.7
###########################################################################
set ( PYTHON_DIR ${THIRD_PARTY_DIR}/lib/python2.7 )
## Set the variables that FindPythonLibs would set
set ( PYTHON_INCLUDE_PATH "${PYTHON_DIR}/Include" )
set ( PYTHON_LIBRARIES ${PYTHON_DIR}/libs/python27.lib )

## The executable
set ( PYTHON_EXECUTABLE "${PYTHON_DIR}/python.exe" CACHE FILEPATH "The location of the python executable" FORCE )
# The "pythonw" executable that avoids raising another terminal when it runs. Used for IPython
set ( PYTHONW_EXECUTABLE "${PYTHON_DIR}/pythonw.exe" CACHE FILEPATH
      "The location of the pythonw executable. This suppresses the new terminal window on startup" FORCE )

###########################################################################
# If required, find tcmalloc
###########################################################################
# Not ready for production use with MSVC 2015
option ( USE_TCMALLOC "If true, link with tcmalloc" OFF )
# If not wanted, just carry on without it
if ( USE_TCMALLOC )
  # Only link in release configurations. There seems to be problem linking in debug mode
  set ( TCMALLOC_LIBRARIES optimized "${CMAKE_LIBRARY_PATH}/libtcmalloc_minimal.lib" )
  # Use an alternate variable name so that it is only set on Windows
  set ( TCMALLOC_LIBRARIES_LINKTIME ${TCMALLOC_LIBRARIES})
  set ( _configs RELEASE RELWITHDEBINFO MINSIZEREL )
  set ( _targets EXE SHARED )
  foreach ( _tgt ${_targets})
    foreach ( _cfg ${_configs})
      set ( CMAKE_${_tgt}_LINKER_FLAGS_${_cfg} "${CMAKE_${_tgt}_LINKER_FLAGS_${_cfg}} /INCLUDE:__tcmalloc" )
    endforeach ()
  endforeach ()
else ( USE_TCMALLOC )
  message ( STATUS "TCMalloc will not be included." )
endif ()

set ( CONSOLE ON CACHE BOOL "Switch for enabling/disabling the console" )

###########################################################################
# Windows import library needs to go to bin as well
###########################################################################
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin )

###########################################################################
# Configure IDE/commandline startup scripts
###########################################################################
set ( WINDOWS_BUILDCONFIG ${PROJECT_SOURCE_DIR}/buildconfig/windows )
configure_file ( ${WINDOWS_BUILDCONFIG}/buildenv.bat.in ${PROJECT_BINARY_DIR}/buildenv.bat @ONLY )
configure_file ( ${WINDOWS_BUILDCONFIG}/command-prompt.bat ${PROJECT_BINARY_DIR}/command-prompt.bat @ONLY )
configure_file ( ${WINDOWS_BUILDCONFIG}/visual-studio.bat ${PROJECT_BINARY_DIR}/visual-studio.bat @ONLY )

###########################################################################
# Configure Mantid startup scripts
###########################################################################
set ( PACKAGING_DIR ${PROJECT_SOURCE_DIR}/buildconfig/CMake/Packaging )
# build version
set ( MANTIDPYTHON_PREAMBLE "call %~dp0..\\..\\buildenv.bat\nset PATH=%_BIN_DIR%;%_BIN_DIR%\\PVPlugins\\PVPlugins;%PATH%" )
configure_file ( ${PACKAGING_DIR}/mantidpython.bat.in
    ${PROJECT_BINARY_DIR}/mantidpython.bat @ONLY )
# build-time rule to place it in the appropriate directory
add_custom_target ( mantidpython ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_BINARY_DIR}/mantidpython.bat
    ${PROJECT_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/mantidpython.bat
    COMMENT "Generating mantidpython" )
# install version
set ( MANTIDPYTHON_PREAMBLE "set PYTHONHOME=%_BIN_DIR%\nset PATH=%_BIN_DIR%;%_BIN_DIR%\\..\\plugins;%_BIN_DIR%\\..\\PVPlugins;%PATH%" )
configure_file ( ${PACKAGING_DIR}/mantidpython.bat.in 
    ${PROJECT_BINARY_DIR}/mantidpython.bat.install @ONLY )

###########################################################################
# (Fake) installation variables to keep windows sweet
###########################################################################
set ( BIN_DIR bin )
set ( LIB_DIR ${BIN_DIR} )
set ( PLUGINS_DIR plugins )
set ( PVPLUGINS_DIR PVPlugins )
set ( PVPLUGINS_SUBDIR PVPlugins ) # Need to tidy these things up!
