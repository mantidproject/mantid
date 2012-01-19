# GLOBs should be replaced with explicit listings
set ( SRC_FILES
	# src/CreateMDFitWorkspace.cpp
	src/AndMD.cpp
	src/BinaryOperationMD.cpp
	src/BooleanBinaryOperationMD.cpp
	src/CobaltSpinWaveDSHO.cpp
	src/CompareMDWorkspaces.cpp
	src/CompositeFunctionBuilder.cpp
	src/CompositeImplicitFunction.cpp
	src/CompositeImplicitFunctionParser.cpp
	src/ConvertToMDEvents.cpp
	src/DivideMD.cpp
	src/EqualToMD.cpp
	src/ExponentialMD.cpp
	src/GreaterThanMD.cpp
	src/IConvertToMDEventsMethods.cpp
	src/IDynamicRebinning.cpp
	src/InvalidParameter.cpp
	src/InvalidParameterParser.cpp
	src/LessThanMD.cpp
	src/LogarithmMD.cpp
	src/MergeMD.cpp
	src/MergeMDFiles.cpp
	src/MinusMD.cpp
	src/MultiplyMD.cpp
	src/NotMD.cpp
	src/NullImplicitFunction.cpp
	src/OrMD.cpp
	src/PlusMD.cpp
	src/PowerMD.cpp
	src/QuadEnBackground.cpp
	src/RunParam.cpp
	src/SetMDUsingMask.cpp
	src/SimulateMDD.cpp
	src/SimulateResolution.cpp
	src/TobyFitSimulate.cpp
	src/TransformMD.cpp
	src/UnaryOperationMD.cpp
	src/Vector3DParameterParser.cpp
	src/XorMD.cpp
	src/processDetectorsPositions.cpp
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
	inc/MantidMDAlgorithms/CompositeFunctionBuilder.h
	inc/MantidMDAlgorithms/CompositeImplicitFunction.h
	inc/MantidMDAlgorithms/CompositeImplicitFunctionParser.h
	inc/MantidMDAlgorithms/ConvertToMDEvents.h
	inc/MantidMDAlgorithms/ConvertToMDEventsCoordTransf.h
	inc/MantidMDAlgorithms/ConvertToMDEventsDetInfo.h
	inc/MantidMDAlgorithms/ConvertToMDEventsEventWS.h
	inc/MantidMDAlgorithms/ConvertToMDEventsEventWSAutoRebin.h
	inc/MantidMDAlgorithms/ConvertToMDEventsHistoWS.h
	inc/MantidMDAlgorithms/ConvertToMDEventsUnitsConv.h
	inc/MantidMDAlgorithms/DivideMD.h
	inc/MantidMDAlgorithms/EqualToMD.h
	inc/MantidMDAlgorithms/ExponentialMD.h
	inc/MantidMDAlgorithms/GreaterThanMD.h
	inc/MantidMDAlgorithms/IConvertToMDEventsMethods.h
	inc/MantidMDAlgorithms/IDynamicRebinning.h
	inc/MantidMDAlgorithms/InvalidParameter.h
	inc/MantidMDAlgorithms/InvalidParameterParser.h
	inc/MantidMDAlgorithms/LessThanMD.h
	inc/MantidMDAlgorithms/LogarithmMD.h
	inc/MantidMDAlgorithms/MergeMD.h
	inc/MantidMDAlgorithms/MergeMDFiles.h
	inc/MantidMDAlgorithms/MinusMD.h
	inc/MantidMDAlgorithms/MultiplyMD.h
	inc/MantidMDAlgorithms/NotMD.h
	inc/MantidMDAlgorithms/NullImplicitFunction.h
	inc/MantidMDAlgorithms/OrMD.h
	inc/MantidMDAlgorithms/PlusMD.h
	inc/MantidMDAlgorithms/PowerMD.h
	inc/MantidMDAlgorithms/QuadEnBackground.h
	inc/MantidMDAlgorithms/RunParam.h
	inc/MantidMDAlgorithms/SetMDUsingMask.h
	inc/MantidMDAlgorithms/SimulateMDD.h
	inc/MantidMDAlgorithms/SimulateResolution.h
	inc/MantidMDAlgorithms/TobyFitSimulate.h
	inc/MantidMDAlgorithms/TransformMD.h
	inc/MantidMDAlgorithms/UnaryOperationMD.h
	inc/MantidMDAlgorithms/Vector3DParameter.h
	inc/MantidMDAlgorithms/Vector3DParameterParser.h
	inc/MantidMDAlgorithms/XorMD.h
)

# Test files. Other source files required.
set ( TEST_FILES
	# these tests are as they test verify different parts of the CPR algorithms
	#test/CobaltSWDTest.h
	#test/CreateMDFitWorkspaceTest.h
	test/AndMDTest.h
	test/BooleanBinaryOperationMDTest.h
	test/CompareMDWorkspacesTest.h
	test/ConvertEventsToMDEventsTest.h
	test/ConvertToMDEventsCoordTransfTest.h
	test/ConvertToMDEventsMethodsTest.h
	test/ConvertToMDEventsTest.h
	test/ConvertToMDEventsUnitsConvTest.h
	test/ConvertToQ3DdETest.h
	test/DivideMDTest.h
	test/EqualToMDTest.h
	test/ExponentialMDTest.h
	test/GreaterThanMDTest.h
	test/InvalidParameterParserTest.h
	test/InvalidParameterTest.h
	test/LessThanMDTest.h
	test/LogarithmMDTest.h
	test/MergeMDFilesTest.h
	test/MergeMDTest.h
	test/MinusMDTest.h
	test/MultiplyMDTest.h
	test/NotMDTest.h
	test/OrMDTest.h
	test/PlusMDTest.h
	test/PowerMDTest.h
	test/QuadEnBackgroundTest.h
	test/RunParamTest.h
	test/SetMDUsingMaskTest.h
	test/SimulateMDDTest.h
	test/TobyFitSimulateTest.h
	test/TransformMDTest.h
	test/UnaryOperationMDTest.h
	test/XorMDTest.h
)

set ( GMOCK_TEST_FILES
	test/CompositeBuilderTest.h
	test/CompositeImplicitFunctionTest.h
	test/NullImplicitFunctionTest.h
	test/BinaryOperationMDTest.h
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

# Create test file projects
if ( CXXTEST_FOUND )
  include_directories( ../TestHelpers/inc ../DataHandling/inc ../CurveFitting/inc )	
  # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../TestHelpers/src/ComponentCreationHelper.cpp 
                        ../TestHelpers/src/WorkspaceCreationHelper.cpp
                        ../TestHelpers/src/MDEventsTestHelper.cpp
                        ../TestHelpers/src/BinaryOperationMDTestHelper.cpp )
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MDAlgorithmsTest ${TEST_FILES} ${GMOCK_TEST_FILES})
    target_link_libraries( MDAlgorithmsTest MDAlgorithms DataHandling CurveFitting Nexus NexusCPP ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} )
  else ()
    cxxtest_add_test ( MDAlgorithmsTest ${TEST_FILES} )
    target_link_libraries( MDAlgorithmsTest MDAlgorithms DataHandling CurveFitting Nexus NexusCPP)
  endif ()
  add_dependencies ( FrameworkTests MDAlgorithmsTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MDAlgorithmsTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MDAlgorithms DESTINATION ${PLUGINS_DIR} )
