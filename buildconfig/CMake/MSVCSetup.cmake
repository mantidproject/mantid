
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
# Workaround Qt compiler detection
#https://forum.qt.io/topic/43778/error-when-initializing-qstringlist-using-initializer-list/3
#https://bugreports.qt.io/browse/QTBUG-39142
add_definitions ( -DQ_COMPILER_INITIALIZER_LISTS )

##########################################################################
# Additional compiler flags
##########################################################################
# /MP     - Compile .cpp files in parallel
# /W3     - Warning Level 3 (This is also the default)
# /Zc:wchar_t- - Do not treat wchar_t as a builtin type. Required for Qt to
#           work with wstring
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /W3 /Zc:wchar_t-" )

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
# On Windows we want to bundle Python.
###########################################################################
set ( PYTHON_DIR ${THIRD_PARTY_DIR}/lib/python${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION} )
## Set the variables that FindPythonLibs would set
set ( PYTHON_INCLUDE_PATH "${PYTHON_DIR}/Include" )
set ( PYTHON_LIBRARIES ${PYTHON_DIR}/libs/python${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION}.lib )

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

if ( MAKE_VATES )
  set ( PARAVIEW_PYTHON_PATHS ";${ParaView_DIR}/bin/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>;${ParaView_DIR}/lib/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>;${ParaView_DIR}/lib/site-packages;${ParaView_DIR}/lib/site-packages/vtk" )
else ()
  set (PARAVIEW_PYTHON_PATHS "" )
endif ()

configure_file ( ${PACKAGING_DIR}/mantidpython.bat.in
    ${PROJECT_BINARY_DIR}/mantidpython.bat.in @ONLY )
# place it in the appropriate directory
file(GENERATE
     OUTPUT
     ${PROJECT_BINARY_DIR}/bin/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/mantidpython.bat
     INPUT
     ${PROJECT_BINARY_DIR}/mantidpython.bat.in
  )
# install version
set ( MANTIDPYTHON_PREAMBLE "set PYTHONHOME=%_BIN_DIR%\nset PATH=%_BIN_DIR%;%_BIN_DIR%\\..\\plugins;%_BIN_DIR%\\..\\PVPlugins;%PATH%" )

if (MAKE_VATES)
  set ( PV_LIBS "%_BIN_DIR%\\..\\lib\\paraview-${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}")
  set ( PARAVIEW_PYTHON_PATHS ";${PV_LIBS}\\site-packages;${PV_LIBS}\\site-packages\\vtk" )
else ()
  set ( PARAVIEW_PYTHON_PATHS "" )
endif ()

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
