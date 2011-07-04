# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required( VERSION 2.6 )

project( VatesAPI )

set( SRC_FILES 
src/Clipper.cpp
src/Common.cpp
src/DimensionPresenter.cpp
src/EscalatingRebinningActionManager.cpp
src/FieldDataToMetadata.cpp
src/GaussianThresholdRange.cpp
src/IMDDimensionComparitor.cpp
src/IMDWorkspaceProxy.cpp
src/MDHistogramRebinningPresenter.cpp
src/MetadataToFieldData.cpp
src/MultiDimensionalDbPresenter.cpp
src/NullRebinningPresenter.cpp
src/ProgressAction.cpp
src/RebinningCutterXMLDefinitions.cpp
src/RebinningKnowledgeSerializer.cpp
src/SynchronisingGeometryPresenter.cpp
src/TimeStepToTimeStep.cpp
src/TimeToTimeStep.cpp
src/UserDefinedThresholdRange.cpp
src/vtkDataSetFactory.cpp
src/vtkDataSetToGeometry.cpp
src/vtkClipperDataSetFactory.cpp
src/vtkPeakMarkerFactory.cpp
src/vtkProxyFactory.cpp
src/vtkStructuredGridFactory.cpp
src/vtkThresholdingHexahedronFactory.cpp
src/vtkThresholdingLineFactory.cpp
src/vtkThresholdingQuadFactory.cpp
src/vtkThresholdingUnstructuredGridFactory.cpp
)

set( INC_FILES 
inc/MantidVatesAPI/Clipper.h
inc/MantidVatesAPI/Common.h
inc/MantidVatesAPI/DimensionPresenter.h
inc/MantidVatesAPI/DimensionView.h
inc/MantidVatesAPI/DimensionViewFactory.h
inc/MantidVatesAPI/EscalatingRebinningActionManager.h
inc/MantidVatesAPI/FieldDataToMetadata.h
inc/MantidVatesAPI/FilteringUpdateProgressAction.h
inc/MantidVatesAPI/GaussianThresholdRange.h
inc/MantidVatesAPI/GeometryPresenter.h
inc/MantidVatesAPI/GeometryView.h
inc/MantidVatesAPI/MDHistogramRebinningPresenter.h
inc/MantidVatesAPI/MDRebinningPresenter.h
inc/MantidVatesAPI/MDRebinningView.h
inc/MantidVatesAPI/IMDDimensionComparitor.h
inc/MantidVatesAPI/IMDWorkspaceProxy.h
inc/MantidVatesAPI/MetadataToFieldData.h
inc/MantidVatesAPI/MultiDimensionalDbPresenter.h
inc/MantidVatesAPI/NullRebinningPresenter.h
inc/MantidVatesAPI/ProgressAction.h
inc/MantidVatesAPI/RebinningActionManager.h
inc/MantidVatesAPI/RebinningCutterXMLDefinitions.h 
inc/MantidVatesAPI/RebinningKnowledgeSerializer.h
inc/MantidVatesAPI/SynchronisingGeometryPresenter.h
inc/MantidVatesAPI/ThresholdRange.h
inc/MantidVatesAPI/TimeStepToTimeStep.h
inc/MantidVatesAPI/TimeToTimeStep.h
inc/MantidVatesAPI/UserDefinedThresholdRange.h
inc/MantidVatesAPI/vtkDataSetFactory.h
inc/MantidVatesAPI/vtkDataSetToGeometry.h
inc/MantidVatesAPI/vtkClipperDataSetFactory.h
inc/MantidVatesAPI/vtkPeakMarkerFactory.h
inc/MantidVatesAPI/vtkProxyFactory.h
inc/MantidVatesAPI/vtkStructuredGridFactory.h
inc/MantidVatesAPI/vtkThresholdingHexahedronFactory.h
inc/MantidVatesAPI/vtkThresholdingLineFactory.h
inc/MantidVatesAPI/vtkThresholdingQuadFactory.h
inc/MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h
)

set( TEST_FILES
test/DimensionPresenterTest.h
test/EscalatingRebinningActionManagerTest.h
test/vtkDataSetFactoryTest.h
test/vtkClipperDataSetFactoryTest.h
test/vtkDataSetToGeometryTest.h
test/vtkPeakMarkerFactoryTest.h
test/vtkProxyFactoryTest.h
test/vtkStructuredGridFactoryTest.h
test/vtkThresholdingUnstructuredGridFactoryTest.h
test/vtkThresholdingHexahedronFactoryTest.h
test/vtkThresholdingLineFactoryTest.h
test/vtkThresholdingQuadFactoryTest.h
test/FieldDataToMetadataTest.h
test/FilteringUpdateProgressActionTest.h
test/IMDWorkspaceProxyTest.h
test/MultiDimensionalDbPresenterTest.h
test/MDHistogramRebinningPresenterTest.h
test/NullRebinningPresenterTest.h
test/MetadataToFieldDataTest.h
test/RebinningKnowledgeSerializerTest.h
test/SynchronisingGeometryPresenterTest.h
test/TimeStepToTimeStepTest.h
test/TimeToTimeStepTest.h
test/UserDefinedThresholdRangeTest.h
test/GaussianThresholdRangeTest.h
)

include_directories( inc )

set( SRC_UNITY_IGNORE_FILES )
if( UNITY_BUILD )
  include( UnityBuild )
  enable_unity_build( VatesAPI SRC_FILES SRC_UNITY_IGNORE_FILES 10 )
endif( UNITY_BUILD )

# For Windows:
add_definitions( -DIN_MANTID_VATESAPI )
# Add the target for this directory
add_library( VatesAPI ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties( VatesAPI PROPERTIES OUTPUT_NAME MantidVatesAPI )
# Add to the 'Framework' group in VS
set_property( TARGET VatesAPI PROPERTY FOLDER "MantidFramework" )

target_link_libraries( VatesAPI
${MANTID_SUBPROJECT_LIBS} 
vtkCommon 
vtkFiltering
Crystal
)

# Create test file projects
if( CXXTEST_FOUND AND GMOCK_FOUND AND GTEST_FOUND )
  cxxtest_add_test( VatesAPITest ${TEST_FILES} )
  target_link_libraries( VatesAPITest VatesAPI ${GMOCK_LIBRARIES} 
      ${GTEST_LIBRARIES})
  add_dependencies( FrameworkTests VatesAPITest )
endif()

install( TARGETS VatesAPI ${TARGET_TYPE} DESTINATION ${LIB_DIR} )
