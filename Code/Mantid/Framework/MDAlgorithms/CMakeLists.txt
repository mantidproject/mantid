# GLOBs should be replaced with explicit listings
set ( SRC_FILES src/BoxFunctionBuilder.cpp
src/BoxImplicitFunction.cpp
src/BoxImplicitFunctionParser.cpp
src/CompositeFunctionBuilder.cpp
src/CompositeImplicitFunction.cpp
src/CompositeImplicitFunctionParser.cpp
src/CenterpieceRebinning.cpp
src/CpRebinningNx3.cpp
src/CpRebinning4x3StructHR.cpp
src/DynamicCPRRebinning.cpp
src/IDynamicRebinning.cpp
src/DepthParameter.cpp
src/DynamicCPRRebinning.cpp
src/DynamicRebinFromXML.cpp
src/HeightParameter.cpp
src/IDynamicRebinning.cpp
src/InvalidParameter.cpp
src/InvalidParameterParser.cpp
src/Load_MDWorkspace.cpp
src/NormalParameter.cpp
src/NormalParameterParser.cpp
src/OriginParameter.cpp
src/OriginParameterParser.cpp
src/PlaneFunctionBuilder.cpp
src/PlaneImplicitFunction.cpp
src/PlaneImplicitFunctionParser.cpp
src/SimulateMDD.cpp
src/SingleValueParameterParser.cpp
src/WidthParameter.cpp 
)

set ( INC_FILES inc/MantidMDAlgorithms/BoxFunctionBuilder.h
inc/MantidMDAlgorithms/BoxImplicitFunction.h
inc/MantidMDAlgorithms/BoxImplicitFunctionParser.h
inc/MantidMDAlgorithms/CenterpieceRebinning.h
inc/MantidMDAlgorithms/CpRebinningNx3.h
inc/MantidMDAlgorithms/CpRebinning4x3StructHR.h
inc/MantidMDAlgorithms/CompositeFunctionBuilder.h
inc/MantidMDAlgorithms/CompositeImplicitFunction.h
inc/MantidMDAlgorithms/CompositeImplicitFunctionParser.h
inc/MantidMDAlgorithms/DepthParameter.h
inc/MantidMDAlgorithms/DynamicCPRRebinning.h
inc/MantidMDAlgorithms/IDynamicRebinning.h
inc/MantidMDAlgorithms/DynamicRebinFromXML.h
inc/MantidMDAlgorithms/HeightParameter.h
inc/MantidMDAlgorithms/IDynamicRebinning.h
inc/MantidMDAlgorithms/InvalidParameter.h
inc/MantidMDAlgorithms/InvalidParameterParser.h
inc/MantidMDAlgorithms/Load_MDWorkspace.h
inc/MantidMDAlgorithms/NormalParameter.h
inc/MantidMDAlgorithms/NormalParameterParser.h
inc/MantidMDAlgorithms/OriginParameter.h
inc/MantidMDAlgorithms/OriginParameterParser.h
inc/MantidMDAlgorithms/PlaneFunctionBuilder.h
inc/MantidMDAlgorithms/PlaneImplicitFunction.h
inc/MantidMDAlgorithms/PlaneImplicitFunctionParser.h
inc/MantidMDAlgorithms/SimulateMDD.h
inc/MantidMDAlgorithms/SingleValueParameterParser.h
inc/MantidMDAlgorithms/Topology.h
inc/MantidMDAlgorithms/VectorMathematics.h
inc/MantidMDAlgorithms/WidthParameter.h 
)

# Test files. Other source files required.
set (TEST_FILES
test/CPR_preselectCellsTest.h
test/DepthParameterTest.h
test/DynamicRebinFromXMLAlgorithmTest.h
test/HeightParameterTest.h
test/InvalidParameterParserTest.h
test/InvalidParameterTest.h
test/Load_MDWorkspaceTest.h
test/NormalParameterTest.h
test/OriginParameterTest.h
test/PlaneFunctionBuilderTest.h
test/SimulateMDDataTest.h
test/SingleValueParameterBaseTest.h
test/testCPComponents.h
test/testCPrebinning.cpp
test/testCPrebinning.h
test/VectorMathematicsTest.h
test/WidthParameterTest.h
)

set ( GMOCK_TEST_FILES
test/BoxBuilderTest.h
test/BoxImplicitFunctionParserTest.h
test/BoxImplicitFunctionTest.h
test/CompositeBuilderTest.h
test/CompositeImplicitFunctionParserTest.h
test/CompositeImplicitFunctionTest.h
test/DepthParameterParserTest.h
test/FunctionParserTest.h
test/HeightParameterParserTest.h
test/NormalParameterParserTest.h
test/OriginParameterParserTest.h
test/PlaneImplicitFunctionParserTest.h
test/PlaneImplicitFunctionTest.h
test/WidthParameterParserTest.h
)

# For Windows:
add_definitions ( -DIN_MANTID_MDALGORITHMS )
# Add the target for this directory
add_library ( MDAlgorithms ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( MDAlgorithms PROPERTIES OUTPUT_NAME MantidMDAlgorithms )
# Add to the 'Framework' group in VS
set_property ( TARGET MDAlgorithms PROPERTY FOLDER "Framework" )

include_directories ( inc )
include_directories (MDDataObjects/inc)
target_link_libraries ( MDAlgorithms ${MANTIDLIBS} )

# Create test file projects
if ( CXXTEST_FOUND )
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MDAlgorithmsTest ${TEST_FILES} ${GMOCK_TEST_FILES})
    target_link_libraries( MDAlgorithmsTest MDAlgorithms MDDataObjects gmock)
  else ()
    cxxtest_add_test ( MDAlgorithmsTest ${TEST_FILES} )
    target_link_libraries( MDAlgorithmsTest MDAlgorithms MDDataObjects )
  endif ()
  add_dependencies ( FrameworkTests MDAlgorithmsTest )
endif ()
