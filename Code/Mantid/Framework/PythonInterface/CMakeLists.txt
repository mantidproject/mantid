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

find_package ( Numpy REQUIRED )
include_directories ( SYSTEM ${PYTHON_NUMPY_INCLUDE_DIR} )
set ( HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidPythonInterface )
include_directories ( inc )


if ( UNIX )
  ###########################################################################
  # In order to keep the type registries for the two Python APIs separate
  # we need to have a separate copy in each library. Once the old
  # API is removed this will be deleted and we can link back to the dynamic
  # library. This is only required on Unix like environments
  ###########################################################################
  set ( BOOST_PYTHON_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../PythonAPI/src/boostpython.cpp )
  include_directories ( SYSTEM ../PythonAPI/inc )
  
  add_definitions ( -DBOOST_DEBUG_PYTHON -DBOOST_PYTHON_NO_LIB -DBOOST_PYTHON_STATIC_LIB -DBOOST_PYTHON_SOURCE )

  # On some Linux systems (seen on various Ubuntu versions) importing Mantid into a standalone python
  # interpreter causes a segfault. It is some issue due to exception handling but the fix is 
  # to ensure that the stdc++ library appears as early in the link list as possible so that it
  # is loaded first, hence the hard coding of it here rather than leaving it to be implicitly defined
  # by the linker.
  # 
  # MG 2011/11/15: A similar issue regarding the Nexus library has now been observed. Nexus 4.3 introduced
  # thread-local variables and this now causes problems whenever we try and load the library from
  # python if other libraries appear before it in the link list.
  set ( PYTHON_DEPS stdc++ ${NEXUS_C_LIBRARIES} ${MANTIDLIBS} ${PYTHON_LIBRARIES}  )
else ()
  add_definitions ( -DBOOST_DEBUG_PYTHON -DBOOST_PYTHON_NO_LIB )
  set ( BOOST_PYTHON_SRC ) # Not necessary on Windows
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
  test/cpp/PropertyWithValueFactoryTest.h
  test/cpp/PythonObjectInstantiatorTest.h
  test/cpp/PySequenceToVectorTest.h
)  
  
if ( CXXTEST_FOUND )
  cxxtest_add_test ( PythonInterfaceTest ${TEST_FILES} )
  if ( WIN32 )
    set_target_properties( PythonInterfaceTest PROPERTIES COMPILE_FLAGS "/w44244" )
  endif ()
  target_link_libraries( PythonInterfaceTest PythonKernelModule PythonAPIModule )
  add_dependencies ( FrameworkTests PythonInterfaceTest )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET PythonInterfaceTest PROPERTY FOLDER "UnitTests" )
endif ()

# python unit tests
set ( TEST_PY_FILES
  test/python/AlgorithmTest.py
  test/python/AlgorithmFactoryTest.py
  test/python/AlgorithmManagerTest.py
  test/python/AnalysisDataServiceTest.py
  test/python/ArrayPropertyTest.py
  test/python/ArrayBoundedValidatorTest.py
  test/python/ArrayLengthValidatorTest.py
  test/python/AxisTest.py
  test/python/BoundedValidatorTest.py
  test/python/CompositeValidatorTest.py
  test/python/ConfigServiceTest.py
  test/python/CreateWorkspaceTest.py
  test/python/DeltaEModeTest.py
  test/python/ExperimentInfoTest.py
  test/python/FacilityInfoTest.py
  test/python/FilePropertyTest.py
  test/python/FileFinderTest.py
  test/python/FilteredTimeSeriesPropertyTest.py
  test/python/FindReflectometryLinesTest.py
  test/python/FrameworkManagerTest.py
  test/python/IComponentTest.py
  test/python/IEventWorkspaceTest.py
  test/python/ImportModuleTest.py
  test/python/IPeaksWorkspaceTest.py
  test/python/InstrumentInfoTest.py
  test/python/InstrumentTest.py
  test/python/ITableWorkspaceTest.py
  test/python/ListValidatorTest.py
  test/python/LogFilterTest.py
  test/python/LoggerTest.py
  test/python/MatrixWorkspaceTest.py
  test/python/MandatoryValidatorTest.py
  test/python/MDHistoWorkspaceTest.py
  test/python/MultipleFilePropertyTest.py
  test/python/NullValidatorTest.py
  test/python/OrientedLatticeTest.py
  test/python/PythonPluginsTest.py
  test/python/PropertyWithValueTest.py
  test/python/PythonAlgorithmPropertiesTest.py
  test/python/PythonAlgorithmTraitsTest.py
  test/python/PythonAlgorithmWorkspacePropertyTest.py
  test/python/ReferenceFrameTest.py
  test/python/RetrieveRunInfoTest.py
  test/python/RunTest.py
  test/python/SimpleAPITest.py
  test/python/SimpleAPILoadTest.py
  test/python/SimpleAPIFitTest.py
  test/python/Stitch1DTest.py
  test/python/TimeSeriesPropertyTest.py
  test/python/QuatTest.py
  test/python/V3DTest.py
  test/python/UnitCellTest.py
  test/python/UnitConversionTest.py
  test/python/UnitFactoryTest.py
  test/python/WorkspaceFactoryTest.py
  test/python/WorkspaceTest.py
  test/python/WorkspaceGroupTest.py
  test/python/WorkspacePropertiesTest.py
  test/python/WorkspaceValidatorsTest.py
)

set ( PYTEST_HELPERS test/python/testhelpers.py )

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

###########################################################################
# Installation settings
###########################################################################

# Regex matches any .py or .pyc file. The $ ensures that this is the last 
# part of the string
install ( DIRECTORY mantid DESTINATION ${BIN_DIR} 
          FILES_MATCHING REGEX ".*.pyc?$"
          PATTERN "src" EXCLUDE )
