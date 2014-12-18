# GLOBs should be replaced with explicit listings
set ( SRC_FILES
	# Old TMP convertToMD code prepared for deprecation:
	# src/CreateMDFitWorkspace.cpp
	#end TMP
	src/AndMD.cpp
	src/BinMD.cpp
	src/BinaryOperationMD.cpp
	src/BooleanBinaryOperationMD.cpp
	src/CalculateCoverageDGS.cpp
	src/CentroidPeaksMD.cpp
	src/CentroidPeaksMD2.cpp
	src/CloneMDWorkspace.cpp
	src/CompareMDWorkspaces.cpp
	src/ConvertToDetectorFaceMD.cpp
	src/ConvertToDiffractionMDWorkspace.cpp
	src/ConvertToDiffractionMDWorkspace2.cpp
	src/ConvertToMD.cpp
	src/ConvertToMDMinMaxGlobal.cpp
	src/ConvertToMDMinMaxLocal.cpp
	src/ConvertToMDParent.cpp
	src/CreateMDHistoWorkspace.cpp
	src/CreateMDWorkspace.cpp
	src/DivideMD.cpp
	src/EqualToMD.cpp
	src/EvaluateMDFunction.cpp
	src/ExponentialMD.cpp
	src/FakeMDEventData.cpp
	src/FindPeaksMD.cpp
	src/GreaterThanMD.cpp
	src/IDynamicRebinning.cpp
	src/IntegrateFlux.cpp
	src/IntegratePeaksMD.cpp
	src/IntegratePeaksMD2.cpp
	src/InvalidParameter.cpp
	src/InvalidParameterParser.cpp
	src/LessThanMD.cpp
	src/LoadILLAscii.cpp
	src/LoadILLAsciiHelper.cpp
	src/LoadMD.cpp
	src/LoadSQW.cpp
	src/LogarithmMD.cpp
	src/MDNormSCD.cpp
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
	src/Quantification/CachedExperimentInfo.cpp
	src/Quantification/FitResolutionConvolvedModel.cpp
	src/Quantification/ForegroundModel.cpp
	src/Quantification/ForegroundModelFactory.cpp
	src/Quantification/MDResolutionConvolution.cpp
	src/Quantification/MDResolutionConvolutionFactory.cpp
	src/Quantification/Models/MullerAnsatz.cpp
	src/Quantification/Models/QCoordinate.cpp
	src/Quantification/Models/Strontium122.cpp
	src/Quantification/Resolution/ModeratorChopperResolution.cpp
	src/Quantification/Resolution/TobyFitBMatrix.cpp
	src/Quantification/Resolution/TobyFitResolutionModel.cpp
	src/Quantification/Resolution/TobyFitYVector.cpp
	src/Quantification/ResolutionConvolvedCrossSection.cpp
	src/Quantification/SimulateResolutionConvolvedModel.cpp
	src/SaveMD.cpp
	src/SaveZODS.cpp
	src/SetMDUsingMask.cpp
	src/SliceMD.cpp
	src/SlicingAlgorithm.cpp
	src/ThresholdMD.cpp
	src/TransformMD.cpp
	src/UnaryOperationMD.cpp
	src/Vector3DParameterParser.cpp
	src/WeightedMeanMD.cpp
	src/XorMD.cpp
)

#set ( SRC_UNITY_IGNORE_FILES src/IDynamicRebinning.cpp
#)

set ( INC_FILES
	# inc/MantidMDAlgorithms/CreateMDFitWorkspace.h
	inc/MantidMDAlgorithms/AndMD.h
	inc/MantidMDAlgorithms/BinMD.h
	inc/MantidMDAlgorithms/BinaryOperationMD.h
	inc/MantidMDAlgorithms/BooleanBinaryOperationMD.h
	inc/MantidMDAlgorithms/CalculateCoverageDGS.h
	inc/MantidMDAlgorithms/CentroidPeaksMD.h
	inc/MantidMDAlgorithms/CentroidPeaksMD2.h
	inc/MantidMDAlgorithms/CloneMDWorkspace.h
	inc/MantidMDAlgorithms/CompareMDWorkspaces.h
	inc/MantidMDAlgorithms/ConvertToDetectorFaceMD.h
	inc/MantidMDAlgorithms/ConvertToDiffractionMDWorkspace.h
	inc/MantidMDAlgorithms/ConvertToDiffractionMDWorkspace2.h
	inc/MantidMDAlgorithms/ConvertToMD.h
	inc/MantidMDAlgorithms/ConvertToMDMinMaxGlobal.h
	inc/MantidMDAlgorithms/ConvertToMDMinMaxLocal.h
	inc/MantidMDAlgorithms/ConvertToMDParent.h
	inc/MantidMDAlgorithms/CreateMDHistoWorkspace.h
	inc/MantidMDAlgorithms/CreateMDWorkspace.h
	inc/MantidMDAlgorithms/DivideMD.h
	inc/MantidMDAlgorithms/DllConfig.h
	inc/MantidMDAlgorithms/EqualToMD.h
	inc/MantidMDAlgorithms/EvaluateMDFunction.h
	inc/MantidMDAlgorithms/ExponentialMD.h
	inc/MantidMDAlgorithms/FakeMDEventData.h
	inc/MantidMDAlgorithms/FindPeaksMD.h
	inc/MantidMDAlgorithms/GSLFunctions.h
	inc/MantidMDAlgorithms/GreaterThanMD.h
	inc/MantidMDAlgorithms/IDynamicRebinning.h
	inc/MantidMDAlgorithms/IntegrateFlux.h
	inc/MantidMDAlgorithms/IntegratePeaksMD.h
	inc/MantidMDAlgorithms/IntegratePeaksMD2.h
	inc/MantidMDAlgorithms/InvalidParameter.h
	inc/MantidMDAlgorithms/InvalidParameterParser.h
	inc/MantidMDAlgorithms/LessThanMD.h
	inc/MantidMDAlgorithms/LoadILLAscii.h
	inc/MantidMDAlgorithms/LoadILLAsciiHelper.h
	inc/MantidMDAlgorithms/LoadMD.h
	inc/MantidMDAlgorithms/LoadSQW.h
	inc/MantidMDAlgorithms/LogarithmMD.h
	inc/MantidMDAlgorithms/MDNormSCD.h
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
	inc/MantidMDAlgorithms/Quantification/CachedExperimentInfo.h
	inc/MantidMDAlgorithms/Quantification/FitResolutionConvolvedModel.h
	inc/MantidMDAlgorithms/Quantification/ForegroundModel.h
	inc/MantidMDAlgorithms/Quantification/ForegroundModelFactory.h
	inc/MantidMDAlgorithms/Quantification/MDResolutionConvolution.h
	inc/MantidMDAlgorithms/Quantification/MDResolutionConvolutionFactory.h
	inc/MantidMDAlgorithms/Quantification/Models/MullerAnsatz.h
	inc/MantidMDAlgorithms/Quantification/Models/QCoordinate.h
	inc/MantidMDAlgorithms/Quantification/Models/Strontium122.h
	inc/MantidMDAlgorithms/Quantification/Resolution/ModeratorChopperResolution.h
	inc/MantidMDAlgorithms/Quantification/Resolution/TobyFitBMatrix.h
	inc/MantidMDAlgorithms/Quantification/Resolution/TobyFitResolutionModel.h
	inc/MantidMDAlgorithms/Quantification/Resolution/TobyFitYVector.h
	inc/MantidMDAlgorithms/Quantification/ResolutionConvolvedCrossSection.h
	inc/MantidMDAlgorithms/Quantification/SimulateResolutionConvolvedModel.h
	inc/MantidMDAlgorithms/SaveMD.h
	inc/MantidMDAlgorithms/SaveZODS.h
	inc/MantidMDAlgorithms/SetMDUsingMask.h
	inc/MantidMDAlgorithms/SliceMD.h
	inc/MantidMDAlgorithms/SlicingAlgorithm.h
	inc/MantidMDAlgorithms/ThresholdMD.h
	inc/MantidMDAlgorithms/TransformMD.h
	inc/MantidMDAlgorithms/UnaryOperationMD.h
	inc/MantidMDAlgorithms/Vector3DParameter.h
	inc/MantidMDAlgorithms/Vector3DParameterParser.h
	inc/MantidMDAlgorithms/WeightedMeanMD.h
	inc/MantidMDAlgorithms/XorMD.h
)

# Test files. Other source files required.
set ( TEST_FILES
	#
	# these tests are as they test verify different parts of the CPR algorithms
	#CreateMDFitWorkspaceTest.h
	AndMDTest.h
	BooleanBinaryOperationMDTest.h
	CachedExperimentInfoTest.h
	CalculateCoverageDGSTest.h
	CentroidPeaksMD2Test.h
	CentroidPeaksMDTest.h
	CloneMDWorkspaceTest.h
	CompareMDWorkspacesTest.h
	ConvertEventsToMDTest.h
	ConvertToDetectorFaceMDTest.h
	ConvertToDiffractionMDWorkspace2Test.h
	ConvertToDiffractionMDWorkspaceTest.h
	ConvertToMDComponentsTest.h
	ConvertToMDMinMaxGlobalTest.h
	ConvertToMDMinMaxLocalTest.h
	ConvertToMDTest.h
	ConvertToQ3DdETest.h
	CreateMDHistoWorkspaceTest.h
	CreateMDWorkspaceTest.h
	DivideMDTest.h
	EqualToMDTest.h
	EvaluateMDFunctionTest.h
	ExponentialMDTest.h
	FakeMDEventDataTest.h
	FindPeaksMDTest.h
	FitResolutionConvolvedModelTest.h
	ForegroundModelTest.h
	GreaterThanMDTest.h
	IntegrateFluxTest.h
	IntegratePeaksMD2Test.h
	IntegratePeaksMDTest.h
	InvalidParameterParserTest.h
	InvalidParameterTest.h
	LessThanMDTest.h
	LoadMDTest.h
	LoadSQWTest.h
	LogarithmMDTest.h
	MDNormSCDTest.h
	MDResolutionConvolutionFactoryTest.h
	MaskMDTest.h
	MergeMDFilesTest.h
	MergeMDTest.h
	MinusMDTest.h
	ModeratorChopperResolutionTest.h
	MullerAnsatzTest.h
	MultiplyMDTest.h
	NotMDTest.h
	OrMDTest.h
	PlusMDTest.h
	PowerMDTest.h
	PreprocessDetectorsToMDTest.h
	ResolutionConvolvedCrossSectionTest.h
	SaveMDTest.h
	SaveZODSTest.h
	SetMDUsingMaskTest.h
	SimulateResolutionConvolvedModelTest.h
	SliceMDTest.h
	SlicingAlgorithmTest.h
	Strontium122Test.h
	ThresholdMDTest.h
	TobyFitBMatrixTest.h
	TobyFitResolutionModelTest.h
	TobyFitYVectorTest.h
	TransformMDTest.h
	UnaryOperationMDTest.h
	WeightedMeanMDTest.h
	XorMDTest.h
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
enable_precompiled_headers( inc/MantidMDAlgorithms/PrecompiledHeader.h SRC_FILES )
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
