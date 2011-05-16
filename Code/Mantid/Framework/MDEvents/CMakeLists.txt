# Source files
set ( SRC_FILES
	src/BinToMDHistoWorkspace.cpp
	src/CoordTransform.cpp
	src/CoordTransformDistance.cpp
	src/CreateMDEventWorkspace.cpp
	src/FakeMDEventData.cpp
	src/IMDBox.cpp
	src/MDBin.cpp
	src/MDBox.cpp
	src/MDEWPeakIntegration.cpp
	src/MDEventFactory.cpp
	src/MDEventWorkspace.cpp
	src/MDGridBox.cpp
	src/MDHistoWorkspace.cpp
	src/MDSplitBox.cpp
	src/MakeDiffractionMDEventWorkspace.cpp
	src/OneStepMDEW.cpp
)

set ( SRC_UNITY_IGNORE_FILES 
    src/IMDBox.cpp
	src/MDBin.cpp
    src/MDBox.cpp
    src/MDEventWorkspace.cpp
    src/MDGridBox.cpp
    src/MDSplitBox.cpp
)

# Header files
set ( INC_FILES
	inc/MantidMDEvents/BinToMDHistoWorkspace.h
	inc/MantidMDEvents/BoxController.h
	inc/MantidMDEvents/CoordTransform.h
	inc/MantidMDEvents/CoordTransformDistance.h
	inc/MantidMDEvents/CreateMDEventWorkspace.h
	inc/MantidMDEvents/FakeMDEventData.h
	inc/MantidMDEvents/IMDBox.h
	inc/MantidMDEvents/MDBin.h
	inc/MantidMDEvents/MDBox.h
	inc/MantidMDEvents/MDDimensionExtents.h
	inc/MantidMDEvents/MDDimensionStats.h
	inc/MantidMDEvents/MDEWPeakIntegration.h
	inc/MantidMDEvents/MDEvent.h
	inc/MantidMDEvents/MDEventFactory.h
	inc/MantidMDEvents/MDEventWorkspace.h
	inc/MantidMDEvents/MDGridBox.h
	inc/MantidMDEvents/MDHistoWorkspace.h
	inc/MantidMDEvents/MDSplitBox.h
	inc/MantidMDEvents/MakeDiffractionMDEventWorkspace.h
	inc/MantidMDEvents/OneStepMDEW.h
)

# Test files. Other source files required.
set ( TEST_FILES
	test/BoxControllerTest.h
	test/CoordTransformDistanceTest.h
	test/CoordTransformTest.h
	test/CreateMDEventWorkspaceTest.h
	test/FakeMDEventDataTest.h
	test/IMDBoxTest.h
	test/MDBinTest.h
	test/MDBoxTest.h
	test/MDDimensionStatsTest.h
	test/MDEWPeakIntegrationTest.h
	test/MDEventFactoryTest.h
	test/MDEventTest.h
	test/MDEventWorkspaceTest.h
	test/MDGridBoxTest.h
	test/MDHistoWorkspaceTest.h
	test/MDSplitBoxTest.h
	test/MakeDiffractionMDEventWorkspaceTest.h
	test/OneStepMDEWTest.h
)

set ( GMOCK_TEST_FILES
	test/BinToMDHistoWorkspaceTest.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(MDEvents SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( MDEvents ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( MDEvents PROPERTIES OUTPUT_NAME MantidMDEvents 
                                                 COMPILE_DEFINITIONS IN_MANTID_MDEVENT )
                                                 
# Add to the 'Framework' group in VS
set_property ( TARGET MDEvents PROPERTY FOLDER "MantidFramework" )

include_directories(${HDF5_INCLUDE_DIRS} inc ../Algorithms/inc)
target_link_libraries ( MDEvents ${MANTIDLIBS} ${HDF5_LIBRARIES} Algorithms)

# Create test file projects
if ( CXXTEST_FOUND )
  include_directories(${HDF5_INCLUDE_DIRS} inc ../TestHelpers/inc 
      ../CurveFitting/inc ../DataHandling/inc ../Nexus/inc ../DataObjects/inc  )
  
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MDEventsTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
    target_link_libraries( MDEventsTest MDEvents ${MANTIDLIBS} ${HDF5_LIBRARIES}
         ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} DataHandling Nexus 
         TestHelpers DataObjects )
  else ()
    cxxtest_add_test ( MDEventsTest ${TEST_FILES} )
    target_link_libraries( MDEventsTest MDEvents ${MANTIDLIBS} ${HDF5_LIBRARIES}
         DataHandling Nexus TestHelpers DataObjects )
  endif ()
  
  add_dependencies ( FrameworkTests MDEventsTest )
  
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MDEventsTest PROPERTY FOLDER "UnitTests" )
  
endif ()

