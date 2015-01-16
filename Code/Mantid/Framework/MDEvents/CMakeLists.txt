# Source files
set ( SRC_FILES
	#
	src/AffineMatrixParameter.cpp
	src/AffineMatrixParameterParser.cpp
	src/BoxControllerNeXusIO.cpp
	src/BoxControllerSettingsAlgorithm.cpp
        src/CalculateReflectometryQBase.cpp  
	src/ConvToMDBase.cpp
	src/ConvToMDEventsWS.cpp
	src/ConvToMDHistoWS.cpp
	src/ConvToMDSelector.cpp 
	src/ConvertToReflectometryQ.cpp
	src/CoordTransformAffine.cpp
	src/CoordTransformAffineParser.cpp
	src/CoordTransformAligned.cpp
	src/CoordTransformDistance.cpp
	src/CoordTransformDistanceParser.cpp
	src/FitMD.cpp
	src/ImportMDEventWorkspace.cpp
	src/ImportMDHistoWorkspace.cpp
	src/ImportMDHistoWorkspaceBase.cpp
	src/MDBin.cpp
	src/MDBox.cpp
	src/MDBoxBase.cpp
	src/MDBoxIterator.cpp
    src/MDBoxFlatTree.cpp    
	src/MDBoxSaveable.cpp    
	src/MDEventFactory.cpp
	src/MDEventWSWrapper.cpp
	src/MDEventWorkspace.cpp
	src/MDGridBox.cpp
	src/MDHistoWorkspace.cpp
	src/MDHistoWorkspaceIterator.cpp
	src/MDLeanEvent.cpp
	src/MDTransfAxisNames.cpp
	src/MDTransfFactory.cpp
	src/MDTransfModQ.cpp
	src/MDTransfNoQ.cpp
	src/MDTransfQ3D.cpp
	src/MDWSDescription.cpp
	src/MDWSTransform.cpp
	src/OneStepMDEW.cpp
	src/QueryMDWorkspace.cpp
	src/ReflectometryTransform.cpp
	src/ReflectometryTransformKiKf.cpp
	src/ReflectometryTransformP.cpp
	src/ReflectometryTransformQxQz.cpp
    src/Integrate3DEvents.cpp
    src/IntegrateEllipsoids.cpp
	src/SaveIsawQvector.cpp
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
	inc/MantidMDEvents/BoxControllerNeXusIO.h        
	inc/MantidMDEvents/BoxControllerSettingsAlgorithm.h
        inc/MantidMDEvents/CalculateReflectometryQBase.h
	inc/MantidMDEvents/ConvToMDBase.h
	inc/MantidMDEvents/ConvToMDEventsWS.h
	inc/MantidMDEvents/ConvToMDHistoWS.h
	inc/MantidMDEvents/ConvToMDSelector.h
	inc/MantidMDEvents/ConvertToReflectometryQ.h
	inc/MantidMDEvents/CoordTransformAffine.h
	inc/MantidMDEvents/CoordTransformAffineParser.h
	inc/MantidMDEvents/CoordTransformAligned.h
	inc/MantidMDEvents/CoordTransformDistance.h
	inc/MantidMDEvents/CoordTransformDistanceParser.h
	inc/MantidMDEvents/FitMD.h
	inc/MantidMDEvents/ImportMDEventWorkspace.h
	inc/MantidMDEvents/ImportMDHistoWorkspace.h
	inc/MantidMDEvents/ImportMDHistoWorkspaceBase.h
	inc/MantidMDEvents/MDBin.h
	inc/MantidMDEvents/MDBox.h
	inc/MantidMDEvents/MDBoxBase.h
	inc/MantidMDEvents/MDBoxIterator.h
    inc/MantidMDEvents/MDBoxFlatTree.h
	inc/MantidMDEvents/MDBoxSaveable.h
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
	inc/MantidMDEvents/MDTransfFactory.h
	inc/MantidMDEvents/MDTransfInterface.h
	inc/MantidMDEvents/MDTransfModQ.h
	inc/MantidMDEvents/MDTransfNoQ.h
	inc/MantidMDEvents/MDTransfQ3D.h
	inc/MantidMDEvents/MDWSDescription.h
	inc/MantidMDEvents/MDWSTransform.h
	inc/MantidMDEvents/OneStepMDEW.h
	inc/MantidMDEvents/QueryMDWorkspace.h
	inc/MantidMDEvents/ReflectometryTransform.h
	inc/MantidMDEvents/ReflectometryTransformKiKf.h
	inc/MantidMDEvents/ReflectometryTransformP.h
	inc/MantidMDEvents/ReflectometryTransformQxQz.h
        inc/MantidMDEvents/Integrate3DEvents.h
        inc/MantidMDEvents/IntegrateEllipsoids.h
	inc/MantidMDEvents/SaveIsawQvector.h
	inc/MantidMDEvents/SkippingPolicy.h
	inc/MantidMDEvents/UnitsConversionHelper.h
	inc/MantidMDEvents/UserFunctionMD.h
)

# Test files.
set ( TEST_FILES
	AffineMatrixParameterParserTest.h
	AffineMatrixParameterTest.h
	BoxControllerNeXusIOTest.h                
	BoxControllerSettingsAlgorithmTest.h
	ConvertToReflectometryQTest.h
	CoordTransformAffineParserTest.h
	CoordTransformAffineTest.h
	CoordTransformAlignedTest.h
	CoordTransformDistanceParserTest.h
	CoordTransformDistanceTest.h
	FitMDTest.h
	ImportMDEventWorkspaceTest.h
	ImportMDHistoWorkspaceTest.h
	MDBinTest.h
	MDBoxBaseTest.h
	MDBoxIteratorTest.h
    MDBoxFlatTreeTest.h     
	MDBoxTest.h
	MDBoxSaveableTest.h    
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
	MDTransfFactoryTest.h
	MDTransfQ3DTest.h
	MDTransfModQTest.h    
	MDWSDescriptionTest.h
	MDWSTransfTest.h
	OneStepMDEWTest.h
	QueryMDWorkspaceTest.h
	ReflectometryTransformQxQzTest.h
	ReflectometryTransformPTest.h
	ReflectometryTransformKiKfTest.h	
    Integrate3DEventsTest.h
	SaveIsawQvectorTest.h
	SkippingPolicyTest.h
	UnitsConversionHelperTest.h
)


if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(MDEvents SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidMDEvents/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( MDEvents ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( MDEvents PROPERTIES OUTPUT_NAME MantidMDEvents 
                                                 COMPILE_DEFINITIONS IN_MANTID_MDEVENT 
                                                 INSTALL_RPATH "@loader_path/../MacOS")

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
