# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required(VERSION 3.5)

project(VatesAPI)

set(SRC_FILES
    src/ADSWorkspaceProvider.cpp
    src/BoxInfo.cpp
    src/Common.cpp
    src/CompositePeaksPresenterVsi.cpp
    src/ConcretePeaksPresenterVsi.cpp
    src/EventNexusLoadingPresenter.cpp
    src/FieldDataToMetadata.cpp
    src/IMDDimensionComparitor.cpp
    src/MDEWEventNexusLoadingPresenter.cpp
    src/MDEWLoadingPresenter.cpp
    src/MDEWInMemoryLoadingPresenter.cpp
    src/MDHWInMemoryLoadingPresenter.cpp
    src/MDHWLoadingPresenter.cpp
    src/MDHWNexusLoadingPresenter.cpp
    src/MDLoadingViewSimple.cpp
    src/MetadataToFieldData.cpp
    src/MetadataToFieldData.cpp
    src/MetaDataExtractorUtils.cpp
    src/MetadataJsonManager.cpp
    src/MDLoadingPresenter.cpp
    src/Normalization.cpp
    src/ProgressAction.cpp
    src/PresenterFactories.cpp
    src/PresenterUtilities.cpp
    src/SingleWorkspaceProvider.cpp
    src/TimeStepToTimeStep.cpp
    src/TimeToTimeStep.cpp
    src/VatesXMLDefinitions.cpp
    src/VatesConfigurations.cpp
    src/ViewFrustum.cpp
    src/VatesKnowledgeSerializer.cpp
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
    src/vtkMD0DFactory.cpp
    src/vtkNullStructuredGrid.cpp
    src/vtkNullUnstructuredGrid.cpp
    src/vtkSplatterPlotFactory.cpp
    src/vtkMDHexFactory.cpp
    src/vtkPeakMarkerFactory.cpp
    src/vtkMDHistoHexFactory.cpp
    src/vtkMDHistoLineFactory.cpp
    src/vtkMDHistoQuadFactory.cpp
    src/vtkMDHistoHex4DFactory.cpp
    src/vtkSinglePeakMarker.cpp
    src/SQWLoadingPresenter.cpp)

set(INC_FILES
    inc/MantidVatesAPI/ADSWorkspaceProvider.h
    inc/MantidVatesAPI/BoxInfo.h
    inc/MantidVatesAPI/Common.h
    inc/MantidVatesAPI/CompositePeaksPresenterVsi.h
    inc/MantidVatesAPI/ConcretePeaksPresenterVsi.h
    inc/MantidVatesAPI/ColorScaleGuard.h
    inc/MantidVatesAPI/DimensionViewFactory.h
    inc/MantidVatesAPI/EventNexusLoadingPresenter.h
    inc/MantidVatesAPI/FactoryChains.h
    inc/MantidVatesAPI/FieldDataToMetadata.h
    inc/MantidVatesAPI/FilteringUpdateProgressAction.h
    inc/MantidVatesAPI/GeometryView.h
    inc/MantidVatesAPI/MDEWEventNexusLoadingPresenter.h
    inc/MantidVatesAPI/MDEWLoadingPresenter.h
    inc/MantidVatesAPI/MDEWInMemoryLoadingPresenter.h
    inc/MantidVatesAPI/MDHWInMemoryLoadingPresenter.h
    inc/MantidVatesAPI/MDHWLoadingPresenter.h
    inc/MantidVatesAPI/MDHWNexusLoadingPresenter.h
    inc/MantidVatesAPI/MDLoadingPresenter.h
    inc/MantidVatesAPI/MDLoadingView.h
    inc/MantidVatesAPI/MDLoadingViewSimple.h
    inc/MantidVatesAPI/MDLoadingViewAdapter.h
    inc/MantidVatesAPI/MetaDataExtractorUtils.h
    inc/MantidVatesAPI/MetadataJsonManager.h
    inc/MantidVatesAPI/Normalization.h
    inc/MantidVatesAPI/IMDDimensionComparitor.h
    inc/MantidVatesAPI/MetadataToFieldData.h
    inc/MantidVatesAPI/NullPeaksPresenterVsi.h
    inc/MantidVatesAPI/PeaksPresenterVsi.h
    inc/MantidVatesAPI/PresenterFactories.h
    inc/MantidVatesAPI/ProgressAction.h
    inc/MantidVatesAPI/SingleWorkspaceProvider.h
    inc/MantidVatesAPI/SQWLoadingPresenter.h
    inc/MantidVatesAPI/TimeStepToTimeStep.h
    inc/MantidVatesAPI/TimeToTimeStep.h
    inc/MantidVatesAPI/VatesXMLDefinitions.h
    inc/MantidVatesAPI/VatesConfigurations.h
    inc/MantidVatesAPI/VatesKnowledgeSerializer.h
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
    inc/MantidVatesAPI/vtkMD0DFactory.h
    inc/MantidVatesAPI/vtkNullStructuredGrid.h
    inc/MantidVatesAPI/vtkNullUnstructuredGrid.h
    inc/MantidVatesAPI/vtkSplatterPlotFactory.h
    inc/MantidVatesAPI/vtkPeakMarkerFactory.h
    inc/MantidVatesAPI/vtkMDHistoHexFactory.h
    inc/MantidVatesAPI/vtkMDHistoLineFactory.h
    inc/MantidVatesAPI/vtkMDHistoQuadFactory.h
    inc/MantidVatesAPI/vtkMDHistoHex4DFactory.h
    inc/MantidVatesAPI/vtkSinglePeakMarker.h
    inc/MantidVatesAPI/WorkspaceProvider.h)

set(TEST_FILES
    test/vtkDataSetToImplicitFunctionTest.h
    test/vtkDataSetToWsNameTest.h
    test/vtkDataSetToWsLocationTest.h
    test/ADSWorkspaceProviderTest.h
    test/BoxInfoTest.h
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
    test/vtkMD0DFactoryTest.h
    test/FieldDataToMetadataTest.h
    test/FilteringUpdateProgressActionTest.h
    test/MDLoadingViewAdapterTest.h
    test/MDEWEventNexusLoadingPresenterTest.h
    test/MDEWInMemoryLoadingPresenterTest.h
    test/MDEWLoadingPresenterTest.h
    test/MDHWInMemoryLoadingPresenterTest.h
    test/MDHWLoadingPresenterTest.h
    test/MDHWNexusLoadingPresenterTest.h
    test/MDLoadingPresenterTest.h
    test/MDLoadingViewSimpleTest.h
    test/MetaDataExtractorUtilsTest.h
    test/MetadataJsonManagerTest.h
    test/MetadataToFieldDataTest.h
    test/NormalizationTest.h
    test/PresenterUtilitiesTest.h
    test/SingleWorkspaceProviderTest.h
    test/SQWLoadingPresenterTest.h
    test/TimeStepToTimeStepTest.h
    test/TimeToTimeStepTest.h
    test/VatesKnowledgeSerializerTest.h
    test/ViewFrustumTest.h
    test/vtkDataSetToScaledDataSetTest.h
    test/vtkDataSetToPeaksFilteredDataSetTest.h
    test/vtkDataSetToNonOrthogonalDataSetTest.h
    test/vtkNullUnstructuredGridTest.h
    test/vtkNullStructuredGridTest.h
    test/vtkMDHWSignalArrayTest.h
    test/NullPeaksPresenterVsiTest.h
    test/ConcretePeaksPresenterVsiTest.h
    test/CompositePeaksPresenterVsiTest.h)

include_directories(inc)
include_directories(SYSTEM ${PARAVIEW_INCLUDE_DIRS})

# For Windows:
add_definitions(-DIN_MANTID_VATESAPI)
# Use a precompiled header where they are supported
enable_precompiled_headers(inc/MantidVatesAPI/PrecompiledHeader.h SRC_FILES)
# Add the target for this directory
add_library(VatesAPI ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties(VatesAPI PROPERTIES OUTPUT_NAME MantidVatesAPI)
# Add to the 'Framework' group in VS
set_property(TARGET VatesAPI PROPERTY FOLDER "MantidVates")

target_link_libraries(VatesAPI
                      PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
                              ${MANTID_SUBPROJECT_LIBS}
                              vtkCommonCore
                              vtkCommonDataModel
                              vtkIOLegacy
                              vtkFiltersExtraction
                              vtkFiltersSources
                              ${vtkjsoncpp_LIBRARIES}
                              vtkPVVTKExtensionsDefault
                              ${POCO_LIBRARIES}
                              ${Boost_LIBRARIES})

if(MSVC)
  # To simplify developer builds from the IDE we copy the ParaView DLLs to the
  # appropriate buildbinary folder. This avoids have to set the PATH to point to
  # a different ParaView build directory for each configuration
  set(_src_dir ${ParaView_DIR}/bin/${CMAKE_CFG_INTDIR})
  set(_dest_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR})
  add_custom_command(
    TARGET VatesAPI POST_BUILD
    COMMAND xcopy
            ARGS \"${_src_dir}/*.dll\"
                 \"${_dest_dir}\"
                 /D
                 /Y
                 /F
    COMMENT "Updating ParaView DLLs in bin/${CMAKE_CFG_INTDIR}")
  unset(_dest_dir)
  unset(_src_dir)
endif()

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(
    VatesAPI
    PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Libraries")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(VatesAPI
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Create test file projects
include_directories(SYSTEM
                    ${CXXTEST_INCLUDE_DIR}
                    ${GMOCK_INCLUDE_DIR}
                    ${GTEST_INCLUDE_DIR})

include_directories(inc
                    ../../../Framework/TestHelpers/inc
                    ../../../Framework/DataHandling/inc
                    ../../../Framework/DataObjects/inc
                    ../../../Framework/MDAlgorithms/inc)
set(TESTHELPER_SRCS
    ../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
    ../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
    ../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
    ../../../Framework/TestHelpers/src/MDEventsTestHelper.cpp
    ../../../Framework/TestHelpers/src/StartFrameworkManager.cpp)
cxxtest_add_test(VatesAPITest ${TEST_FILES})
target_link_libraries(VatesAPITest
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${MANTID_SUBPROJECT_LIBS}
                      VatesAPI
                      DataHandling
                      Kernel
                      DataObjects
                      Indexing
                      vtkCommonCore
                      vtkCommonDataModel
                      vtkIOLegacy
                      vtkFiltersExtraction
                      vtkFiltersSources
                      vtkPVVTKExtensionsDefault
                      ${vtkjsoncpp_LIBRARIES}
                      ${POCO_LIBRARIES}
                      ${Boost_LIBRARIES}
                      ${GMOCK_LIBRARIES}
                      ${GTEST_LIBRARIES})
add_dependencies(AllTests VatesAPITest)
# Add to the 'UnitTests' group in VS
set_property(TARGET VatesAPITest PROPERTY FOLDER "UnitTests")

install(TARGETS VatesAPI ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR})
