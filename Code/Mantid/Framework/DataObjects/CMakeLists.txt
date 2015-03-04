set ( SRC_FILES
	src/AffineMatrixParameter.cpp
	src/AffineMatrixParameterParser.cpp
	src/BoxControllerNeXusIO.cpp
	src/CoordTransformAffine.cpp
	src/CoordTransformAffineParser.cpp
	src/CoordTransformAligned.cpp
	src/CoordTransformDistance.cpp
	src/CoordTransformDistanceParser.cpp
	src/EventList.cpp
	src/EventWorkspace.cpp
	src/EventWorkspaceHelpers.cpp
	src/EventWorkspaceMRU.cpp
	src/Events.cpp
	src/GroupingWorkspace.cpp
	src/Histogram1D.cpp
	src/MDBin.cpp
	src/MDBox.cpp
	src/MDBoxBase.cpp
	src/MDBoxFlatTree.cpp
	src/MDBoxIterator.cpp
	src/MDBoxSaveable.cpp
	src/MDEventFactory.cpp
	src/MDEventWorkspace.cpp
	src/MDGridBox.cpp
	src/MDHistoWorkspace.cpp
	src/MDHistoWorkspaceIterator.cpp
	src/MDLeanEvent.cpp
	src/MaskWorkspace.cpp
	src/MementoTableWorkspace.cpp
	src/NoShape.cpp
	src/OffsetsWorkspace.cpp
	src/Peak.cpp
	src/PeakColumn.cpp
	src/PeakNoShapeFactory.cpp
	src/PeakShapeBase.cpp
	src/PeakShapeEllipsoid.cpp
	src/PeakShapeEllipsoidFactory.cpp
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
	inc/MantidDataObjects/AffineMatrixParameter.h
	inc/MantidDataObjects/AffineMatrixParameterParser.h
	inc/MantidDataObjects/BoxControllerNeXusIO.h
	inc/MantidDataObjects/CoordTransformAffine.h
	inc/MantidDataObjects/CoordTransformAffineParser.h
	inc/MantidDataObjects/CoordTransformAligned.h
	inc/MantidDataObjects/CoordTransformDistance.h
	inc/MantidDataObjects/CoordTransformDistanceParser.h
	inc/MantidDataObjects/DllConfig.h
	inc/MantidDataObjects/EventList.h
	inc/MantidDataObjects/EventWorkspace.h
	inc/MantidDataObjects/EventWorkspaceHelpers.h
	inc/MantidDataObjects/EventWorkspaceMRU.h
	inc/MantidDataObjects/Events.h
	inc/MantidDataObjects/GroupingWorkspace.h
	inc/MantidDataObjects/Histogram1D.h
	inc/MantidDataObjects/MDBin.h
	inc/MantidDataObjects/MDBox.h
	inc/MantidDataObjects/MDBoxBase.h
	inc/MantidDataObjects/MDBoxFlatTree.h
	inc/MantidDataObjects/MDBoxIterator.h
	inc/MantidDataObjects/MDBoxSaveable.h
	inc/MantidDataObjects/MDDimensionStats.h
	inc/MantidDataObjects/MDEventFactory.h
	inc/MantidDataObjects/MDEventInserter.h
	inc/MantidDataObjects/MDEventWorkspace.h
	inc/MantidDataObjects/MDGridBox.h
	inc/MantidDataObjects/MDHistoWorkspace.h
	inc/MantidDataObjects/MDHistoWorkspaceIterator.h
	inc/MantidDataObjects/MDLeanEvent.h
	inc/MantidDataObjects/MaskWorkspace.h
	inc/MantidDataObjects/MementoTableWorkspace.h
	inc/MantidDataObjects/NoShape.h
	inc/MantidDataObjects/OffsetsWorkspace.h
	inc/MantidDataObjects/Peak.h
	inc/MantidDataObjects/PeakColumn.h
	inc/MantidDataObjects/PeakNoShapeFactory.h
	inc/MantidDataObjects/PeakShapeBase.h
	inc/MantidDataObjects/PeakShapeEllipsoid.h
	inc/MantidDataObjects/PeakShapeEllipsoidFactory.h
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
	AffineMatrixParameterParserTest.h
	AffineMatrixParameterTest.h
	BoxControllerNeXusIOTest.h
	CoordTransformAffineParserTest.h
	CoordTransformAffineTest.h
	CoordTransformAlignedTest.h
	CoordTransformDistanceParserTest.h
	CoordTransformDistanceTest.h
	EventListTest.h
	EventWorkspaceMRUTest.h
	EventWorkspaceTest.h
	EventsTest.h
	GroupingWorkspaceTest.h
	Histogram1DTest.h
	LibraryManagerTest.h
	MDBinTest.h
	MDBoxBaseTest.h
	MDBoxFlatTreeTest.h
	MDBoxIteratorTest.h
	MDBoxSaveableTest.h
	MDBoxTest.h
	MDDimensionStatsTest.h
	MDEventTest.h
	MDEventFactoryTest.h
	MDEventInserterTest.h
	MDEventWorkspaceTest.h
	MDGridBoxTest.h
	MDHistoWorkspaceIteratorTest.h
	MDHistoWorkspaceTest.h
	MDLeanEventTest.h
	MaskWorkspaceTest.h
	MementoTableWorkspaceTest.h
	NoShapeTest.h
	OffsetsWorkspaceTest.h
	PeakColumnTest.h
	PeakNoShapeFactoryTest.h
	PeakShapeEllipsoidFactoryTest.h
	PeakShapeEllipsoidTest.h
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
                                               COMPILE_DEFINITIONS IN_MANTID_DATAOBJECTS )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( DataObjects PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()
                                               
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


