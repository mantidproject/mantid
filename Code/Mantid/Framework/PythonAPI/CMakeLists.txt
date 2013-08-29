###########################################################################
# Source file listing
###########################################################################

set ( SRC_FILES
	src/FrameworkManagerProxy.cpp
	src/MantidVecHelper.cpp
	src/PyAlgorithmWrapper.cpp
	src/PythonInterfaceFunctions.cpp
	src/PythonThreading.cpp
	src/PythonWrapper.cpp
	src/WorkspaceProxies.cpp
	src/api_exports.cpp
	src/geometry_exports.cpp
	src/geometryhelper.cpp
	src/kernel_exports.cpp
)

set ( BOOST_SRC src/boostpython.cpp )

set ( SRC_UNITY_IGNORE_FILES )

set ( INC_FILES
	inc/MantidPythonAPI/FrameworkManagerProxy.h
	inc/MantidPythonAPI/MantidVecHelper.h
	inc/MantidPythonAPI/PyAlgorithmWrapper.h
	inc/MantidPythonAPI/PythonInterfaceFunctions.h
	inc/MantidPythonAPI/PythonThreading.h
	inc/MantidPythonAPI/WorkspaceProxies.h
	inc/MantidPythonAPI/api_exports.h
	inc/MantidPythonAPI/geometryhelper.h
	inc/MantidPythonAPI/kernel_exports.h
	inc/MantidPythonAPI/stl_proxies.h
	inc/MantidPythonAPI/SharedPtrToPythonMacro.h
)

# Python files that need copying across
set ( PY_FILES MantidFramework.py __init__.py mantidsimple.py)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(PythonAPI SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

set ( TEST_PY_FILES 
  GoniometerTest.py
  ImportTest.py
  IPeaksWorkspaceTest.py
  MatrixWorkspaceTest.py
  MDHistoWorkspaceTest.py
  NumpyTest.py
  PythonAlgorithmTest.py
  SANSMaskCommandsTest.py
  SimpleAPILoadTest.py
  SimpleAPIFitTest.py
  MantidSimpleTest.py
  WorkspaceGroupTest.py
  WorkspaceHistoryTest.py
)

###########################################################################
# Clear stale pyc files
###########################################################################
clean_orphaned_pyc_files ( ${CMAKE_CURRENT_SOURCE_DIR}/PythonAlgorithms )

###########################################################################
# Add local dependencies
###########################################################################
add_definitions ( -DBOOST_DEBUG_PYTHON -DBOOST_PYTHON_NO_LIB -DBOOST_PYTHON_STATIC_LIB -DBOOST_PYTHON_SOURCE )

include_directories ( ${PYTHON_INCLUDE_PATH} )
find_package ( Numpy REQUIRED )
include_directories ( ${PYTHON_NUMPY_INCLUDE_DIR} )
include_directories ( inc )

###########################################################################
# Copy Python files to build's binary directory 
###########################################################################

copy_python_files_to_dir( "${PY_FILES}" ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} PYTHON_INSTALL_FILES )

###########################################################################
# Create the target for this directory
###########################################################################

add_library ( PythonAPI ${SRC_FILES} ${INC_FILES} ${BOOST_SRC} ${PYTHON_INSTALL_FILES} )
add_dependencies( PythonAPI PythonAlgorithmsOld )
if ( UNIX )
  set ( DLOPEN_SRC src/dlopen.c )
  add_library ( dlopen ${DLOPEN_SRC} )
if ( APPLE )
  # and in .so on the Mac
  # Need to remove OpenMP
  set ( CMAKE_C_FLAGS -m64 )
  set_target_properties ( dlopen PROPERTIES SUFFIX .so )
  target_link_libraries ( dlopen ${PYTHON_LIBRARIES} )
endif()
  add_dependencies( PythonAPI dlopen )
endif()

# Set the name of the generated library
set_target_properties ( PythonAPI PROPERTIES OUTPUT_NAME MantidPythonAPI )
# Add to the 'Framework' group in VS
set_property ( TARGET PythonAPI PROPERTY FOLDER "MantidFramework" )
# Debug python library expects imported module names to end in _d
if ( PYTHON_DEBUG_LIBRARY )
  set_target_properties ( PythonAPI PROPERTIES DEBUG_OUTPUT_NAME MantidPythonAPI_d )
endif ()

###########################################################################
# Bundled Python - A post-build event so must be after target is
# created.
###########################################################################
if( WIN32 )
  # On Windows we are bundling Python. This requires developers to have the Python directories
  # next to the Mantid libraries in order for Python to set its sys.path correctly.
  # We use xcopy to as it has the ability to copy directories but only those files that
  # have been updated. It requires native paths though.
  file(TO_NATIVE_PATH ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} BIN_CFG_NATIVE)
  add_custom_command( TARGET PythonAPI POST_BUILD
                      COMMAND ${CMAKE_COMMAND} ARGS -E echo
                      "Setting up bundled Python installation"
  )
  foreach ( PYDIR ${PY_DIST_DIRS} )
    string(REGEX REPLACE ".*/(.*)" "\\1" PYDIR_RELATIVE ${PYDIR})
    file(TO_NATIVE_PATH ${PYDIR} PYDIR_NATIVE)
    add_custom_command( TARGET PythonAPI POST_BUILD
                        COMMAND ${CMAKE_COMMAND} ARGS -E echo
                        "Updating ${PYDIR} directory"
                        COMMAND XCOPY ARGS /Y /E /F /D /I 
                        ${PYDIR_NATIVE} ${BIN_CFG_NATIVE}\\${PYDIR_RELATIVE}
                      )
  endforeach ( PYDIR )
  set ( BIN_CFG_NATIVE )
  set ( PYDIR_NATIVE )
  
  # Now the binary files
  # The mingw libs are required by the compiled fortran files to import correctly
  file ( GLOB MINGW_DLLS "${CMAKE_LIBRARY_PATH}/mingw/*.dll" )
  if ( MSVC_IDE )
    foreach ( TYPE DLL EXE EXEW )
      add_custom_command( TARGET PythonAPI POST_BUILD 
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_RELEASE=${PY_${TYPE}_SUFFIX_RELEASE}
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_RELWITHDEBINFO=${PY_${TYPE}_SUFFIX_RELWITHDEBINFO}
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_MINSIZEREL=${PY_${TYPE}_SUFFIX_MINSIZEREL}
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_DEBUG=${PY_${TYPE}_SUFFIX_DEBUG}
                          COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
                          ${PY_${TYPE}_PREFIX}%PY_${TYPE}_SUFFIX_${CMAKE_CFG_INTDIR}% ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
                        )
    endforeach ( TYPE )
    foreach ( DLL_FILE ${MINGW_DLLS} )
      add_custom_command( TARGET PythonAPI POST_BUILD
                          COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
                          ${DLL_FILE} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} )
    endforeach( DLL_FILE )
  else () # Need to do things slightly differently for nmake
    foreach ( TYPE DLL EXE EXEW )
      add_custom_command( TARGET PythonAPI POST_BUILD 
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_RELEASE=${PY_${TYPE}_SUFFIX_RELEASE}
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_RELWITHDEBINFO=${PY_${TYPE}_SUFFIX_RELWITHDEBINFO}
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_MINSIZEREL=${PY_${TYPE}_SUFFIX_MINSIZEREL}
                          COMMAND SET ARGS PY_${TYPE}_SUFFIX_DEBUG=${PY_${TYPE}_SUFFIX_DEBUG}
                          COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
                          ${PY_${TYPE}_PREFIX}%PY_${TYPE}_SUFFIX_${CMAKE_BUILD_TYPE}% ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                        )
    endforeach ( TYPE )
    foreach ( DLL_FILE MINGW_DLLS )
      add_custom_command( TARGET PythonAPI POST_BUILD
                          COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
                          ${DLL_FILE} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
    endforeach( DLL_FILE )
  endif ()
endif( WIN32 )

###########################################################################
# Platform and compiler-dependent parts
###########################################################################

# Library name needs to end in .pyd for Windows
if ( WIN32 )
  set_target_properties ( PythonAPI PROPERTIES SUFFIX .pyd )
elseif ( APPLE )
# and in .so on the Mac
  set_target_properties ( PythonAPI PROPERTIES SUFFIX .so )
endif ()
# Intensive use of templated libaries can cause large objects to be generated. These require
# an additional flag in MSVC.
if ( MSVC )
  set_target_properties ( PythonAPI PROPERTIES COMPILE_FLAGS "/bigobj" )
endif ()

# Note: On some Linux systems, seen on various Ubuntu versions, importing Mantid into a standalone python
# interpreter causes a segfault. It is some issue due to exception handling but the fix is 
# to ensure that the stdc++ library appears as early in the link list as possible so that it
# is loaded first, hence the hard coding of it here rather than leaving it to be implicitly defined.
if ( UNIX ) 
  set ( PYTHON_DEPS stdc++ ${NEXUS_C_LIBRARIES} ${MANTIDLIBS} ${PYTHON_LIBRARIES} )
else ()
  set ( PYTHON_DEPS ${MANTIDLIBS} ${PYTHON_LIBRARIES}  )
endif ()

###########################################################################
# Add dependent libraries to target
###########################################################################

target_link_libraries ( PythonAPI ${PYTHON_DEPS} )

# Disable Intel compiler coming from boost python code
if ( ${CMAKE_CXX_COMPILER_ID} MATCHES "Intel" )
  add_definitions ( -wd1875 )
endif ()

###########################################################################
# Unit tests setup
###########################################################################

add_subdirectory ( test )

###########################################################################
# things to put together a main program
###########################################################################

set ( MAIN_TARGET OFF CACHE BOOL "Enable the target to generate a 'main' executable" )

find_program( GEN_TEST_MAIN generatetestmain.py
              ${PROJECT_SOURCE_DIR}/TestingTools/ ${PROJECT_SOURCE_DIR}/../TestingTools/)
mark_as_advanced ( GEN_TEST_MAIN )
if (MAIN_TARGET AND GEN_TEST_MAIN)
  # generate a default "main.cpp" file
  add_custom_command ( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                       COMMAND ${GEN_TEST_MAIN} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp )
  add_custom_target( maincpp
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp )

  # build an executable
  add_executable(main EXCLUDE_FROM_ALL ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp )
  target_link_libraries ( main ${MANTIDLIBS} Algorithms DataHandling )

  # add required include directories from Framework
  include_directories ( ../Kernel/inc )
  include_directories ( ../Geometry/inc )
  include_directories ( ../API/inc )
  include_directories ( ../DataObjects/inc )

  # run the executable
  add_custom_target ( cleanmain
                      DEPENDS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/main 
                      COMMAND ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/main )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS PythonAPI ${SYSTEM_PACKAGE_TARGET} DESTINATION ${BIN_DIR} )
if ( UNIX )
  install ( TARGETS dlopen DESTINATION ${BIN_DIR} )
endif ()

# Pure Python files 
install ( FILES ${PY_FILES} DESTINATION ${BIN_DIR} )
# Algorithms
#install ( DIRECTORY PythonAlgorithms/ DESTINATION ${PLUGINS_DIR}/python/algorithms
#          PATTERN "*.pyc" EXCLUDE
#          PATTERN ".svn" EXCLUDE )
