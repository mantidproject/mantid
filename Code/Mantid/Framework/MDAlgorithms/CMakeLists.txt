# GLOBs should be replaced with explicit listings
set ( SRC_FILES
	# src/CreateMDFitWorkspace.cpp
	src/AndMD.cpp
	src/BinaryOperationMD.cpp
	src/BinMD.cpp		
	src/BinToMDHistoWorkspace.cpp	
	src/BooleanBinaryOperationMD.cpp
	src/CentroidPeaksMD.cpp    
	src/CloneMDWorkspace.cpp	
	src/CobaltSpinWaveDSHO.cpp
	src/CompareMDWorkspaces.cpp
	src/ConvertToDetectorFaceMD.cpp
	src/ConvertToMD.cpp
	src/CreateMDHistoWorkspace.cpp
	src/CreateMDWorkspace.cpp	
	src/DampedHisenbergFMSW.cpp
	src/DivideMD.cpp
	src/EqualToMD.cpp
	src/ExponentialMD.cpp
	src/FakeMDEventData.cpp    
	src/FindPeaksMD.cpp    
	src/GreaterThanMD.cpp
	src/IDynamicRebinning.cpp
	src/IntegratePeaksMD.cpp        
	src/InvalidParameter.cpp
	src/InvalidParameterParser.cpp
	src/LessThanMD.cpp
	src/LoadMD.cpp
	src/LoadSQW.cpp
	src/LogarithmMD.cpp
	src/MagneticFormFactor.cpp
	src/MaskMD.cpp
	src/MergeMD.cpp
	src/MergeMDFiles.cpp
	src/MinusMD.cpp
	src/MultiplyMD.cpp
	src/NotMD.cpp
	src/OrMD.cpp
	src/PlusMD.cpp
	src/PowerMD.cpp
    src/PreprocessDetectorsToMD.cpp
	src/QuadEnBackground.cpp
	src/Quantification/CachedExperimentInfo.cpp
	src/Quantification/FitResolutionConvolvedModel.cpp
	src/Quantification/ForegroundModel.cpp
	src/Quantification/ForegroundModelFactory.cpp
	src/Quantification/MDResolutionConvolution.cpp
	src/Quantification/MDResolutionConvolutionFactory.cpp
	src/Quantification/Models/Strontium122.cpp
	src/Quantification/Resolution/ModeratorChopperResolution.cpp
	src/Quantification/Resolution/TobyFitBMatrix.cpp
	src/Quantification/Resolution/TobyFitResolutionModel.cpp
	src/Quantification/Resolution/TobyFitYVector.cpp
	src/Quantification/ResolutionConvolvedCrossSection.cpp
	src/Quantification/SimulateResolutionConvolvedModel.cpp
	src/RunParam.cpp
	src/SaveZODS.cpp
	src/SetMDUsingMask.cpp
	src/SimulateMDD.cpp
	src/SimulateResolution.cpp
	src/SliceMD.cpp	
	src/TobyFitSimulate.cpp
	src/TransformMD.cpp
	src/UnaryOperationMD.cpp
	src/Vector3DParameterParser.cpp
	src/WeightedMeanMD.cpp
	src/XorMD.cpp
# Old TMP convertToMD code prepared for deprecation:	
	src/ConvertToMDEvents.cpp	
	src/ConvertToMDEventsParams.cpp
	src/ConvertToMDEventsSubalgFactory.cpp
	src/ConvertToMDEventsWSBase.cpp
#end TMP
	src/SaveMD.cpp	
	src/SlicingAlgorithm.cpp
	src/Stitch1DMD.cpp
)

#set ( SRC_UNITY_IGNORE_FILES src/IDynamicRebinning.cpp
#)

set ( INC_FILES
	# inc/MantidMDAlgorithms/CreateMDFitWorkspace.h
	inc/MantidMDAlgorithms/AndMD.h
	inc/MantidMDAlgorithms/BinaryOperationMD.h
	inc/MantidMDAlgorithms/BinMD.h	
	inc/MantidMDAlgorithms/BinToMDHistoWorkspace.h	
	inc/MantidMDAlgorithms/BooleanBinaryOperationMD.h
	inc/MantidMDAlgorithms/CentroidPeaksMD.h    
	inc/MantidMDAlgorithms/CloneMDWorkspace.h	
	inc/MantidMDAlgorithms/CobaltSpinWaveDSHO.h
	inc/MantidMDAlgorithms/CompareMDWorkspaces.h
	inc/MantidMDAlgorithms/ConvertToDetectorFaceMD.h
	inc/MantidMDAlgorithms/ConvertToMD.h
	inc/MantidMDAlgorithms/CreateMDHistoWorkspace.h
	inc/MantidMDAlgorithms/CreateMDWorkspace.h	
	inc/MantidMDAlgorithms/DampedHisenbergFMSW.h
	inc/MantidMDAlgorithms/DivideMD.h
	inc/MantidMDAlgorithms/DllConfig.h
	inc/MantidMDAlgorithms/EqualToMD.h
	inc/MantidMDAlgorithms/ExponentialMD.h
	inc/MantidMDAlgorithms/FakeMDEventData.h    
	inc/MantidMDAlgorithms/FindPeaksMD.h    
	inc/MantidMDAlgorithms/GreaterThanMD.h
	inc/MantidMDAlgorithms/IDynamicRebinning.h
	inc/MantidMDAlgorithms/IntegratePeaksMD.h    
	inc/MantidMDAlgorithms/InvalidParameter.h
	inc/MantidMDAlgorithms/InvalidParameterParser.h
	inc/MantidMDAlgorithms/LessThanMD.h
	inc/MantidMDAlgorithms/LoadMD.h
	inc/MantidMDAlgorithms/LoadSQW.h
	inc/MantidMDAlgorithms/LogarithmMD.h
	inc/MantidMDAlgorithms/MagneticFormFactor.h
	inc/MantidMDAlgorithms/MaskMD.h
	inc/MantidMDAlgorithms/MergeMD.h
	inc/MantidMDAlgorithms/MergeMDFiles.h
	inc/MantidMDAlgorithms/MinusMD.h
	inc/MantidMDAlgorithms/MultiplyMD.h
	inc/MantidMDAlgorithms/NotMD.h
	inc/MantidMDAlgorithms/OrMD.h
	inc/MantidMDAlgorithms/PlusMD.h
	inc/MantidMDAlgorithms/PowerMD.h
    inc/MantidMDAlgorithms/PreprocessDetectorsToMD.h	
	inc/MantidMDAlgorithms/QuadEnBackground.h
	inc/MantidMDAlgorithms/Quantification/CachedExperimentInfo.h
	inc/MantidMDAlgorithms/Quantification/FitResolutionConvolvedModel.h
	inc/MantidMDAlgorithms/Quantification/ForegroundModel.h
	inc/MantidMDAlgorithms/Quantification/ForegroundModelFactory.h
	inc/MantidMDAlgorithms/Quantification/MDResolutionConvolution.h
	inc/MantidMDAlgorithms/Quantification/MDResolutionConvolutionFactory.h
	inc/MantidMDAlgorithms/Quantification/Models/Strontium122.h
	inc/MantidMDAlgorithms/Quantification/Resolution/ModeratorChopperResolution.h
	inc/MantidMDAlgorithms/Quantification/Resolution/TobyFitBMatrix.h
	inc/MantidMDAlgorithms/Quantification/Resolution/TobyFitResolutionModel.h
	inc/MantidMDAlgorithms/Quantification/Resolution/TobyFitYVector.h
	inc/MantidMDAlgorithms/Quantification/ResolutionConvolvedCrossSection.h
	inc/MantidMDAlgorithms/Quantification/SimulateResolutionConvolvedModel.h
	inc/MantidMDAlgorithms/RunParam.h
	inc/MantidMDAlgorithms/SaveMD.h	
	inc/MantidMDAlgorithms/SaveZODS.h
	inc/MantidMDAlgorithms/SetMDUsingMask.h
	inc/MantidMDAlgorithms/SimulateMDD.h
	inc/MantidMDAlgorithms/SimulateResolution.h
	inc/MantidMDAlgorithms/SliceMD.h	
	inc/MantidMDAlgorithms/TobyFitSimulate.h
	inc/MantidMDAlgorithms/TransformMD.h
	inc/MantidMDAlgorithms/UnaryOperationMD.h
	inc/MantidMDAlgorithms/Vector3DParameter.h
	inc/MantidMDAlgorithms/Vector3DParameterParser.h
	inc/MantidMDAlgorithms/WeightedMeanMD.h
	inc/MantidMDAlgorithms/XorMD.h
# Old TMP convertToMD code prepared for deprication:
    inc/MantidMDAlgorithms/ConvertToMDEvents.h
	inc/MantidMDAlgorithms/ConvertToMDEventsParams.h 
	inc/MantidMDAlgorithms/ConvertToMDEventsSubalgFactory.h 	
	inc/MantidMDAlgorithms/ConvertToMDEventsEventWS.h
	inc/MantidMDAlgorithms/ConvertToMDEventsHistoWS.h
	inc/MantidMDAlgorithms/ConvertToMDEventsParams.h
	inc/MantidMDAlgorithms/ConvertToMDEventsSubalgFactory.h
	inc/MantidMDAlgorithms/ConvertToMDEventsTransfInterface.h
	inc/MantidMDAlgorithms/ConvertToMDEventsTransfModQ.h
	inc/MantidMDAlgorithms/ConvertToMDEventsTransfNoQ.h
	inc/MantidMDAlgorithms/ConvertToMDEventsTransfQ3D.h
	inc/MantidMDAlgorithms/ConvertToMDEventsUnitsConv.h
	inc/MantidMDAlgorithms/ConvertToMDEventsWSInterface.h	
	inc/MantidMDAlgorithms/ConvertToMDEventsWSBase.h	
# END TMP	
	inc/MantidMDAlgorithms/SlicingAlgorithm.h
        inc/MantidMDAlgorithms/Stitch1DMD.h
)

# Test files. Other source files required.
set ( TEST_FILES
	#
	#	ConvertToMDEventsCoordTransfTest.h
	# these tests are as they test verify different parts of the CPR algorithms
	#CreateMDFitWorkspaceTest.h
	AndMDTest.h
	BinToMDHistoWorkspaceTest.h	
	BooleanBinaryOperationMDTest.h
	CachedExperimentInfoTest.h
	CentroidPeaksMDTest.h    
	CloneMDWorkspaceTest.h	
	CobaltSWDTest.h
	CompareMDWorkspacesTest.h
	ConvertEventsToMDTest.h
	ConvertToDetectorFaceMDTest.h
	ConvertToMDComponentsTest.h	
	ConvertToMDTest.h
	ConvertToQ3DdETest.h
	CreateMDHistoWorkspaceTest.h
	CreateMDWorkspaceTest.h
	DampedHisenbergFMSWTest.h
	DivideMDTest.h
	EqualToMDTest.h
	ExponentialMDTest.h
	FakeMDEventDataTest.h
	FindPeaksMDTest.h    
	FitResolutionConvolvedModelTest.h
	ForegroundModelTest.h
	GreaterThanMDTest.h
	IntegratePeaksMDTest.h    
	InvalidParameterParserTest.h
	InvalidParameterTest.h
	LessThanMDTest.h
	LoadMDTest.h
	LoadSQWTest.h
	LogarithmMDTest.h
	MDResolutionConvolutionFactoryTest.h
	MagneticFormFactorTest.h
	MaskMDTest.h
	MergeMDFilesTest.h
	MergeMDTest.h
	MinusMDTest.h
	ModeratorChopperResolutionTest.h
	MultiplyMDTest.h
	NotMDTest.h
	OrMDTest.h
	PlusMDTest.h
	PowerMDTest.h
	PreprocessDetectorsToMDTest.h	
	QuadEnBackgroundTest.h
	ResolutionConvolvedCrossSectionTest.h
	RunParamTest.h
	SaveMDTest.h	
	SaveZODSTest.h
	SetMDUsingMaskTest.h
	SimulateMDDTest.h
	SimulateResolutionConvolvedModelTest.h
	SimulateResolutionTest.h
	SliceMDTest.h	
	Strontium122Test.h
	TobyFitBMatrixTest.h
	TobyFitResolutionModelTest.h
	TobyFitSimulateTest.h
	TobyFitYVectorTest.h
	TransformMDTest.h
	UnaryOperationMDTest.h
	WeightedMeanMDTest.h
	XorMDTest.h
        SlicingAlgorithmTest.h	
        Stitch1DMDTest.h
# Old TMD convertToMD code prepared for deprication:
#    ConvertToMDEventsUnitsConvTest.h 
)

set ( GMOCK_TEST_FILES
	BinaryOperationMDTest.h
	BinMDTest.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(MDAlgorithms SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidMDAlgorithms/PrecompiledHeader.h MantidMDAlgorithms src/PrecompiledHeader.cpp SRC_FILES INC_FILES )
# Add the target for this directory
add_library ( MDAlgorithms ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( MDAlgorithms PROPERTIES OUTPUT_NAME MantidMDAlgorithms 
                                                COMPILE_DEFINITIONS IN_MANTID_MDALGORITHMS )
# Add to the 'Framework' group in VS
set_property ( TARGET MDAlgorithms PROPERTY FOLDER "MantidFramework" )

include_directories ( inc ../MDEvents/inc)

target_link_libraries ( MDAlgorithms ${MANTIDLIBS} MDEvents)

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MDAlgorithms ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
