# Source files
set ( SRC_FILES
	# test helpers
	src/MDIndexCalculator.cpp
)

set ( SRC_UNITY_IGNORE_FILES )

# Header files
set ( INC_FILES
	# test helpers
	inc/MantidMDDataObjects/DllExport.h
	inc/MantidMDDataObjects/MDIndexCalculator.h
)

# Test files. Other source files required.
set ( TEST_FILES
	# test some complex test helpers
)

set ( GMOCK_TEST_FILES
	test/MDIndexCalculatorTest.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(MDDataObjects SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidMDDataObjects/PrecompiledHeader.h MantidMDDataObjects src/PrecompiledHeader.cpp SRC_FILES INC_FILES )
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
