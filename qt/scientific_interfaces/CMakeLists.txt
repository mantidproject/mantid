# Ugly hack to get around issue in Boost version 1.48.0 in conjunction with Qt
# 4.7.4 or greater
set(qt_version ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH})
if(${Boost_VERSION} GREATER 104799 AND ${qt_version} VERSION_GREATER 4.7.3)
  set(extra_options "-DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED")
else(${Boost_VERSION} GREATER 104799 AND ${qt_version} VERSION_GREATER 4.7.3)
  set(extra_options "")
endif(${Boost_VERSION} GREATER 104799 AND ${qt_version} VERSION_GREATER 4.7.3)

# Now add the packages one-by-one, building up the dependencies as we go
add_subdirectory(General)
add_subdirectory(MultiDatasetFit)
add_subdirectory(Muon)
add_subdirectory(ISISSANS)
add_subdirectory(ISISReflectometry)
add_subdirectory(Indirect)
add_subdirectory(EnggDiffraction)
add_subdirectory(DynamicPDF)

# Testing - These should be split out by library
set(TEST_FILES
    test/ALCBaselineModellingModelTest.h
    test/ALCBaselineModellingPresenterTest.h
    test/ALCDataLoadingPresenterTest.h
    test/ALCLatestFileFinderTest.h
    test/ALCPeakFittingModelTest.h
    test/ALCPeakFittingPresenterTest.h
    test/EnggDiffGSASFittingModelTest.h
    test/EnggDiffGSASFittingPresenterTest.h
    test/EnggDiffMultiRunFittingWidgetModelTest.h
    test/EnggDiffMultiRunFittingWidgetPresenterTest.h
    test/EnggDiffFittingModelTest.h
    test/EnggDiffFittingPresenterTest.h
    test/EnggDiffractionPresenterTest.h
    test/EnggVanadiumCorrectionsModelTest.h
    test/IO_MuonGroupingTest.h
    test/MDFLogValueFinderTest.h
    test/MeasurementItemTest.h
    test/MuonAnalysisDataLoaderTest.h
    test/MuonAnalysisFitDataPresenterTest.h
    test/MuonAnalysisFitFunctionPresenterTest.h
    test/MuonAnalysisHelperTest.h
    test/MuonAnalysisResultTableCreatorTest.h
    test/ReflDataProcessorPresenterTest.h
    test/ReflEventPresenterTest.h
    test/ReflEventTabPresenterTest.h
    test/ReflLegacyTransferStrategyTest.h
    test/ReflMainWindowPresenterTest.h
    test/ReflMeasureTransferStrategyTest.h
    test/ReflNexusMeasurementItemSourceTest.h
    test/ReflRunsTabPresenterTest.h
    test/ReflSaveTabPresenterTest.h
    test/ReflSettingsPresenterTest.h
    test/ReflSettingsTabPresenterTest.h
    test/RunMapTest.h)

mtd_add_qt_tests(TARGET_NAME MantidQtScientificInterfacesTest
                 QT_VERSION 4
                 SRC ${TEST_FILES}
                 INCLUDE_DIRS
                   ../../Framework/DataObjects/inc
                   ../../Framework/TestHelpers/inc
                   inc
                 TEST_HELPER_SRCS
                   ../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
                   ../../Framework/TestHelpers/src/DataProcessorTestHelper.cpp
                   ../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
                   ../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
                   ../../Framework/TestHelpers/src/ScopedFileHelper.cpp
                   ../../Framework/TestHelpers/src/TearDownWorld.cpp
                 LINK_LIBS
                   ${TCMALLOC_LIBRARIES_LINKTIME}
                   ${CORE_MANTIDLIBS}
                   DataObjects
                   ${GMOCK_LIBRARIES}
                   ${GTEST_LIBRARIES}
                   ${POCO_LIBRARIES}
                   ${Boost_LIBRARIES}
                 QT4_LINK_LIBS
                   Qwt5
                 MTD_QT_LINK_LIBS
                   MantidScientificInterfacesGeneral
                   MantidScientificInterfacesMuon
                   MantidScientificInterfacesISISReflectometry
                   MantidScientificInterfacesEnggDiffraction
                   MantidScientificInterfacesMultiDatasetFit
                   MantidQtWidgetsCommon
                 PARENT_DEPENDENCIES
                   GUITests)
