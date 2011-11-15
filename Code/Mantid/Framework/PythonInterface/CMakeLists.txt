###########################################################################
#
# Python API (version 2)
#
###########################################################################

###########################################################################
# Set local dependencies
###########################################################################
set ( Boost_LIBRARIES ) # Empty out the variable after previous use
set ( Boost_USE_DEBUG_PYTHON TRUE )
find_package ( Boost REQUIRED python )
add_definitions ( -DBOOST_DEBUG_PYTHON -DBOOST_PYTHON_NO_LIB )

find_package ( Numpy REQUIRED )
include_directories ( SYSTEM ${PYTHON_NUMPY_INCLUDE_DIR} )
set ( HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidPythonInterface )
include_directories ( inc )

# Note: 
# On some Linux systems (seen on various Ubuntu versions) importing Mantid into a standalone python
# interpreter causes a segfault. It is some issue due to exception handling but the fix is 
# to ensure that the stdc++ library appears as early in the link list as possible so that it
# is loaded first, hence the hard coding of it here rather than leaving it to be implicitly defined
# by the linker.
# 
# MG 2011/11/15: A similar issue regarding the Nexus library has now been observed. Nexus 4.3 introduced
# thread-local variables and this now causes problems whenever we try and load the library from
# python if other libraries appear before it in the link list.
if ( UNIX ) 
  set ( PYTHON_DEPS stdc++ ${NEXUS_LIBRARIES} ${MANTIDLIBS} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES}  )
else ()
  set ( PYTHON_DEPS ${MANTIDLIBS} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES}  )
endif ()

###########################################################################
# mantid package
###########################################################################
add_subdirectory ( mantid )

###########################################################################
# tests 
###########################################################################
# C++ unit tests
set ( TEST_FILES
  test/cpp/PythonObjectInstantiatorTest.h
  test/cpp/VectorDelegateTest.h # Windows doesn't like having two around
)  
  
if ( CXXTEST_FOUND )
  include_directories ( ../DataHandling/inc ../DataObjects/inc ../TestHelpers/inc )
  cxxtest_add_test ( PythonInterfaceTest ${TEST_FILES} )
  target_link_libraries( PythonInterfaceTest TestHelpers PythonKernelModule PythonAPIModule )
  # Specify implicit dependency, but don't link to it
  add_dependencies ( PythonInterfaceTest UserAlgorithms )
  add_dependencies ( FrameworkTests PythonInterfaceTest )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET PythonInterfaceTest PROPERTY FOLDER "UnitTests" )
endif ()

# python unit tests
set ( TEST_PY_FILES
  test/python/AlgorithmTest.py
  test/python/AlgorithmManagerTest.py
  test/python/AnalysisDataServiceTest.py
  test/python/FilePropertyTest.py
  test/python/FrameworkManagerTest.py
  test/python/IEventWorkspaceTest.py
  test/python/ImportModuleTest.py
  test/python/MatrixWorkspaceTest.py
  test/python/PropertyWithValueTest.py
  test/python/PythonAlgorithmTest.py
  test/python/QuatTest.py
  test/python/V3DTest.py
  test/python/WorkspaceTest.py
)

set ( PYTEST_HELPERS test/python/testhelpers.py 
)

# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test ( VanillaPythonInterfaceTest.py ${TEST_PY_FILES} )
  # Helpers
  set ( VANILLA_TEST_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/VanillaPythonInterfaceTest )
  set ( PYTEST_HELPER_INSTALLS "" )
  foreach( PYTEST_HELPER ${PYTEST_HELPERS} )
    get_filename_component(helper_file ${PYTEST_HELPER} NAME)
    add_custom_command ( OUTPUT ${VANILLA_TEST_DIR}/${helper_file}
                         DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${PYTEST_HELPER} 
                         COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                         ${CMAKE_CURRENT_SOURCE_DIR}/${PYTEST_HELPER}
                         ${VANILLA_TEST_DIR}/${helper_file} )
    set ( PYTEST_HELPER_INSTALLS ${PYTEST_HELPER_INSTALLS} ${VANILLA_TEST_DIR}/${helper_file} )
  endforeach( PYTEST_HELPER ${PYTEST_HELPERS} )
  add_custom_target ( PythonTestHelpers DEPENDS ${PYTEST_HELPER_INSTALLS} )
                      
  # Test Dependencies  
  add_dependencies ( VanillaPythonInterfaceTest.py PythonInterface Algorithms PythonTestHelpers )
  add_dependencies ( FrameworkTests VanillaPythonInterfaceTest.py )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET VanillaPythonInterfaceTest.py PROPERTY FOLDER "UnitTests" )
endif ()

