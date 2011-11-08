# GLOBs should be replaced with explicit listings
set ( SRC_FILES
	# src/CreateMDFitWorkspace.cpp
	src/BinaryOperationMD.cpp
	src/CobaltSpinWaveDSHO.cpp
	src/CompositeFunctionBuilder.cpp
	src/CompositeImplicitFunction.cpp
	src/CompositeImplicitFunctionParser.cpp
	src/ConvertToQ3DdE.cpp
	src/ConvertToQNDany.cpp
	src/DivideMD.cpp
	src/IDynamicRebinning.cpp
	src/InvalidParameter.cpp
	src/InvalidParameterParser.cpp
	src/MinusMD.cpp
	src/MultiplyMD.cpp
	src/NullImplicitFunction.cpp
	src/PlusMD.cpp
	src/QuadEnBackground.cpp
	src/RunParam.cpp
	src/SimulateMDD.cpp
	src/SimulateResolution.cpp
	src/TobyFitSimulate.cpp
	src/UnaryOperationMD.cpp
	src/Vector3DParameterParser.cpp
)

#set ( SRC_UNITY_IGNORE_FILES src/IDynamicRebinning.cpp
#)

set ( INC_FILES
	# inc/MantidMDAlgorithms/CreateMDFitWorkspace.h
	inc/MantidMDAlgorithms/BinaryOperationMD.h
	inc/MantidMDAlgorithms/CobaltSpinWaveDSHO.h
	inc/MantidMDAlgorithms/CompositeFunctionBuilder.h
	inc/MantidMDAlgorithms/CompositeImplicitFunction.h
	inc/MantidMDAlgorithms/CompositeImplicitFunctionParser.h
	inc/MantidMDAlgorithms/ConvertToQ3DdE.h
	inc/MantidMDAlgorithms/ConvertToQNDany.h
	inc/MantidMDAlgorithms/DivideMD.h
	inc/MantidMDAlgorithms/IDynamicRebinning.h
	inc/MantidMDAlgorithms/InvalidParameter.h
	inc/MantidMDAlgorithms/InvalidParameterParser.h
	inc/MantidMDAlgorithms/MinusMD.h
	inc/MantidMDAlgorithms/MultiplyMD.h
	inc/MantidMDAlgorithms/NullImplicitFunction.h
	inc/MantidMDAlgorithms/PlusMD.h
	inc/MantidMDAlgorithms/QuadEnBackground.h
	inc/MantidMDAlgorithms/RunParam.h
	inc/MantidMDAlgorithms/SimulateMDD.h
	inc/MantidMDAlgorithms/SimulateResolution.h
	inc/MantidMDAlgorithms/TobyFitSimulate.h
	inc/MantidMDAlgorithms/UnaryOperationMD.h
	inc/MantidMDAlgorithms/Vector3DParameter.h
	inc/MantidMDAlgorithms/Vector3DParameterParser.h
)

# Test files. Other source files required.
set ( TEST_FILES
	# these tests are as they test verify different parts of the CPR algorithms
	#test/CobaltSWDTest.h
	#test/CreateMDFitWorkspaceTest.h
	#test/QuadEnBackgroundTest.h
	test/ConvertToQ3DdETest.h
	test/ConvertToQNDanyTest.h
	test/DivideMDTest.h
	test/InvalidParameterParserTest.h
	test/InvalidParameterTest.h
	test/MinusMDTest.h
	test/MultiplyMDTest.h
	test/PlusMDTest.h
	test/RunParamTest.h
	test/SimulateMDDTest.h
	test/TobyFitSimulateTest.h
	test/UnaryOperationMDTest.h
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
  include_directories(../CurveFitting/inc ../MDEvents/inc ../Nexus/inc ../TestHelpers/inc 
          ../DataHandling/inc ../DataObjects/inc )	
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MDAlgorithmsTest ${TEST_FILES} ${GMOCK_TEST_FILES})
    target_link_libraries( MDAlgorithmsTest MDAlgorithms CurveFitting MDEvents API DataHandling Nexus 
	   TestHelpers
      ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES})
  else ()
    cxxtest_add_test ( MDAlgorithmsTest ${TEST_FILES} )
    target_link_libraries( MDAlgorithmsTest MDAlgorithms CurveFitting MDEvents API  
	TestHelpers DataHandling Nexus )
  endif ()
  add_dependencies ( FrameworkTests MDAlgorithmsTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MDAlgorithmsTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MDAlgorithms DESTINATION ${PLUGINS_DIR} )
