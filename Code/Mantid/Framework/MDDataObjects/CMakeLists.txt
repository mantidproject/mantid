# Source files
set ( SRC_FILES
	#src/Events/MDBox.cpp
	#src/Events/MDEventWorkspace.cpp # breaks the windows build
	src/IMD_FileFormat.cpp
	src/MDDPoints_MemManager.cpp
	src/MDDataPoint.cpp
	src/MDDataPointDescription.cpp
	src/MDDataPoints.cpp
	src/MDFitWorkspace.cpp
	src/MDImage.cpp
	src/MDIndexCalculator.cpp
	src/MDWorkspace.cpp
    src/MDWorkspaceIterator.cpp
	src/MD_FileFormatFactory.cpp
	src/MD_FileHoraceReader.cpp
# test helpers
	testHelpers/MD_FileTestDataGenerator.cpp
    testHelpers/MDDensityTestHelper.cpp 
)

set ( SRC_UNITY_IGNORE_FILES )

# Header files
set ( INC_FILES
	inc/MDDataObjects/DllExport.h
	inc/MDDataObjects/IMD_FileFormat.h
	inc/MDDataObjects/MDDPoints_MemManager.h
	inc/MDDataObjects/MDDataPoint.h
	inc/MDDataObjects/MDDataPointDescription.h
	inc/MDDataObjects/MDDataPoints.h
	inc/MDDataObjects/MDFitWorkspace.h
	inc/MDDataObjects/MDImage.h
	inc/MDDataObjects/MDImageDatatypes.h
	inc/MDDataObjects/MDIndexCalculator.h
	inc/MDDataObjects/MDWorkspace.h
    inc/MDDataObjects/MDWorkspaceIterator.h
	inc/MDDataObjects/MD_FileFormatFactory.h
	inc/MDDataObjects/MD_FileHoraceReader.h
# test helpers
	inc/MDDataObjectsTestHelpers/MDTestWorkspace.h 	
	inc/MDDataObjectsTestHelpers/MD_FileTestDataGenerator.h 	
	inc/MDDataObjectsTestHelpers/MDDensityTestHelper.h 
)

# Test files. Other source files required.
set ( TEST_FILES
	test/HoraceReaderTest.h
	#test/MDDPoints_MemManagerTest.h #TODO fix. This currently brings the test suite down.
	test/MDDataPointDescriptionTest.h
	test/MDFitWorkspaceTest.h
	test/MD_FileFactoryTest.h
# test some complex test helpers
	test/MDDensityTestHelperTest.h 
    test/MDFileTestDataGeneratorTest.h 
)

set ( GMOCK_TEST_FILES
test/MDIndexCalculatorTest.h
test/MDDataPointTest.h
test/MDImageTest.h
test/MDPixelsTest.h
test/MDWorkspaceTest.h
test/MDWorkspaceIteratorTest.h
)

# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MDDataObjects/PrecompiledHeader.h MDDataObjects MDDataObjects SRC_FILES INC_FILES )

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(MDDataObjects SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)


# Add the target for this directory
add_library ( MDDataObjects ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( MDDataObjects PROPERTIES OUTPUT_NAME MantidMDDataObjects 
                                                 COMPILE_DEFINITIONS IN_MANTID_MDDATAOBJECTS )
# Add to the 'Framework' group in VS
set_property ( TARGET MDDataObjects PROPERTY FOLDER "MantidFramework" )

include_directories(inc)
target_link_libraries ( MDDataObjects ${MANTIDLIBS})

# Create test file projects
if ( CXXTEST_FOUND )
  include_directories(inc)
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MDDataObjectsTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
    target_link_libraries( MDDataObjectsTest MDDataObjects ${MANTIDLIBS} 
        ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES})
  else ()
    cxxtest_add_test ( MDDataObjectsTest ${TEST_FILES} )
    target_link_libraries( MDDataObjectsTest MDDataObjects ${MANTIDLIBS} )
  endif ()
  
  add_dependencies ( FrameworkTests MDDataObjectsTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET MDDataObjectsTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MDDataObjects DESTINATION ${LIB_DIR} )