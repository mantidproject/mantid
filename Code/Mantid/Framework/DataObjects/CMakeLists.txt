set ( SRC_FILES
	src/EventList.cpp
	src/EventWorkspace.cpp
	src/EventWorkspaceHelpers.cpp
	src/EventWorkspaceMRU.cpp
	src/Events.cpp
	src/GroupingWorkspace.cpp
	src/Histogram1D.cpp
	src/MaskWorkspace.cpp
	src/MementoTableWorkspace.cpp
	src/NoShape.cpp
	src/OffsetsWorkspace.cpp
	src/Peak.cpp
	src/PeakColumn.cpp
	src/PeakNoShapeFactory.cpp
	src/PeakShapeBase.cpp
	src/PeakShapeSpherical.cpp
	src/PeakShapeSphericalFactory.cpp
	src/PeaksWorkspace.cpp
	src/RebinnedOutput.cpp
	src/SpecialWorkspace2D.cpp
	src/SplittersWorkspace.cpp
	src/TableColumn.cpp
	src/TableWorkspace.cpp
	src/VectorColumn.cpp
	src/Workspace2D.cpp
	src/WorkspaceSingleValue.cpp
)

set ( SRC_UNITY_IGNORE_FILES 
	src/Workspace2D.cpp
	src/WorkspaceSingleValue.cpp
	src/EventWorkspace.cpp
)

set ( INC_FILES
	inc/MantidDataObjects/DllConfig.h
	inc/MantidDataObjects/EventList.h
	inc/MantidDataObjects/EventWorkspace.h
	inc/MantidDataObjects/EventWorkspaceHelpers.h
	inc/MantidDataObjects/EventWorkspaceMRU.h
	inc/MantidDataObjects/Events.h
	inc/MantidDataObjects/GroupingWorkspace.h
	inc/MantidDataObjects/Histogram1D.h
	inc/MantidDataObjects/MaskWorkspace.h
	inc/MantidDataObjects/MementoTableWorkspace.h
	inc/MantidDataObjects/NoShape.h
	inc/MantidDataObjects/OffsetsWorkspace.h
	inc/MantidDataObjects/Peak.h
	inc/MantidDataObjects/PeakColumn.h
	inc/MantidDataObjects/PeakNoShapeFactory.h
	inc/MantidDataObjects/PeakShape.h
	inc/MantidDataObjects/PeakShapeBase.h
	inc/MantidDataObjects/PeakShapeFactory.h
	inc/MantidDataObjects/PeakShapeSpherical.h
	inc/MantidDataObjects/PeakShapeSphericalFactory.h
	inc/MantidDataObjects/PeaksWorkspace.h
	inc/MantidDataObjects/RebinnedOutput.h
	inc/MantidDataObjects/SpecialWorkspace2D.h
	inc/MantidDataObjects/SplittersWorkspace.h
	inc/MantidDataObjects/TableColumn.h
	inc/MantidDataObjects/TableWorkspace.h
	inc/MantidDataObjects/VectorColumn.h
	inc/MantidDataObjects/Workspace2D.h
	inc/MantidDataObjects/WorkspaceSingleValue.h
)

set ( TEST_FILES
	EventListTest.h
	EventWorkspaceMRUTest.h
	EventWorkspaceTest.h
	EventsTest.h
	GroupingWorkspaceTest.h
	Histogram1DTest.h
	LibraryManagerTest.h
	MaskWorkspaceTest.h
	MementoTableWorkspaceTest.h
	NoShapeTest.h
	OffsetsWorkspaceTest.h
	PeakColumnTest.h
	PeakNoShapeFactoryTest.h
	PeakShapeSphericalFactoryTest.h
	PeakShapeSphericalTest.h
	PeakTest.h
	PeaksWorkspaceTest.h
	RebinnedOutputTest.h
	RefAxisTest.h
	SpecialWorkspace2DTest.h
	SplittersWorkspaceTest.h
	TableColumnTest.h
	TableWorkspacePropertyTest.h
	TableWorkspaceTest.h
	TofEventTest.h
	VectorColumnTest.h
	WeightedEventNoTimeTest.h
	WeightedEventTest.h
	Workspace2DTest.h
	WorkspaceSingleValueTest.h
	WorkspaceValidatorsTest.h
)

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(DataObjects SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidDataObjects/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library ( DataObjects ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( DataObjects PROPERTIES OUTPUT_NAME MantidDataObjects 
                                               COMPILE_DEFINITIONS IN_MANTID_DATAOBJECTS 
                                               INSTALL_RPATH "@loader_path/../MacOS")
                                               
# Add to the 'Framework' group in VS
set_property ( TARGET DataObjects PROPERTY FOLDER "MantidFramework" )

target_link_libraries ( DataObjects ${MANTIDLIBS} )

# Add the unit tests directory
add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

if ( WIN32 )
  install ( TARGETS DataObjects ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
else()
  install ( TARGETS DataObjects DESTINATION ${LIB_DIR} )
endif ()


