set ( SRC_FILES src/api_exports.cpp src/FrameworkManagerProxy.cpp
      src/geometry_exports.cpp src/kernel_exports.cpp
      src/MantidVecHelper.cpp src/PyAlgorithmWrapper.cpp
      src/PythonInterfaceFunctions.cpp src/PythonWrapper.cpp
      src/SimplePythonAPI.cpp src/WorkspaceProxies.cpp )

set ( INC_FILES inc/MantidPythonAPI/api_exports.h
      inc/MantidPythonAPI/FrameworkManagerProxy.h
      inc/MantidPythonAPI/kernel_exports.h
      inc/MantidPythonAPI/MantidVecHelper.h
      inc/MantidPythonAPI/PyAlgorithmWrapper.h
      inc/MantidPythonAPI/PythonInterfaceFunctions.h
      inc/MantidPythonAPI/SimplePythonAPI.h
      inc/MantidPythonAPI/std_operator_definitions.h
      inc/MantidPythonAPI/stl_proxies.h
      inc/MantidPythonAPI/WorkspaceProxies.h )

set ( PY_FILES MantidFramework.py __init__.py setup.py )

set ( TEST_FILES test/PythonFrameworkTests.h
                 test/SimplePythonAPITest.h )

set ( TEST_PY_FILES test/ImportTest.py
                    test/PythonAlgorithmTest.py 
                    test/NumpyTest.py )

# Add local dependencies
set ( Boost_USE_DEBUG_PYTHON TRUE )
find_package ( Boost REQUIRED python )
add_definitions ( -DBOOST_DEBUG_PYTHON -DBOOST_PYTHON_NO_LIB )

find_package ( Numpy REQUIRED )
include_directories ( ${PYTHON_NUMPY_INCLUDE_DIR} )

include_directories ( inc )

# Now copy the required python files into the build destination
set ( PYTHON_INSTALL_FILES "")
foreach ( PYFILE ${PY_FILES} )
  add_custom_command ( OUTPUT ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PYFILE}
                       DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${PYFILE}
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                         ${CMAKE_CURRENT_SOURCE_DIR}/${PYFILE}
                         ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} 
  )
  set ( PYTHON_INSTALL_FILES ${PYTHON_INSTALL_FILES} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PYFILE})
endforeach ( PYFILE )

# Add the target for this directory
add_library ( PythonAPI ${SRC_FILES} ${INC_FILES} ${PYTHON_INSTALL_FILES})
# Set the name of the generated library
set_target_properties ( PythonAPI PROPERTIES OUTPUT_NAME MantidPythonAPI )
# Add to the 'Framework' group in VS
set_property ( TARGET PythonAPI PROPERTY FOLDER "Framework" )
# Library name needs to end in .pyd for Windows
if ( WIN32 )
    set_target_properties ( PythonAPI PROPERTIES SUFFIX .pyd )
endif ()
# Debug python library expects imported module names to end in _d
if ( PYTHON_DEBUG_LIBRARY )
    set_target_properties ( PythonAPI PROPERTIES DEBUG_OUTPUT_NAME MantidPythonAPI_d )
endif ()

# Note: On some Linux systems, seen on various Ubuntu versions, importing Mantid into a standalone python
# interpreter causes a segfault. It is some issue due to exception handling but the fix is 
# to ensure that the stdc++ library appears as early in the link list as possible so that it
# is loaded first, hence the hard coding of it here rather than leaving it to be implicitly defined.
if ( UNIX ) 
  set ( PYTHON_DEPS stdc++ ${MANTIDLIBS} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} )
else ()
  set ( PYTHON_DEPS ${MANTIDLIBS} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} )
endif ()

target_link_libraries ( PythonAPI ${PYTHON_DEPS} )
set ( PYTHON_DEPS )


if ( CXXTEST_FOUND )
  include_directories ( ../DataHandling/inc ../DataObjects/inc ../TestHelpers/inc)
  cxxtest_add_test ( PythonAPITest ${TEST_FILES} )
  target_link_libraries( PythonAPITest PythonAPI TestHelpers )
  # Specify implicit dependency, but don't link to it
  add_dependencies ( PythonAPITest UserAlgorithms )
  add_dependencies ( FrameworkTests PythonAPITest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET PythonAPITest PROPERTY FOLDER "FrameworkTests" )
endif ()

# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test ( VanillaPythonTest.py ${TEST_PY_FILES} )
  add_dependencies ( VanillaPythonTest.py PythonAPI Algorithms )
  add_dependencies ( FrameworkTests VanillaPythonTest.py )
endif ()
