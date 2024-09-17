set(SRC_FILES
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
    src/FakeMD.cpp
    src/FractionalRebinning.cpp
    src/GroupingWorkspace.cpp
    src/Histogram1D.cpp
    src/MDBoxFlatTree.cpp
    src/MDBoxSaveable.cpp
    src/MDEventFactory.cpp
    src/MDFramesToSpecialCoordinateSystem.cpp
    src/MDHistoWorkspace.cpp
    src/MDHistoWorkspaceIterator.cpp
    src/MDLeanEvent.cpp
    src/MaskWorkspace.cpp
    src/MementoTableWorkspace.cpp
    src/NoShape.cpp
    src/OffsetsWorkspace.cpp
    src/Peak.cpp
    src/LeanElasticPeak.cpp
    src/BasePeak.cpp
    src/PeakColumn.cpp
    src/PeakNoShapeFactory.cpp
    src/PeakShapeBase.cpp
    src/PeakShapeEllipsoid.cpp
    src/PeakShapeEllipsoidFactory.cpp
    src/PeakShapeSpherical.cpp
    src/PeakShapeSphericalFactory.cpp
    src/PeaksWorkspace.cpp
    src/LeanElasticPeaksWorkspace.cpp
    src/PropertyWithValue.cpp
    src/RebinnedOutput.cpp
    src/ReflectometryTransform.cpp
    src/ScanningWorkspaceBuilder.cpp
    src/SpecialWorkspace2D.cpp
    src/SplittersWorkspace.cpp
    src/TableColumn.cpp
    src/TableWorkspace.cpp
    src/TableWorkspaceNotEmptyValidator.cpp
    src/TimeSplitter.cpp
    src/VectorColumn.cpp
    src/Workspace2D.cpp
    src/WorkspaceCreation.cpp
    src/WorkspaceProperty.cpp
    src/WorkspaceSingleValue.cpp
)

set(INC_FILES
    inc/MantidDataObjects/AffineMatrixParameter.h
    inc/MantidDataObjects/AffineMatrixParameterParser.h
    inc/MantidDataObjects/BoxControllerNeXusIO.h
    inc/MantidDataObjects/CalculateReflectometry.h
    inc/MantidDataObjects/CalculateReflectometryKiKf.h
    inc/MantidDataObjects/CalculateReflectometryP.h
    inc/MantidDataObjects/CalculateReflectometryQxQz.h
    inc/MantidDataObjects/CoordTransformAffine.h
    inc/MantidDataObjects/CoordTransformAffineParser.h
    inc/MantidDataObjects/CoordTransformAligned.h
    inc/MantidDataObjects/CoordTransformDistance.h
    inc/MantidDataObjects/CoordTransformDistanceParser.h
    inc/MantidDataObjects/EventList.h
    inc/MantidDataObjects/EventWorkspace.h
    inc/MantidDataObjects/EventWorkspace_fwd.h
    inc/MantidDataObjects/EventWorkspaceHelpers.h
    inc/MantidDataObjects/EventWorkspaceMRU.h
    inc/MantidDataObjects/Events.h
    inc/MantidDataObjects/FakeMD.h
    inc/MantidDataObjects/FractionalRebinning.h
    inc/MantidDataObjects/GroupingWorkspace.h
    inc/MantidDataObjects/Histogram1D.h
    inc/MantidDataObjects/MDBin.h
    inc/MantidDataObjects/MDBin.tcc
    inc/MantidDataObjects/MDBox.h
    inc/MantidDataObjects/MDBox.tcc
    inc/MantidDataObjects/MDBoxBase.h
    inc/MantidDataObjects/MDBoxBase.tcc
    inc/MantidDataObjects/MDBoxFlatTree.h
    inc/MantidDataObjects/MDBoxIterator.h
    inc/MantidDataObjects/MDBoxIterator.tcc
    inc/MantidDataObjects/MDBoxSaveable.h
    inc/MantidDataObjects/MDDimensionStats.h
    inc/MantidDataObjects/MDEvent.h
    inc/MantidDataObjects/MDEventFactory.h
    inc/MantidDataObjects/MDEventInserter.h
    inc/MantidDataObjects/MDEventWorkspace.h
    inc/MantidDataObjects/MDEventWorkspace.tcc
    inc/MantidDataObjects/MDFramesToSpecialCoordinateSystem.h
    inc/MantidDataObjects/MDGridBox.h
    inc/MantidDataObjects/MDGridBox.tcc
    inc/MantidDataObjects/MDHistoWorkspace.h
    inc/MantidDataObjects/MDHistoWorkspaceIterator.h
    inc/MantidDataObjects/MDLeanEvent.h
    inc/MantidDataObjects/MaskWorkspace.h
    inc/MantidDataObjects/MortonIndex/BitInterleaving.h
    inc/MantidDataObjects/MortonIndex/CoordinateConversion.h
    inc/MantidDataObjects/MortonIndex/Types.h
    inc/MantidDataObjects/MementoTableWorkspace.h
    inc/MantidDataObjects/NoShape.h
    inc/MantidDataObjects/OffsetsWorkspace.h
    inc/MantidDataObjects/Peak.h
    inc/MantidDataObjects/LeanElasticPeak.h
    inc/MantidDataObjects/BasePeak.h
    inc/MantidDataObjects/PeakColumn.h
    inc/MantidDataObjects/PeakNoShapeFactory.h
    inc/MantidDataObjects/PeakShapeBase.h
    inc/MantidDataObjects/PeakShapeEllipsoid.h
    inc/MantidDataObjects/PeakShapeEllipsoidFactory.h
    inc/MantidDataObjects/PeakShapeFactory.h
    inc/MantidDataObjects/PeakShapeSpherical.h
    inc/MantidDataObjects/PeakShapeSphericalFactory.h
    inc/MantidDataObjects/PeaksWorkspace.h
    inc/MantidDataObjects/LeanElasticPeaksWorkspace.h
    inc/MantidDataObjects/RebinnedOutput.h
    inc/MantidDataObjects/ReflectometryTransform.h
    inc/MantidDataObjects/ScanningWorkspaceBuilder.h
    inc/MantidDataObjects/SkippingPolicy.h
    inc/MantidDataObjects/SpecialWorkspace2D.h
    inc/MantidDataObjects/SplittersWorkspace.h
    inc/MantidDataObjects/TableColumn.h
    inc/MantidDataObjects/TableWorkspace.h
    inc/MantidDataObjects/TableWorkspaceValidator.h
    inc/MantidDataObjects/TableWorkspaceNotEmptyValidator.h
    inc/MantidDataObjects/TableWorkspace_fwd.h
    inc/MantidDataObjects/TimeSplitter.h
    inc/MantidDataObjects/VectorColumn.h
    inc/MantidDataObjects/Workspace2D.h
    inc/MantidDataObjects/Workspace2D_fwd.h
    inc/MantidDataObjects/WorkspaceCreation.h
    inc/MantidDataObjects/WorkspaceSingleValue.h
)

set(TEST_FILES
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
    FakeMDTest.h
    GroupingWorkspaceTest.h
    Histogram1DTest.h
    MDBinTest.h
    MDBoxBaseTest.h
    MDBoxFlatTreeTest.h
    MDBoxIteratorTest.h
    MDBoxSaveableTest.h
    MDBoxTest.h
    MDDimensionStatsTest.h
    MDEventFactoryTest.h
    MDEventInserterTest.h
    MDEventTest.h
    MDEventWorkspaceTest.h
    MDFramesToSpecialCoordinateSystemTest.h
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
    LeanElasticPeakTest.h
    PeaksWorkspaceTest.h
    LeanElasticPeaksWorkspaceTest.h
    RebinnedOutputTest.h
    RefAxisTest.h
    ReflectometryTransformTest.h
    ScanningWorkspaceBuilderTest.h
    SkippingPolicyTest.h
    SpecialWorkspace2DTest.h
    SplittersWorkspaceTest.h
    TableColumnTest.h
    TableWorkspacePropertyTest.h
    TableWorkspaceTest.h
    TimeSplitterTest.h
    VectorColumnTest.h
    WeightedEventNoTimeTest.h
    WeightedEventTest.h
    Workspace2DTest.h
    WorkspaceCreationTest.h
    WorkspaceSingleValueTest.h
    WorkspaceValidatorsTest.h
    MortonIndex/BitInterleavingTest.h
)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Use a precompiled header where they are supported
enable_precompiled_headers(inc/MantidDataObjects/PrecompiledHeader.h SRC_FILES)
# Add the target for this directory
add_library(DataObjects ${SRC_FILES} ${INC_FILES})
add_library(Mantid::DataObjects ALIAS DataObjects)

target_include_directories(
  DataObjects PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

# Set the name of the generated library
set_target_properties(DataObjects PROPERTIES OUTPUT_NAME MantidDataObjects COMPILE_DEFINITIONS IN_MANTID_DATAOBJECTS)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(DataObjects PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Frameworks")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(DataObjects PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET DataObjects PROPERTY FOLDER "MantidFramework")

target_link_libraries(
  DataObjects
  PUBLIC Mantid::API Mantid::Geometry Mantid::HistogramData Mantid::Kernel
  PRIVATE Mantid::Json Mantid::Indexing
)

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  DataObjects PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/DataObjects> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(DataObjects FALSE)

# Installation settings
set(TARGET_EXPORT_NAME "MantidDataObjectsTargets")
mtd_install_framework_lib(TARGETS DataObjects INSTALL_EXPORT_FILE EXPORT_NAME ${TARGET_EXPORT_NAME})
