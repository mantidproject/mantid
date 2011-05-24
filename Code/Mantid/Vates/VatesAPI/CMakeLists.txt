# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required( VERSION 2.6 )

project( VatesAPI )

set( SRC_FILES 
src/Clipper.cpp
src/Common.cpp
src/DimensionComparitor.cpp
src/EscalatingRebinningActionManager.cpp
src/FieldDataToMetadata.cpp
src/GeometryXMLParser.cpp
src/IMDWorkspaceProxy.cpp
src/MetadataToFieldData.cpp
src/MultiDimensionalDbPresenter.cpp
src/ProgressAction.cpp
src/RebinningCutterPresenter.cpp
src/RebinningCutterXMLDefinitions.cpp
src/RebinningXMLGenerator.cpp
src/TimeStepToTimeStep.cpp
src/TimeToTimeStep.cpp
src/vtkDataSetFactory.cpp
src/vtkDataSetToGeometry.cpp
src/vtkClipperDataSetFactory.cpp
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
inc/MantidVatesAPI/DimensionComparitor.h
inc/MantidVatesAPI/EscalatingRebinningActionManager.h
inc/MantidVatesAPI/FieldDataToMetadata.h
inc/MantidVatesAPI/FilteringUpdateProgressAction.h
inc/MantidVatesAPI/GeometryXMLParser.h
inc/MantidVatesAPI/IMDWorkspaceProxy.h
inc/MantidVatesAPI/MetadataToFieldData.h
inc/MantidVatesAPI/MultiDimensionalDbPresenter.h
inc/MantidVatesAPI/ProgressAction.h
inc/MantidVatesAPI/RebinningActionManager.h
inc/MantidVatesAPI/RebinningCutterPresenter.h
inc/MantidVatesAPI/RebinningCutterXMLDefinitions.h 
inc/MantidVatesAPI/RebinningXMLGenerator.h
inc/MantidVatesAPI/TimeStepToTimeStep.h
inc/MantidVatesAPI/TimeToTimeStep.h
inc/MantidVatesAPI/vtkDataSetFactory.h
inc/MantidVatesAPI/vtkDataSetToGeometry.h
inc/MantidVatesAPI/vtkClipperDataSetFactory.h
inc/MantidVatesAPI/vtkProxyFactory.h
inc/MantidVatesAPI/vtkStructuredGridFactory.h
inc/MantidVatesAPI/vtkThresholdingHexahedronFactory.h
inc/MantidVatesAPI/vtkThresholdingLineFactory.h
inc/MantidVatesAPI/vtkThresholdingQuadFactory.h
inc/MantidVatesAPI/vtkThresholdingUnstructuredGridFactory.h
)

set( TEST_FILES
test/EscalatingRebinningActionManagerTest.h
test/GeometryXMLParserTest.h
test/vtkDataSetFactoryTest.h
test/vtkClipperDataSetFactoryTest.h
test/vtkDataSetToGeometryTest.h
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
test/MetadataToFieldDataTest.h
test/RebinningCutterTest.h
test/RebinningXMLGeneratorTest.h
test/TimeStepToTimeStepTest.h
test/TimeToTimeStepTest.h
)

include_directories( ${ParaView_DIR}/VTK )
link_directories( ${ParaView_DIR}/bin ) # TODO: handle /bin/Debug bin/Release

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
)

# Create test file projects
if( CXXTEST_FOUND AND GMOCK_FOUND AND GTEST_FOUND )
  cxxtest_add_test( VatesAPITest ${TEST_FILES} )
  target_link_libraries( VatesAPITest VatesAPI ${GMOCK_LIBRARIES} 
      ${GTEST_LIBRARIES})
  add_dependencies( FrameworkTests VatesAPITest )
endif()
