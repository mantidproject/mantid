###########################################################################
# Source file listing
###########################################################################

set ( SRC_FILES
	src/FrameworkManagerProxy.cpp
	src/MantidVecHelper.cpp
	src/PyAlgorithmWrapper.cpp
	src/PythonInterfaceFunctions.cpp
	src/PythonScriptWriter.cpp
	src/PythonWrapper.cpp
	src/SimplePythonAPI.cpp
	src/WorkspaceProxies.cpp
	src/api_exports.cpp
	src/geometry_exports.cpp
	src/geometryhelper.cpp
	src/kernel_exports.cpp
)

set ( SRC_UNITY_IGNORE_FILES )

set ( INC_FILES
	inc/MantidPythonAPI/FrameworkManagerProxy.h
	inc/MantidPythonAPI/MantidVecHelper.h
	inc/MantidPythonAPI/PyAlgorithmWrapper.h
	inc/MantidPythonAPI/PythonInterfaceFunctions.h
	inc/MantidPythonAPI/PythonScriptWriter.h
	inc/MantidPythonAPI/SimplePythonAPI.h
	inc/MantidPythonAPI/WorkspaceProxies.h
	inc/MantidPythonAPI/api_exports.h
	inc/MantidPythonAPI/geometryhelper.h
	inc/MantidPythonAPI/kernel_exports.h
	inc/MantidPythonAPI/std_operator_definitions.h
	inc/MantidPythonAPI/stl_proxies.h
)

# Python files that need copying across
set ( PY_FILES MantidFramework.py __init__.py setup.py )

# cxxtest unit tests
set ( TEST_FILES
	test/PythonFrameworkTests.h
	test/PythonScriptWriterTest.h
	test/SimplePythonAPITest.h
)

# Python unit tests
set ( TEST_PY_FILES test/ImportTest.py
                    test/PythonAlgorithmTest.py
                    test/NumpyTest.py
		    test/WorkspaceHistoryTest.py
		    test/WorkspaceTests.py
		    test/SettingsTest.py
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(PythonAPI SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

###########################################################################
# Add local dependencies
###########################################################################

set ( Boost_USE_DEBUG_PYTHON TRUE )
find_package ( Boost REQUIRED python )
add_definitions ( -DBOOST_DEBUG_PYTHON -DBOOST_PYTHON_NO_LIB )

find_package ( Numpy REQUIRED )
include_directories ( ${PYTHON_NUMPY_INCLUDE_DIR} )
include_directories ( inc )

###########################################################################
# Copy the required python files into the build destination
###########################################################################

# Now the common files for all distributions
set ( PYTHON_INSTALL_FILES "" )
foreach ( PYFILE ${PY_FILES} )
  add_custom_command ( OUTPUT ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PYFILE}
                       DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${PYFILE}
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                         ${CMAKE_CURRENT_SOURCE_DIR}/${PYFILE}
                         ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} 
  )
  set ( PYTHON_INSTALL_FILES ${PYTHON_INSTALL_FILES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PYFILE} )
endforeach ( PYFILE )

###########################################################################
# Create the target for this directory
###########################################################################

add_library ( PythonAPI ${SRC_FILES} ${INC_FILES} ${PYTHON_INSTALL_FILES})
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
  foreach ( TYPE DLL EXE )
    add_custom_command( TARGET PythonAPI POST_BUILD 
                        COMMAND SET ARGS PY_${TYPE}_SUFFIX_RELEASE=${PY_${TYPE}_SUFFIX_RELEASE}
                        COMMAND SET ARGS PY_${TYPE}_SUFFIX_RELWITHDEBINFO=${PY_${TYPE}_SUFFIX_RELWITHDEBINFO}
                        COMMAND SET ARGS PY_${TYPE}_SUFFIX_MINSIZEREL=${PY_${TYPE}_SUFFIX_MINSIZEREL}
                        COMMAND SET ARGS PY_${TYPE}_SUFFIX_DEBUG=${PY_${TYPE}_SUFFIX_DEBUG}
                        COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
                        ${PY_${TYPE}_PREFIX}%PY_${TYPE}_SUFFIX_${CMAKE_CFG_INTDIR}% ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
                       )
  endforeach ( TYPE )
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
  set ( PYTHON_DEPS stdc++ ${MANTIDLIBS} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES}  )
else ()
  set ( PYTHON_DEPS ${MANTIDLIBS} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES}  )
endif ()

###########################################################################
# Add dependent libraries to target
###########################################################################

target_link_libraries ( PythonAPI ${PYTHON_DEPS} )

###########################################################################
# Unit tests setup
###########################################################################

if ( CXXTEST_FOUND )
  include_directories ( ../DataHandling/inc ../DataObjects/inc ../TestHelpers/inc)
  cxxtest_add_test ( PythonAPITest ${TEST_FILES} )
  target_link_libraries( PythonAPITest PythonAPI TestHelpers )
  # Specify implicit dependency, but don't link to it
  add_dependencies ( PythonAPITest UserAlgorithms )
  add_dependencies ( FrameworkTests PythonAPITest )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET PythonAPITest PROPERTY FOLDER "UnitTests" )
endif ()

# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test ( VanillaPythonTest.py ${TEST_PY_FILES} )
  add_dependencies ( VanillaPythonTest.py PythonAPI Algorithms )
  add_dependencies ( FrameworkTests VanillaPythonTest.py )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET VanillaPythonTest.py PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# things to put together a main program
###########################################################################

set ( NAME_IS_PETE OFF CACHE BOOL "Ha ha" )

find_program( GEN_TEST_MAIN generatetestmain.py
              ${PROJECT_SOURCE_DIR}/TestingTools/ ${PROJECT_SOURCE_DIR}/../TestingTools/)
mark_as_advanced ( GEN_TEST_MAIN )
if (NAME_IS_PETE AND GEN_TEST_MAIN)
  # generate a default "main.cpp" file
  add_custom_command ( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
                       COMMAND ${GEN_TEST_MAIN} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp )
  add_custom_target( russellcpp
                     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp )

  # build an executable
  add_executable(russell EXCLUDE_FROM_ALL ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp )
  target_link_libraries ( russell ${MANTIDLIBS} Algorithms DataHandling )

  # run the executable
  add_custom_target ( russellhappy
                      DEPENDS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/russell 
                      COMMAND ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/russell )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS PythonAPI DESTINATION ${BIN_DIR} )
install ( FILES ${PY_FILES} DESTINATION ${BIN_DIR} )
install ( DIRECTORY PythonAlgorithms/ DESTINATION ${PLUGINS_DIR}/PythonAlgs
                                      PATTERN ".svn" EXCLUDE PATTERN "*.pyc" EXCLUDE )
