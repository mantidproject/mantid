# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required( VERSION 2.6 )

project( VatesAPI )

set( SRC_FILES 
src/ADSWorkspaceProvider.cpp
src/Clipper.cpp
src/Common.cpp
src/ConcretePeaksPresenterVsi.cpp
src/DimensionPresenter.cpp
src/EscalatingRebinningActionManager.cpp
src/EventNexusLoadingPresenter.cpp
src/FieldDataToMetadata.cpp
src/IgnoreZerosThresholdRange.cpp
src/IMDDimensionComparitor.cpp
src/LoadVTK.cpp
src/MDEWEventNexusLoadingPresenter.cpp
src/MDEWRebinningPresenter.cpp
src/MDEWLoadingPresenter.cpp
src/MDEWInMemoryLoadingPresenter.cpp
src/MDHWInMemoryLoadingPresenter.cpp
src/MDHWLoadingPresenter.cpp
src/MDHWNexusLoadingPresenter.cpp
src/MedianAndBelowThresholdRange.cpp
src/MetadataToFieldData.cpp
src/MetadataToFieldData.cpp
src/MetaDataExtractorUtils.cpp
src/MetadataJsonManager.cpp
src/NoThresholdRange.cpp
src/NullRebinningPresenter.cpp
src/ProgressAction.cpp
src/RebinningCutterXMLDefinitions.cpp
src/RebinningKnowledgeSerializer.cpp
src/SynchronisingGeometryPresenter.cpp
src/TimeStepToTimeStep.cpp
src/TimeToTimeStep.cpp
src/UserDefinedThresholdRange.cpp
src/VatesConfigurations.cpp
src/ViewFrustum.cpp
src/vtkDataSetFactory.cpp
src/vtkDataSetToGeometry.cpp
src/vtkDataSetToImplicitFunction.cpp
src/vtkDataSetToNonOrthogonalDataSet.cpp
src/vtkDataSetToPeaksFilteredDataSet.cpp
src/vtkDataSetToScaledDataSet.cpp
src/vtkDataSetToWsName.cpp
src/vtkDataSetToWsLocation.cpp
src/vtkMDLineFactory.cpp
src/vtkMDQuadFactory.cpp
src/vtkSplatterPlotFactory.cpp
src/vtkMDHexFactory.cpp
src/vtkPeakMarkerFactory.cpp
src/vtkMDHistoHexFactory.cpp
src/vtkMDHistoLineFactory.cpp
src/vtkMDHistoQuadFactory.cpp
src/vtkMDHistoHex4DFactory.cpp
src/SQWLoadingPresenter.cpp
)

set( INC_FILES 
inc/MantidVatesAPI/ADSWorkspaceProvider.h
inc/MantidVatesAPI/Clipper.h
inc/MantidVatesAPI/Common.h
inc/MantidVatesAPI/ConcretePeaksPresenterVsi.h
inc/MantidVatesAPI/DimensionPresenter.h
inc/MantidVatesAPI/DimensionView.h
inc/MantidVatesAPI/DimensionViewFactory.h
inc/MantidVatesAPI/EscalatingRebinningActionManager.h
inc/MantidVatesAPI/EventNexusLoadingPresenter.h
inc/MantidVatesAPI/FieldDataToMetadata.h
inc/MantidVatesAPI/FilteringUpdateProgressAction.h
inc/MantidVatesAPI/GeometryPresenter.h
inc/MantidVatesAPI/GeometryView.h
inc/MantidVatesAPI/LoadVTK.h
inc/MantidVatesAPI/MDEWEventNexusLoadingPresenter.h
inc/MantidVatesAPI/MDEWLoadingPresenter.h
inc/MantidVatesAPI/MDEWInMemoryLoadingPresenter.h
inc/MantidVatesAPI/MDEWRebinningPresenter.h
inc/MantidVatesAPI/MDHWInMemoryLoadingPresenter.h
inc/MantidVatesAPI/MDHWLoadingPresenter.h
inc/MantidVatesAPI/MDHWNexusLoadingPresenter.h
inc/MantidVatesAPI/MDLoadingPresenter.h
inc/MantidVatesAPI/MDLoadingView.h
inc/MantidVatesAPI/MDLoadingViewAdapter.h
inc/MantidVatesAPI/MDRebinningPresenter.h
inc/MantidVatesAPI/MDRebinningView.h
inc/MantidVatesAPI/MDRebinningViewAdapter.h
inc/MantidVatesAPI/MedianAndBelowThresholdRange.h
inc/MantidVatesAPI/MetaDataExtractorUtils.h
inc/MantidVatesAPI/MetadataJsonManager.h
inc/MantidVatesAPI/IgnoreZerosThresholdRange.h
inc/MantidVatesAPI/IMDDimensionComparitor.h
inc/MantidVatesAPI/MetadataToFieldData.h
inc/MantidVatesAPI/NoThresholdRange.h
inc/MantidVatesAPI/NullRebinningPresenter.h
inc/MantidVatesAPI/NullPeaksPresenterVsi.h
inc/MantidVatesAPI/PeaksPresenterVsi.h
inc/MantidVatesAPI/ProgressAction.h
inc/MantidVatesAPI/RebinningActionManager.h
inc/MantidVatesAPI/RebinningCutterXMLDefinitions.h 
inc/MantidVatesAPI/RebinningKnowledgeSerializer.h
inc/MantidVatesAPI/SQWLoadingPresenter.h
inc/MantidVatesAPI/SynchronisingGeometryPresenter.h
inc/MantidVatesAPI/ThresholdRange.h
inc/MantidVatesAPI/TimeStepToTimeStep.h
inc/MantidVatesAPI/TimeToTimeStep.h
inc/MantidVatesAPI/UserDefinedThresholdRange.h
inc/MantidVatesAPI/VatesConfigurations.h
inc/MantidVatesAPI/ViewFrustum.h
inc/MantidVatesAPI/vtkDataSetFactory.h
inc/MantidVatesAPI/vtkDataSetToGeometry.h
inc/MantidVatesAPI/vtkDataSetToImplicitFunction.h
inc/MantidVatesAPI/vtkDataSetToNonOrthogonalDataSet.h
inc/MantidVatesAPI/vtkDataSetToPeaksFilteredDataSet.h
inc/MantidVatesAPI/vtkDataSetToScaledDataSet.h
inc/MantidVatesAPI/vtkDataSetToWsName.h
inc/MantidVatesAPI/vtkDataSetToWsLocation.h
inc/MantidVatesAPI/vtkMDLineFactory.h
inc/MantidVatesAPI/vtkMDQuadFactory.h
inc/MantidVatesAPI/vtkMDHexFactory.h
inc/MantidVatesAPI/vtkSplatterPlotFactory.h
inc/MantidVatesAPI/vtkPeakMarkerFactory.h
inc/MantidVatesAPI/vtkMDHistoHexFactory.h
inc/MantidVatesAPI/vtkMDHistoLineFactory.h
inc/MantidVatesAPI/vtkMDHistoQuadFactory.h
inc/MantidVatesAPI/vtkMDHistoHex4DFactory.h
inc/MantidVatesAPI/WorkspaceProvider.h
)

set( TEST_FILES
test/vtkDataSetToImplicitFunctionTest.h
test/vtkDataSetToWsNameTest.h
test/vtkDataSetToWsLocationTest.h
test/ADSWorkspaceProviderTest.h
test/DimensionPresenterTest.h
test/EscalatingRebinningActionManagerTest.h
test/EventNexusLoadingPresenterTest.h
test/vtkDataSetFactoryTest.h
test/vtkDataSetToGeometryTest.h
test/vtkMDLineFactoryTest.h
test/vtkMDQuadFactoryTest.h
test/vtkMDHexFactoryTest.h
test/vtkSplatterPlotFactoryTest.h
test/vtkPeakMarkerFactoryTest.h
test/vtkMDHistoHex4DFactoryTest.h
test/vtkMDHistoHexFactoryTest.h
test/vtkMDHistoLineFactoryTest.h
test/vtkMDHistoQuadFactoryTest.h
test/FieldDataToMetadataTest.h
test/FilteringUpdateProgressActionTest.h
test/LoadVTKTest.h
test/MDLoadingViewAdapterTest.h
test/MDRebinningViewAdapterTest.h
test/MDEWEventNexusLoadingPresenterTest.h
test/MDEWInMemoryLoadingPresenterTest.h
test/MDEWLoadingPresenterTest.h
test/MDEWRebinningPresenterTest.h
test/MDHWInMemoryLoadingPresenterTest.h
test/MDHWLoadingPresenterTest.h
test/MDHWNexusLoadingPresenterTest.h
test/MetaDataExtractorUtilsTest.h
test/MetadataJsonManagerTest.h
test/NullRebinningPresenterTest.h
test/MetadataToFieldDataTest.h
test/RebinningKnowledgeSerializerTest.h
test/SQWLoadingPresenterTest.h
test/SynchronisingGeometryPresenterTest.h
test/TimeStepToTimeStepTest.h
test/TimeToTimeStepTest.h
test/UserDefinedThresholdRangeTest.h
test/MedianAndBelowThresholdRangeTest.h
test/NoThresholdRangeTest.h
test/IgnoreZerosThresholdRangeTest.h
test/ViewFrustumTest.h
test/vtkDataSetToScaledDataSetTest.h
test/vtkDataSetToPeaksFilteredDataSetTest.h
test/vtkDataSetToNonOrthogonalDataSetTest.h
)

include_directories( inc )
include_directories ( SYSTEM ${QWT_INCLUDE_DIR} )

set( SRC_UNITY_IGNORE_FILES )
if( UNITY_BUILD )
  include( UnityBuild )
  enable_unity_build( VatesAPI SRC_FILES SRC_UNITY_IGNORE_FILES 10 )
endif( UNITY_BUILD )

# For Windows:
add_definitions( -DIN_MANTID_VATESAPI )
# Use a precompiled header where they are supported
enable_precompiled_headers ( inc/MantidVatesAPI/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library( VatesAPI ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties( VatesAPI PROPERTIES OUTPUT_NAME MantidVatesAPI )
# Add to the 'Framework' group in VS
set_property( TARGET VatesAPI PROPERTY FOLDER "MantidVates" )


target_link_libraries( VatesAPI
${MANTID_SUBPROJECT_LIBS} 
vtkCommonCore
vtkCommonDataModel
vtkIOLegacy
vtkFiltersExtraction
vtkFiltersSources
vtkPVVTKExtensionsDefault
${QWT_LIBRARIES}
)

# Create test file projects
if( CXXTEST_FOUND AND GMOCK_FOUND AND GTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

  include_directories( inc ../../Framework/TestHelpers/inc ../../Framework/DataHandling/inc ../../Framework/DataObjects/inc)
  set ( TESTHELPER_SRCS ../../Framework/TestHelpers/src/ComponentCreationHelper.cpp 
                        ../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
                        ../../Framework/TestHelpers/src/MDEventsTestHelper.cpp 
                        ../../Framework/TestHelpers/src/StartFrameworkManager.cpp )
  cxxtest_add_test( VatesAPITest ${TEST_FILES} )
  target_link_libraries( VatesAPITest VatesAPI DataHandling Kernel DataObjects ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} )
  add_dependencies( AllTests VatesAPITest )
  # Add to the 'UnitTests' group in VS
  set_property ( TARGET VatesAPITest PROPERTY FOLDER "UnitTests" )
endif()

install( TARGETS VatesAPI ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
