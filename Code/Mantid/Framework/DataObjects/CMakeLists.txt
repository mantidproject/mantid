set ( SRC_FILES
	src/AbsManagedWorkspace2D.cpp
	src/CompressedWorkspace2D.cpp
	src/EventList.cpp
	src/EventWorkspace.cpp
	src/EventWorkspaceHelpers.cpp
	src/EventWorkspaceMRU.cpp
	src/Events.cpp
	src/GroupingWorkspace.cpp
	src/Histogram1D.cpp
	src/ManagedDataBlock2D.cpp
	src/ManagedHistogram1D.cpp
	src/ManagedWorkspace2D.cpp
	src/MementoTableWorkspace.cpp
	src/OffsetsWorkspace.cpp
	src/Peak.cpp
	src/PeakColumn.cpp
	src/PeaksWorkspace.cpp
	src/SpecialWorkspace2D.cpp
	src/TableColumn.cpp
	src/TableWorkspace.cpp
	src/Workspace2D.cpp
	src/WorkspaceSingleValue.cpp
)

set ( SRC_UNITY_IGNORE_FILES 
	src/Workspace2D.cpp
	src/WorkspaceSingleValue.cpp
	src/EventWorkspace.cpp
)

set ( INC_FILES
	inc/MantidDataObjects/AbsManagedWorkspace2D.h
	inc/MantidDataObjects/CompressedWorkspace2D.h
	inc/MantidDataObjects/DllConfig.h
	inc/MantidDataObjects/EventList.h
	inc/MantidDataObjects/EventWorkspace.h
	inc/MantidDataObjects/EventWorkspaceHelpers.h
	inc/MantidDataObjects/EventWorkspaceMRU.h
	inc/MantidDataObjects/Events.h
	inc/MantidDataObjects/GroupingWorkspace.h
	inc/MantidDataObjects/Histogram1D.h
	inc/MantidDataObjects/ManagedDataBlock2D.h
	inc/MantidDataObjects/ManagedHistogram1D.h
	inc/MantidDataObjects/ManagedWorkspace2D.h
	inc/MantidDataObjects/MementoTableWorkspace.h
	inc/MantidDataObjects/OffsetsWorkspace.h
	inc/MantidDataObjects/Peak.h
	inc/MantidDataObjects/PeakColumn.h
	inc/MantidDataObjects/PeaksWorkspace.h
	inc/MantidDataObjects/SpecialWorkspace2D.h
	inc/MantidDataObjects/TableColumn.h
	inc/MantidDataObjects/TableWorkspace.h
	inc/MantidDataObjects/Workspace2D.h
	inc/MantidDataObjects/WorkspaceSingleValue.h
)

set ( TEST_FILES 
	test/CompressedWorkspace2DTest.h
	test/EventListTest.h
	test/EventWorkspaceMRUTest.h
	test/EventWorkspaceTest.h
	test/GroupingWorkspaceTest.h
	test/Histogram1DTest.h
	test/LibraryManagerTest.h
	test/ManagedDataBlock2DTest.h
	test/ManagedHistogram1DTest.h
	test/ManagedWorkspace2DTest.h
	test/MementoTableWorkspaceTest.h
	test/OffsetsWorkspaceTest.h
	test/PeakColumnTest.h
	test/PeakTest.h
	test/PeaksWorkspaceTest.h
	test/RefAxisTest.h
	test/SpecialWorkspace2DTest.h
	test/TableWorkspacePropertyTest.h
	test/TableWorkspaceTest.h
	test/TofEventTest.h
	test/WeightedEventNoTimeTest.h
	test/WeightedEventTest.h
	test/Workspace2DTest.h
	test/WorkspaceIteratorTest.h
	test/WorkspaceSingleValueTest.h
	test/WorkspaceValidatorsTest.h
)


if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(DataObjects SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidDataObjects/PrecompiledHeader.h MantidDataObjects src/PrecompiledHeader.cpp SRC_FILES INC_FILES )
# Add the target for this directory
add_library ( DataObjects ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( DataObjects PROPERTIES OUTPUT_NAME MantidDataObjects 
                                               COMPILE_DEFINITIONS IN_MANTID_DATA_OBJECTS )
# Add to the 'Framework' group in VS
set_property ( TARGET DataObjects PROPERTY FOLDER "MantidFramework" )

include_directories ( ${ZLIB_INCLUDE_DIRS} )

target_link_libraries ( DataObjects ${MANTIDLIBS} ${ZLIB_LIBRARIES} )

if ( CXXTEST_FOUND )
  include_directories ( ../TestHelpers/inc )
  # This variable is used within the cxxtest_add_test macro to build this helper class into the test executable.
  # It will go out of scope at the end of this file so doesn't need un-setting
  set ( TESTHELPER_SRCS ../TestHelpers/src/ComponentCreationHelper.cpp ../TestHelpers/src/WorkspaceCreationHelper.cpp )
  cxxtest_add_test ( DataObjectsTest ${TEST_FILES} )
  target_link_libraries( DataObjectsTest DataObjects )
  # Specify implicit dependency, but don't link to it
  add_dependencies ( DataObjectsTest UserAlgorithms )
  add_dependencies ( FrameworkTests DataObjectsTest )
  # Add to the 'FrameworkTests' group in VS
  set_property ( TARGET DataObjectsTest PROPERTY FOLDER "UnitTests" )
endif ()

###########################################################################
# Installation settings
###########################################################################

# This MUST go to here rather than plugins for the Mac install to work
install ( TARGETS DataObjects DESTINATION ${LIB_DIR} )
