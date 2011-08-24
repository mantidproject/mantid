# GLOBs should be replaced with explicit listings
set ( SRC_FILES
	src/Box3DImplicitFunction.cpp
	src/BoxFunctionBuilder.cpp
	src/BoxImplicitFunctionParser.cpp
	src/BoxInterpreter.cpp
	src/CenterpieceRebinning.cpp
	src/CobaltSpinWaveDSHO.cpp
	src/CompositeFunctionBuilder.cpp
	src/CompositeImplicitFunction.cpp
	src/CompositeImplicitFunctionParser.cpp
	src/CpRebinningNx3.cpp
	src/CreateMDFitWorkspace.cpp
	src/DynamicCPRRebinning.cpp
	src/DynamicRebinFromXML.cpp
	src/IDynamicRebinning.cpp
	src/InvalidParameter.cpp
	src/InvalidParameterParser.cpp
	src/Load_MDWorkspace.cpp
	src/MDParameterParserDeclarations.cpp
	src/NormalParameter.cpp
	src/NullImplicitFunction.cpp
	src/Plane3DImplicitFunction.cpp
	src/PlaneFunctionBuilder.cpp
	src/PlaneImplicitFunctionParser.cpp
	src/PlaneInterpreter.cpp
	src/QuadEnBackground.cpp
	src/RunParam.cpp
	src/SimulateMDD.cpp
	src/SimulateResolution.cpp
	src/TobyFitSimulate.cpp
	src/Vector3DParameterParser.cpp
	src/VectorMathematics.cpp
)

#set ( SRC_UNITY_IGNORE_FILES src/IDynamicRebinning.cpp
#)

set ( INC_FILES
	inc/MantidMDAlgorithms/Box3DImplicitFunction.h
	inc/MantidMDAlgorithms/BoxFunctionBuilder.h
	inc/MantidMDAlgorithms/BoxImplicitFunctionParser.h
	inc/MantidMDAlgorithms/BoxInterpreter.h
	inc/MantidMDAlgorithms/CenterpieceRebinning.h
	inc/MantidMDAlgorithms/CobaltSpinWaveDSHO.h
	inc/MantidMDAlgorithms/CompositeFunctionBuilder.h
	inc/MantidMDAlgorithms/CompositeImplicitFunction.h
	inc/MantidMDAlgorithms/CompositeImplicitFunctionParser.h
	inc/MantidMDAlgorithms/CpRebinningNx3.h
	inc/MantidMDAlgorithms/CreateMDFitWorkspace.h
	inc/MantidMDAlgorithms/DepthParameter.h
	inc/MantidMDAlgorithms/DynamicCPRRebinning.h
	inc/MantidMDAlgorithms/DynamicRebinFromXML.h
	inc/MantidMDAlgorithms/HeightParameter.h
	inc/MantidMDAlgorithms/IDynamicRebinning.h
	inc/MantidMDAlgorithms/InvalidParameter.h
	inc/MantidMDAlgorithms/InvalidParameterParser.h
	inc/MantidMDAlgorithms/Load_MDWorkspace.h
	inc/MantidMDAlgorithms/MDParameterParserDeclarations.h
	inc/MantidMDAlgorithms/NormalParameter.h
	inc/MantidMDAlgorithms/NullImplicitFunction.h
	inc/MantidMDAlgorithms/OriginParameter.h
	inc/MantidMDAlgorithms/PerpendicularParameter.h
	inc/MantidMDAlgorithms/Plane3DImplicitFunction.h
	inc/MantidMDAlgorithms/PlaneFunctionBuilder.h
	inc/MantidMDAlgorithms/PlaneImplicitFunctionParser.h
	inc/MantidMDAlgorithms/PlaneInterpreter.h
	inc/MantidMDAlgorithms/QuadEnBackground.h
	inc/MantidMDAlgorithms/RunParam.h
	inc/MantidMDAlgorithms/SimulateMDD.h
	inc/MantidMDAlgorithms/SimulateResolution.h
	inc/MantidMDAlgorithms/TobyFitSimulate.h
	inc/MantidMDAlgorithms/UpParameter.h
	inc/MantidMDAlgorithms/Vector3DParameter.h
	inc/MantidMDAlgorithms/Vector3DParameterParser.h
	inc/MantidMDAlgorithms/VectorMathematics.h
	inc/MantidMDAlgorithms/WidthParameter.h
)

# Test files. Other source files required.
set ( TEST_FILES
	# these tests are as they test verify different parts of the CPR algorithms
	#test/CobaltSWDTest.h
	#test/CreateMDFitWorkspaceTest.h
	#test/QuadEnBackgroundTest.h
	test/Box3DImplicitFunctionTest.h
	test/CP3DxN_RebinningTest.h
	test/CPR_preselectCellsTest.h
	test/CPRebinKeepPixTest.h
	test/DepthParameterTest.h
	test/DynamicRebinFromXMLAlgorithmTest.h
	test/HeightParameterTest.h
	test/InvalidParameterParserTest.h
	test/InvalidParameterTest.h
	test/Load_MDWorkspaceTest.h
	test/NormalParameterTest.h
	test/OriginParameterTest.h
	test/Plane3DImplicitFunctionTest.h
	test/PlaneFunctionBuilderTest.h
	test/RunParamTest.h
	test/SimulateMDDataTest.h
	test/TobyFitSimulateTest.h
	test/VectorMathematicsTest.h
	test/WidthParameterTest.h
)

set ( GMOCK_TEST_FILES
test/BoxBuilderTest.h
test/BoxImplicitFunctionParserTest.h
test/BoxInterpreterTest.h
test/CompositeBuilderTest.h
test/CompositeImplicitFunctionParserTest.h
test/CompositeImplicitFunctionTest.h
test/DepthParameterParserTest.h
test/HeightParameterParserTest.h
test/NormalParameterParserTest.h
test/NullImplicitFunctionTest.h
test/OriginParameterParserTest.h
test/PerpendicularParameterParserTest.h
test/PerpendicularParameterTest.h
test/PlaneImplicitFunctionParserTest.h
test/PlaneInterpreterTest.h
test/WidthParameterParserTest.h
test/UpParameterParserTest.h
test/UpParameterTest.h
)

# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidMDAlgorithms/PrecompiledHeader.h MantidMDAlgorithms MDAlgorithms SRC_FILES INC_FILES )

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(MDAlgorithms SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add the target for this directory
add_library ( MDAlgorithms ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( MDAlgorithms PROPERTIES OUTPUT_NAME MantidMDAlgorithms 
                                                COMPILE_DEFINITIONS IN_MANTID_MDALGORITHMS )
# Add to the 'Framework' group in VS
set_property ( TARGET MDAlgorithms PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )

target_link_libraries ( MDAlgorithms ${MANTIDLIBS} MDEvents)

# Create test file projects
if ( CXXTEST_FOUND )
  include_directories(../CurveFitting/inc ../MDEvents/inc ../Nexus/inc)	
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MDAlgorithmsTest ${TEST_FILES} ${GMOCK_TEST_FILES})
    target_link_libraries( MDAlgorithmsTest MDAlgorithms MDDataObjects CurveFitting MDEvents API
      ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES})
  else ()
    cxxtest_add_test ( MDAlgorithmsTest ${TEST_FILES} )
    target_link_libraries( MDAlgorithmsTest MDAlgorithms MDDataObjects CurveFitting MDEvents API )
  endif ()
  add_dependencies ( FrameworkTests MDAlgorithmsTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MDAlgorithmsTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MDAlgorithms DESTINATION ${PLUGINS_DIR} )
