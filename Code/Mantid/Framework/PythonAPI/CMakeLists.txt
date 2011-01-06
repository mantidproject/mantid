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

set ( TEST_FILES test/PythonFrameworkTests.h
                 test/SimplePythonAPITest.h )

# Add local dependencies
set ( Boost_USE_DEBUG_PYTHON TRUE )
find_package ( Boost REQUIRED python )
add_definitions ( -DBOOST_DEBUG_PYTHON -DBOOST_PYTHON_NO_LIB )

find_package ( Numpy REQUIRED )
include_directories ( ${PYTHON_NUMPY_INCLUDE_DIR} )

include_directories ( inc )

# Add the target for this directory
add_library ( PythonAPI ${SRC_FILES} ${INC_FILES})
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
	
target_link_libraries ( PythonAPI ${MANTIDLIBS} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} )

# Now copy the required python files into the build destination
add_custom_command ( TARGET PythonAPI POST_BUILD 
                     COMMAND ${CMAKE_COMMAND}
                     ARGS -E copy_if_different 
                               ${CMAKE_CURRENT_SOURCE_DIR}/MantidFramework.py 
                               ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
add_custom_command ( TARGET PythonAPI POST_BUILD 
                     COMMAND ${CMAKE_COMMAND}
                     ARGS -E copy_if_different 
                               ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py 
                               ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
add_custom_command ( TARGET PythonAPI POST_BUILD 
                     COMMAND ${CMAKE_COMMAND}
                     ARGS -E copy_if_different 
                               ${CMAKE_CURRENT_SOURCE_DIR}/setup.py 
                               ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )

if ( CXXTEST_FOUND )
  include_directories ( ../DataHandling/inc ../DataObjects/inc ../TestHelpers/inc)
  cxxtest_add_test ( PythonAPITest ${TEST_FILES} )
  target_link_libraries( PythonAPITest PythonAPI DataHandling DataObjects TestHelpers)
  add_dependencies ( FrameworkTests PythonAPITest )
endif ()
