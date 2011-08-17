# Source files
set ( SRC_FILES
	src/AffineMatrixParameter.cpp
	src/AffineMatrixParameterParser.cpp
	src/BinToMDHistoWorkspace.cpp
	src/BoxController.cpp
	src/CloneMDEventWorkspace.cpp
	src/CoordTransform.cpp
	src/CoordTransformDistance.cpp
	src/CoordTransformDistanceParser.cpp
	src/CoordTransformParser.cpp
	src/CreateMDEventWorkspace.cpp
	src/FakeMDEventData.cpp
	src/IMDBox.cpp
	src/LoadMDEW.cpp
	src/LoadSQW.cpp
	src/MDBin.cpp
	src/MDBox.cpp
	src/MDBoxIterator.cpp
	src/MDCentroidPeaks.cpp
	src/MDEWFindPeaks.cpp
	src/MDEWPeakIntegration.cpp
	src/MDEventFactory.cpp
	src/MDEventWorkspace.cpp
	src/MDGridBox.cpp
	src/MDHistoWorkspace.cpp
	src/MDLeanEvent.cpp
	src/MakeDiffractionMDEventWorkspace.cpp
	src/MergeMDEW.cpp
	src/OneStepMDEW.cpp
	src/PlusMDEW.cpp
	src/SaveMDEW.cpp
)

set ( SRC_UNITY_IGNORE_FILES 
    src/IMDBox.cpp
	src/MDBin.cpp
    src/MDBox.cpp
    src/MDBoxIterator.cpp
    src/MDEventWorkspace.cpp
    src/MDGridBox.cpp
)

# Header files
set ( INC_FILES
	inc/MantidMDEvents/AffineMatrixParameter.h
	inc/MantidMDEvents/AffineMatrixParameterParser.h
	inc/MantidMDEvents/BinToMDHistoWorkspace.h
	inc/MantidMDEvents/BoxController.h
	inc/MantidMDEvents/CloneMDEventWorkspace.h
	inc/MantidMDEvents/CoordTransform.h
	inc/MantidMDEvents/CoordTransformDistance.h
	inc/MantidMDEvents/CoordTransformDistanceParser.h
	inc/MantidMDEvents/CoordTransformParser.h
	inc/MantidMDEvents/CreateMDEventWorkspace.h
	inc/MantidMDEvents/FakeMDEventData.h
	inc/MantidMDEvents/IMDBox.h
	inc/MantidMDEvents/LoadMDEW.h
	inc/MantidMDEvents/LoadSQW.h
	inc/MantidMDEvents/MDBin.h
	inc/MantidMDEvents/MDBox.h
	inc/MantidMDEvents/MDBoxIterator.h
	inc/MantidMDEvents/MDCentroidPeaks.h
	inc/MantidMDEvents/MDDimensionStats.h
	inc/MantidMDEvents/MDEWFindPeaks.h
	inc/MantidMDEvents/MDEWPeakIntegration.h
	inc/MantidMDEvents/MDEventFactory.h
	inc/MantidMDEvents/MDEventWorkspace.h
	inc/MantidMDEvents/MDGridBox.h
	inc/MantidMDEvents/MDHistoWorkspace.h
	inc/MantidMDEvents/MDLeanEvent.h
	inc/MantidMDEvents/MakeDiffractionMDEventWorkspace.h
	inc/MantidMDEvents/MergeMDEW.h
	inc/MantidMDEvents/OneStepMDEW.h
	inc/MantidMDEvents/PlusMDEW.h
	inc/MantidMDEvents/SaveMDEW.h
)

# Test files. Other source files required.
set ( TEST_FILES
	test/AffineMatrixParameterParserTest.h
	test/AffineMatrixParameterTest.h
	test/BoxControllerTest.h
	test/CloneMDEventWorkspaceTest.h
	test/CoordTransformDistanceParserTest.h
	test/CoordTransformDistanceTest.h
	test/CoordTransformParserTest.h
	test/CoordTransformTest.h
	test/CreateMDEventWorkspaceTest.h
	test/FakeMDEventDataTest.h
	test/IMDBoxTest.h
	test/LoadMDEWTest.h
	test/LoadSQWTest.h
	test/MDBinTest.h
	test/MDBoxIteratorTest.h
	test/MDBoxTest.h
	test/MDCentroidPeaksTest.h
	test/MDDimensionStatsTest.h
	test/MDEWFindPeaksTest.h
	test/MDEWPeakIntegrationTest.h
	test/MDEventFactoryTest.h
	test/MDEventTest.h
	test/MDEventWorkspaceTest.h
	test/MDGridBoxTest.h
	test/MDHistoWorkspaceTest.h
	test/MDLeanEventTest.h
	test/MakeDiffractionMDEventWorkspaceTest.h
	test/MergeMDEWTest.h
	test/OneStepMDEWTest.h
	test/PlusMDEWTest.h
	test/SaveMDEWTest.h
)

set ( GMOCK_TEST_FILES
	test/BinToMDHistoWorkspaceTest.h
)

# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidMDEvents/PrecompiledHeader.h MantidMDEvents MDEvents SRC_FILES INC_FILES )

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(MDEvents SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( MDEvents ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( MDEvents PROPERTIES OUTPUT_NAME MantidMDEvents 
                                                 COMPILE_DEFINITIONS IN_MANTID_MDEVENT )
# Intensive use of templated libaries can cause large objects to be generated. These require
# an additional flag in MSVC.
if ( MSVC )
  set_target_properties ( MDEvents PROPERTIES COMPILE_FLAGS "/bigobj" )
endif ()
                                                 
# Add to the 'Framework' group in VS
set_property ( TARGET MDEvents PROPERTY FOLDER "MantidFramework" )

include_directories( inc  ../Nexus/inc ../API/inc )
target_link_libraries ( MDEvents ${MANTIDLIBS} Nexus )

# Create test file projects
if ( CXXTEST_FOUND )
  include_directories( inc ../TestHelpers/inc 
      ../CurveFitting/inc ../DataHandling/inc ../Nexus/inc ../DataObjects/inc  )
  
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MDEventsTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
    target_link_libraries( MDEventsTest MDEvents ${MANTIDLIBS}
         ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} DataHandling Nexus 
         TestHelpers DataObjects API)
  else ()
    cxxtest_add_test ( MDEventsTest ${TEST_FILES} )
    target_link_libraries( MDEventsTest MDEvents ${MANTIDLIBS}
         DataHandling Nexus TestHelpers )
  endif ()
  
  add_dependencies ( FrameworkTests MDEventsTest )
  
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MDEventsTest PROPERTY FOLDER "UnitTests" )
  
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MDEvents DESTINATION ${LIB_DIR} )
