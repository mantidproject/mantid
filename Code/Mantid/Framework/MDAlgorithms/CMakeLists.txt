# GLOBs should be replaced with explicit listings
set ( SRC_FILES src/BoxFunctionBuilder.cpp
src/BoxImplicitFunction.cpp
src/BoxImplicitFunctionParser.cpp
src/BoxInterpreter.cpp
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
inc/MantidMDAlgorithms/BoxInterpreter.h
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
#test/CPR_preselectCellsTest.h TODO: Is this test really necessary?
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
#test/SingleValueParameterBaseTest.h TODO: Not a CxxTest? Sure looks like one.
#test/testCPComponents.h TODO: Is this test really necessary?
test/testCPrebinning.h
test/VectorMathematicsTest.h
test/WidthParameterTest.h
)

set ( GMOCK_TEST_FILES
test/BoxBuilderTest.h
test/BoxImplicitFunctionParserTest.h
test/BoxImplicitFunctionTest.h
test/BoxInterpreterTest.h
test/CompositeBuilderTest.h
test/CompositeImplicitFunctionParserTest.h
test/CompositeImplicitFunctionTest.h
test/DepthParameterParserTest.h
#test/FunctionParserTest.h TODO: No tests defined??
test/HeightParameterParserTest.h
test/NormalParameterParserTest.h
test/OriginParameterParserTest.h
test/PlaneImplicitFunctionParserTest.h
test/PlaneImplicitFunctionTest.h
test/WidthParameterParserTest.h
)

# Add the target for this directory
add_library ( MDAlgorithms ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( MDAlgorithms PROPERTIES OUTPUT_NAME MantidMDAlgorithms 
                                                COMPILE_DEFINITIONS IN_MANTID_MDALGORITHMS )
# Add to the 'Framework' group in VS
set_property ( TARGET MDAlgorithms PROPERTY FOLDER "Framework" )

include_directories ( inc )
include_directories (MDDataObjects/inc)


# sorting out hdf5 links for MDAlgorithms -- included through MDDataObjects and should be refractored to go out of here
if( WIN32 )
	set ( HDF5_INCLUDE_DIRS ${CMAKE_INCLUDE_PATH}/hdf5 )
	# separate configuration files h5config.h for hdf5 in 32 and 64 bit versions
	if ( CMAKE_CL_64 )
		set ( HDF5_INCLUDE_DIRS ${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS}/win64 )
	else()
		set ( HDF5_INCLUDE_DIRS ${HDF5_INCLUDE_DIRS} ${HDF5_INCLUDE_DIRS}/win32 )
	endif()
else()
# unix build currently works wirh hdf1.6 linked against Nexus.
# Some preprocessor command should already be defined to distinguish between 5.1.6 and 5.1.8 
	find_package (HDF5 REQUIRED)
	add_definitions ( -DHDF_1_6 )
endif()
include_directories (${HDF5_INCLUDE_DIRS})

target_link_libraries ( MDAlgorithms ${MANTIDLIBS} )


# Create test file projects
if ( CXXTEST_FOUND )
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MDAlgorithmsTest ${TEST_FILES} ${GMOCK_TEST_FILES})
    target_link_libraries( MDAlgorithmsTest MDAlgorithms MDDataObjects 
      ${GMOCK_LIBRARIES} )
  else ()
    cxxtest_add_test ( MDAlgorithmsTest ${TEST_FILES} )
    target_link_libraries( MDAlgorithmsTest MDAlgorithms MDDataObjects )
  endif ()
  add_dependencies ( FrameworkTests MDAlgorithmsTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MDAlgorithmsTest PROPERTY FOLDER "FrameworkTests" )
endif ()
