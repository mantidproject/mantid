# Source files
set ( SRC_FILES
	#
	src/AffineMatrixParameter.cpp
	src/AffineMatrixParameterParser.cpp
	src/BinMD.cpp
	src/BinToMDHistoWorkspace.cpp
	src/BoxControllerSettingsAlgorithm.cpp
	src/CentroidPeaksMD.cpp
	src/CloneMDWorkspace.cpp
	src/ConvToMDBase.cpp
	src/ConvToMDEventsWS.cpp
	src/ConvToMDEventsHisto.cpp
	src/ConvToMDEventsSelector.cpp
	src/ConvToMDPreprocDet.cpp
	src/ConvertToDiffractionMDWorkspace.cpp
	src/ConvertToReflectometryQ.cpp
	src/CoordTransformAffine.cpp
	src/CoordTransformAffineParser.cpp
	src/CoordTransformAligned.cpp
	src/CoordTransformDistance.cpp
	src/CoordTransformDistanceParser.cpp
	src/CreateMDHistoWorkspace.cpp
	src/CreateMDWorkspace.cpp
	src/FakeMDEventData.cpp
	src/FindPeaksMD.cpp
	src/ImportMDHistoWorkspace.cpp
	src/ImportMDHistoWorkspaceBase.cpp
	src/IntegratePeaksMD.cpp
	src/LoadMD.cpp
	src/LoadSQW.cpp
	src/MDBin.cpp
	src/MDBox.cpp
	src/MDBoxBase.cpp
	src/MDBoxIterator.cpp
	src/MDEventFactory.cpp
	src/MDEventWSWrapper.cpp
	src/MDEventWorkspace.cpp
	src/MDGridBox.cpp
	src/MDHistoWorkspace.cpp
	src/MDHistoWorkspaceIterator.cpp
	src/MDLeanEvent.cpp
	src/MDTransfAxisNames.cpp
	src/MDTransfDEHelper.cpp
	src/MDTransfFactory.cpp
	src/MDTransfModQ.cpp
	src/MDTransfNoQ.cpp
	src/MDTransfQ3D.cpp
	src/MDWSDescription.cpp
	src/MDWSTransform.cpp
	src/OneStepMDEW.cpp
	src/QueryMDWorkspace.cpp
	src/ReflectometryTransformKiKf.cpp
	src/ReflectometryTransformP.cpp
	src/ReflectometryTransformQxQz.cpp
	src/SaveMD.cpp
	src/SliceMD.cpp
	src/SlicingAlgorithm.cpp
	src/UnitsConversionHelper.cpp
)

set ( SRC_UNITY_IGNORE_FILES 
    src/MDBoxBase.cpp
	src/MDBin.cpp
    src/MDBox.cpp
    src/MDBoxIterator.cpp
    src/MDEventWorkspace.cpp
    src/MDGridBox.cpp
)

# Header files
set ( INC_FILES
	#
	inc/MantidMDEvents/AffineMatrixParameter.h
	inc/MantidMDEvents/AffineMatrixParameterParser.h
	inc/MantidMDEvents/BinMD.h
	inc/MantidMDEvents/BinToMDHistoWorkspace.h
	inc/MantidMDEvents/BoxControllerSettingsAlgorithm.h
	inc/MantidMDEvents/CentroidPeaksMD.h
	inc/MantidMDEvents/CloneMDWorkspace.h
	inc/MantidMDEvents/ConvToMDBase.h
	inc/MantidMDEvents/ConvToMDEventsWS.h
	inc/MantidMDEvents/ConvToMDEventsHisto.h
	inc/MantidMDEvents/ConvToMDEventsSelector.h
	inc/MantidMDEvents/ConvToMDPreprocDet.h
	inc/MantidMDEvents/ConvertToDiffractionMDWorkspace.h
	inc/MantidMDEvents/ConvertToReflectometryQ.h
	inc/MantidMDEvents/CoordTransformAffine.h
	inc/MantidMDEvents/CoordTransformAffineParser.h
	inc/MantidMDEvents/CoordTransformAligned.h
	inc/MantidMDEvents/CoordTransformDistance.h
	inc/MantidMDEvents/CoordTransformDistanceParser.h
	inc/MantidMDEvents/CreateMDHistoWorkspace.h
	inc/MantidMDEvents/CreateMDWorkspace.h
	inc/MantidMDEvents/FakeMDEventData.h
	inc/MantidMDEvents/FindPeaksMD.h
	inc/MantidMDEvents/ImportMDHistoWorkspace.h
	inc/MantidMDEvents/ImportMDHistoWorkspaceBase.h
	inc/MantidMDEvents/IntegratePeaksMD.h
	inc/MantidMDEvents/LoadMD.h
	inc/MantidMDEvents/LoadSQW.h
	inc/MantidMDEvents/MDBin.h
	inc/MantidMDEvents/MDBox.h
	inc/MantidMDEvents/MDBoxBase.h
	inc/MantidMDEvents/MDBoxIterator.h
	inc/MantidMDEvents/MDDimensionStats.h
	inc/MantidMDEvents/MDEvent.h
	inc/MantidMDEvents/MDEventFactory.h
	inc/MantidMDEvents/MDEventWSWrapper.h
	inc/MantidMDEvents/MDEventWorkspace.h
	inc/MantidMDEvents/MDGridBox.h
	inc/MantidMDEvents/MDHistoWorkspace.h
	inc/MantidMDEvents/MDHistoWorkspaceIterator.h
	inc/MantidMDEvents/MDLeanEvent.h
	inc/MantidMDEvents/MDTransfAxisNames.h
	inc/MantidMDEvents/MDTransfDEHelper.h
	inc/MantidMDEvents/MDTransfFactory.h
	inc/MantidMDEvents/MDTransfInterface.h
	inc/MantidMDEvents/MDTransfModQ.h
	inc/MantidMDEvents/MDTransfNoQ.h
	inc/MantidMDEvents/MDTransfQ3D.h
	inc/MantidMDEvents/MDWSDescription.h
	inc/MantidMDEvents/MDWSTransform.h
	inc/MantidMDEvents/OneStepMDEW.h
	inc/MantidMDEvents/QueryMDWorkspace.h
	inc/MantidMDEvents/ReflectometryMDTransform.h
	inc/MantidMDEvents/ReflectometryTransformKiKf.h
	inc/MantidMDEvents/ReflectometryTransformP.h
	inc/MantidMDEvents/ReflectometryTransformQxQz.h
	inc/MantidMDEvents/SaveMD.h
	inc/MantidMDEvents/SkippingPolicy.h
	inc/MantidMDEvents/SliceMD.h
	inc/MantidMDEvents/SlicingAlgorithm.h
	inc/MantidMDEvents/UnitsConversionHelper.h
)

# Test files. Other source files required.
set ( TEST_FILES
	#
	test/AffineMatrixParameterParserTest.h
	test/AffineMatrixParameterTest.h
	test/BinToMDHistoWorkspaceTest.h
	test/BoxControllerSettingsAlgorithmTest.h
	test/CentroidPeaksMDTest.h
	test/CloneMDWorkspaceTest.h
	test/ConvertToDiffractionMDWorkspaceTest.h
	test/ConvertToMDEventsParamsTest.h
	test/ConvertToMDEventsSubalgFactoryTest.h
	test/ConvertToMDEventsUnitsConvTest.h
	test/ConvertToMDEventsWSTest.h
	test/ConvertToReflectometryQTest.h
	test/CoordTransformAffineParserTest.h
	test/CoordTransformAffineTest.h
	test/CoordTransformAlignedTest.h
	test/CoordTransformDistanceParserTest.h
	test/CoordTransformDistanceTest.h
	test/CreateMDHistoWorkspaceTest.h
	test/CreateMDWorkspaceTest.h
	test/FakeMDEventDataTest.h
	test/FindPeaksMDTest.h
	test/ImportMDHistoWorkspaceTest.h
	test/IntegratePeaksMDTest.h
	test/LoadMDTest.h
	test/LoadSQWTest.h
	test/MDBinTest.h
	test/MDBoxBaseTest.h
	test/MDBoxIteratorTest.h
	test/MDBoxTest.h
	test/MDDimensionStatsTest.h
	test/MDEventFactoryTest.h
	test/MDEventTest.h
	test/MDEventWSWrapperTest.h
	test/MDEventWorkspaceTest.h
	test/MDGridBoxTest.h
	test/MDHistoWorkspaceIteratorTest.h
	test/MDHistoWorkspaceTest.h
	test/MDLeanEventTest.h
	test/MDTransfAxisNamesTest.h
	test/MDWSTransfTest.h
	test/OneStepMDEWTest.h
	test/QueryMDWorkspaceTest.h
	test/ReflectometryTransformQxQzTest.h
	test/SaveMDTest.h
	test/SkippingPolicyTest.h
	test/SliceMDTest.h
	test/SlicingAlgorithmTest.h
)

set ( GMOCK_TEST_FILES
	test/BinMDTest.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(MDEvents SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidMDEvents/PrecompiledHeader.h MantidMDEvents src/PrecompiledHeader.cpp SRC_FILES INC_FILES )
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

include_directories( inc )
target_link_libraries ( MDEvents ${MANTIDLIBS} )

# Create test file projects
if ( CXXTEST_FOUND )
  include_directories ( ../TestHelpers/inc ../DataHandling/inc )
  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../TestHelpers/src/ComponentCreationHelper.cpp 
                        ../TestHelpers/src/WorkspaceCreationHelper.cpp
                        ../TestHelpers/src/MDEventsTestHelper.cpp )
  
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MDEventsTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
    target_link_libraries( MDEventsTest MDEvents ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} DataHandling )
  else ()
    cxxtest_add_test ( MDEventsTest ${TEST_FILES} )
    target_link_libraries( MDEventsTest MDEvents DataHandling)
  endif ()
  
  add_dependencies ( FrameworkTests MDEventsTest )
  
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MDEventsTest PROPERTY FOLDER "UnitTests" )
  
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MDEvents ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
