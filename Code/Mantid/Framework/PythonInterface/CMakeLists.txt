###########################################################################
# Python API (version 2)
###########################################################################

###########################################################################
# Set local dependencies
###########################################################################

set ( Boost_LIBRARIES ) # Empty out the variable after previous use
set ( Boost_USE_DEBUG_PYTHON TRUE )
find_package ( Boost REQUIRED python )
add_definitions ( -DBOOST_DEBUG_PYTHON -DBOOST_PYTHON_NO_LIB )

find_package ( Numpy REQUIRED )
include_directories ( ${PYTHON_NUMPY_INCLUDE_DIR} )
include_directories ( inc )

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
# mantid package
###########################################################################
add_subdirectory ( mantid )

###########################################################################
# tests 
###########################################################################
# cxxtest unit tests
set ( TEST_FILES
  test/cpp/PythonObjectInstantiatorTest.h
)

if ( CXXTEST_FOUND )
  include_directories ( ../DataHandling/inc ../DataObjects/inc ../TestHelpers/inc )
  cxxtest_add_test ( PythonInterfaceTest ${TEST_FILES} )
  target_link_libraries( PythonInterfaceTest _kernel _api TestHelpers )
  # Specify implicit dependency, but don't link to it
  add_dependencies ( PythonInterfaceTest UserAlgorithms )
  add_dependencies ( FrameworkTests PythonInterfaceTest )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET PythonInterfaceTest PROPERTY FOLDER "UnitTests" )
endif ()

set ( TEST_PY_FILES
  test/python/AlgorithmManagerTest.py
)

# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test ( VanillaPythonInterfaceTest.py ${TEST_PY_FILES} )
  add_dependencies ( VanillaPythonInterfaceTest.py PythonInterface Algorithms )
  add_dependencies ( FrameworkTests VanillaPythonInterfaceTest.py )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET VanillaPythonInterfaceTest.py PROPERTY FOLDER "UnitTests" )
endif ()

