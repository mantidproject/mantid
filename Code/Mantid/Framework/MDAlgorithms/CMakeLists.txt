# GLOBs should be replaced with explicit listings
set ( SRC_FILES
	# src/CreateMDFitWorkspace.cpp
	src/AndMD.cpp
	src/BinaryOperationMD.cpp
	src/BooleanBinaryOperationMD.cpp
	src/CobaltSpinWaveDSHO.cpp
	src/CompareMDWorkspaces.cpp
	src/ConvertToDetectorFaceMD.cpp
	src/ConvertToMD.cpp
	src/DampedHisenbergFMSW.cpp
	src/DivideMD.cpp
	src/EqualToMD.cpp
	src/ExponentialMD.cpp
	src/GreaterThanMD.cpp
	src/IDynamicRebinning.cpp
	src/InvalidParameter.cpp
	src/InvalidParameterParser.cpp
	src/LessThanMD.cpp
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
	src/TobyFitSimulate.cpp
	src/TransformMD.cpp
	src/UnaryOperationMD.cpp
	src/Vector3DParameterParser.cpp
	src/WeightedMeanMD.cpp
	src/XorMD.cpp
# Old TMD convertToMD code prepared for deprication:	
	src/ConvertToMDEvents.cpp	
	src/ConvertToMDEventsParams.cpp
	src/ConvertToMDEventsSubalgFactory.cpp
	src/ConvertToMDEventsWSBase.cpp
	src/StitchGroup1D.cpp
)

#set ( SRC_UNITY_IGNORE_FILES src/IDynamicRebinning.cpp
#)

set ( INC_FILES
	# inc/MantidMDAlgorithms/CreateMDFitWorkspace.h
	inc/MantidMDAlgorithms/AndMD.h
	inc/MantidMDAlgorithms/BinaryOperationMD.h
	inc/MantidMDAlgorithms/BooleanBinaryOperationMD.h
	inc/MantidMDAlgorithms/CobaltSpinWaveDSHO.h
	inc/MantidMDAlgorithms/CompareMDWorkspaces.h
	inc/MantidMDAlgorithms/ConvertToDetectorFaceMD.h
	inc/MantidMDAlgorithms/ConvertToMD.h
	inc/MantidMDAlgorithms/DampedHisenbergFMSW.h
	inc/MantidMDAlgorithms/DivideMD.h
	inc/MantidMDAlgorithms/DllConfig.h
	inc/MantidMDAlgorithms/EqualToMD.h
	inc/MantidMDAlgorithms/ExponentialMD.h
	inc/MantidMDAlgorithms/GreaterThanMD.h
	inc/MantidMDAlgorithms/IDynamicRebinning.h
	inc/MantidMDAlgorithms/InvalidParameter.h
	inc/MantidMDAlgorithms/InvalidParameterParser.h
	inc/MantidMDAlgorithms/LessThanMD.h
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
	inc/MantidMDAlgorithms/SaveZODS.h
	inc/MantidMDAlgorithms/SetMDUsingMask.h
	inc/MantidMDAlgorithms/SimulateMDD.h
	inc/MantidMDAlgorithms/SimulateResolution.h
	inc/MantidMDAlgorithms/TobyFitSimulate.h
	inc/MantidMDAlgorithms/TransformMD.h
	inc/MantidMDAlgorithms/UnaryOperationMD.h
	inc/MantidMDAlgorithms/Vector3DParameter.h
	inc/MantidMDAlgorithms/Vector3DParameterParser.h
	inc/MantidMDAlgorithms/WeightedMeanMD.h
	inc/MantidMDAlgorithms/XorMD.h
# Old TMD convertToMD code prepared for deprication:
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
    inc/MantidMDAlgorithms/StitchGroup1D.h
)

# Test files. Other source files required.
set ( TEST_FILES
	#
	#	ConvertToMDEventsCoordTransfTest.h
	# these tests are as they test verify different parts of the CPR algorithms
	#CreateMDFitWorkspaceTest.h
	AndMDTest.h
	BooleanBinaryOperationMDTest.h
	CachedExperimentInfoTest.h
	CobaltSWDTest.h
	CompareMDWorkspacesTest.h
	ConvertEventsToMDTest.h
	ConvertToDetectorFaceMDTest.h
	ConvertToMDTest.h
	ConvertToMDTestPerformance.h
	ConvertToQ3DdETest.h
	DampedHisenbergFMSWTest.h
	DivideMDTest.h
	EqualToMDTest.h
	ExponentialMDTest.h
	FitResolutionConvolvedModelTest.h
	ForegroundModelTest.h
	GreaterThanMDTest.h
	InvalidParameterParserTest.h
	InvalidParameterTest.h
	LessThanMDTest.h
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
	QuadEnBackgroundTest.h
	ResolutionConvolvedCrossSectionTest.h
	RunParamTest.h
	SaveZODSTest.h
	SetMDUsingMaskTest.h
	SimulateMDDTest.h
	SimulateResolutionConvolvedModelTest.h
	SimulateResolutionTest.h
	Strontium122Test.h
	TobyFitBMatrixTest.h
	TobyFitResolutionModelTest.h
	TobyFitSimulateTest.h
	TobyFitYVectorTest.h
	TransformMDTest.h
	UnaryOperationMDTest.h
	WeightedMeanMDTest.h
	XorMDTest.h
    StitchGroup1DTest.h
# Old TMD convertToMD code prepared for deprication:
#    ConvertToMDEventsUnitsConvTest.h 
)

set ( GMOCK_TEST_FILES
	BinaryOperationMDTest.h
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
