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
	src/ConvToMDHistoWS.cpp
	src/ConvToMDPreprocDet.cpp
	src/ConvToMDSelector.cpp
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
	src/FitMD.cpp
	src/ImportMDEventWorkspace.cpp
	src/ImportMDHistoWorkspace.cpp
	src/ImportMDHistoWorkspaceBase.cpp
	src/IntegratePeaksMD.cpp
	src/LoadMD.cpp
	src/LoadSQW.cpp
	src/MDBin.cpp
	src/MDBox.cpp
	src/MDBoxToChange.cpp
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
	src/UserFunctionMD.cpp
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
	inc/MantidMDEvents/ConvToMDHistoWS.h
	inc/MantidMDEvents/ConvToMDPreprocDet.h
	inc/MantidMDEvents/ConvToMDSelector.h
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
	inc/MantidMDEvents/FitMD.h
	inc/MantidMDEvents/ImportMDEventWorkspace.h
	inc/MantidMDEvents/ImportMDHistoWorkspace.h
	inc/MantidMDEvents/ImportMDHistoWorkspaceBase.h
	inc/MantidMDEvents/IntegratePeaksMD.h
	inc/MantidMDEvents/LoadMD.h
	inc/MantidMDEvents/LoadSQW.h
	inc/MantidMDEvents/MDBin.h
	inc/MantidMDEvents/MDBox.h
	inc/MantidMDEvents/MDBoxToChange.h	
	inc/MantidMDEvents/BoxCtrlChangesList.h	
	inc/MantidMDEvents/MDBoxBase.h
	inc/MantidMDEvents/MDBoxIterator.h
	inc/MantidMDEvents/MDDimensionStats.h
	inc/MantidMDEvents/MDEvent.h
	inc/MantidMDEvents/MDEventFactory.h
	inc/MantidMDEvents/MDEventInserter.h
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
	inc/MantidMDEvents/UserFunctionMD.h
)

# Test files.
set ( TEST_FILES
	AffineMatrixParameterParserTest.h
	AffineMatrixParameterTest.h
	BinToMDHistoWorkspaceTest.h
	BoxControllerSettingsAlgorithmTest.h
	CentroidPeaksMDTest.h
	CloneMDWorkspaceTest.h
	ConvertToDiffractionMDWorkspaceTest.h
	ConvertToReflectometryQTest.h
	ConvToMDBaseTest.h
	ConvToMDEventsVSHistoTest.h		
	ConvToMDUnitsConversionHelperTest.h	
	CoordTransformAffineParserTest.h
	CoordTransformAffineTest.h
	CoordTransformAlignedTest.h
	CoordTransformDistanceParserTest.h
	CoordTransformDistanceTest.h
	CreateMDHistoWorkspaceTest.h
	CreateMDWorkspaceTest.h
	FakeMDEventDataTest.h
	FindPeaksMDTest.h
	FitMDTest.h
	ImportMDEventWorkspaceTest.h
	ImportMDHistoWorkspaceTest.h
	IntegratePeaksMDTest.h
	LoadMDTest.h
	LoadSQWTest.h
	MDBinTest.h
	MDBoxBaseTest.h
	MDBoxIteratorTest.h
	MDBoxTest.h
	MDBoxToChangeTest.h
	MDDimensionStatsTest.h
	MDEventFactoryTest.h
	MDEventInserterTest.h
	MDEventTest.h
	MDEventWSWrapperTest.h
	MDEventWorkspaceTest.h
	MDGridBoxTest.h
	MDHistoWorkspaceIteratorTest.h
	MDHistoWorkspaceTest.h
	MDLeanEventTest.h
	MDTransfAxisNamesTest.h
	MDWSTransfTest.h
	MDTransfFactoryTest.h		
	MDWSDescriptionTest.h	
	OneStepMDEWTest.h
	QueryMDWorkspaceTest.h
	ReflectometryTransformQxQzTest.h
	SaveMDTest.h
	SkippingPolicyTest.h
	SliceMDTest.h
	SlicingAlgorithmTest.h
)

set ( GMOCK_TEST_FILES
	BinMDTest.h
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

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MDEvents ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
