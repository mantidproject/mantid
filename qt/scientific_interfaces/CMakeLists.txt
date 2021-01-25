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
add_subdirectory(Direct)
add_subdirectory(MultiDatasetFit)
add_subdirectory(Muon)
add_subdirectory(ISISSANS)
add_subdirectory(ISISReflectometry)
add_subdirectory(Indirect)
add_subdirectory(DynamicPDF)

# Testing - These should be split out by library
set(TEST_FILES
    test/IO_MuonGroupingTest.h
    test/MuonAnalysisDataLoaderTest.h
    test/MuonAnalysisFitDataPresenterTest.h
    test/MuonAnalysisFitFunctionPresenterTest.h
    test/MuonAnalysisHelperTest.h
    test/MuonAnalysisResultTableCreatorTest.h
    test/ISISReflectometry/Batch/BatchJobRunnerProcessingTest.h
    test/ISISReflectometry/Batch/BatchJobRunnerProgressBarTest.h
    test/ISISReflectometry/Batch/BatchJobRunnerWorkspacesTest.h
    test/ISISReflectometry/Batch/BatchPresenterTest.h
    test/ISISReflectometry/Batch/GroupProcessingAlgorithmTest.h
    test/ISISReflectometry/Batch/RowProcessingAlgorithmTest.h
    test/ISISReflectometry/Reduction/GroupTest.h
    test/ISISReflectometry/Reduction/ValidateRowTest.h
    test/ISISReflectometry/Reduction/ParseReflectometryStringsTest.h
    test/ISISReflectometry/Reduction/ReductionJobsMergeTest.h
    test/ISISReflectometry/Reduction/ValidatePerThetaDefaultsTest.h
    test/ISISReflectometry/RunsTable/RunsTablePresenterDisplayTest.h
    test/ISISReflectometry/RunsTable/RunsTablePresenterGroupDeletionTest.h
    test/ISISReflectometry/RunsTable/RunsTablePresenterGroupInsertionTest.h
    test/ISISReflectometry/RunsTable/RunsTablePresenterRowInsertionTest.h
    test/ISISReflectometry/RunsTable/RunsTablePresenterRowDeletionTest.h
    test/ISISReflectometry/RunsTable/RunsTablePresenterMergeJobsTest.h
    test/ISISReflectometry/RunsTable/RunsTablePresenterProcessingTest.h
    test/ISISReflectometry/Event/EventPresenterTest.h
    test/ISISReflectometry/Experiment/ExperimentPresenterTest.h
    test/ISISReflectometry/Experiment/ExperimentOptionDefaultsTest.h
    test/ISISReflectometry/Experiment/PerThetaDefaultsTableValidatorTest.h
    test/ISISReflectometry/Instrument/InstrumentPresenterTest.h
    test/ISISReflectometry/Instrument/InstrumentOptionDefaultsTest.h
    test/ISISReflectometry/Runs/CatalogRunNotifierTest.h
    test/ISISReflectometry/Runs/QtCatalogSearcherTest.h
    test/ISISReflectometry/Runs/RunsPresenterTest.h
    test/ISISReflectometry/MainWindow/MainWindowPresenterTest.h
    test/ISISReflectometry/Options/OptionsDialogPresenterTest.h
    test/ISISReflectometry/Runs/SearchResultTest.h
    test/ISISReflectometry/Save/SavePresenterTest.h
    test/ISISReflectometry/Common/ClipboardTest.h
    test/ISISReflectometry/Common/PlotterTestQt4.h
)

set(CXXTEST_EXTRA_HEADER_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}/test/ScientificInterfacesTestInitialization.h)

mtd_add_qt_tests(TARGET_NAME MantidQtScientificInterfacesTest
                 QT_VERSION 4
                 SRC ${TEST_FILES}
                 INCLUDE_DIRS
                   ../../Framework/DataObjects/inc
                   ../../Framework/TestHelpers/inc
                   inc
                   ${PYTHON_INCLUDE_PATH}
                 TEST_HELPER_SRCS
                   ../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
                   ../../Framework/TestHelpers/src/DataProcessorTestHelper.cpp
                   ../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
                   ../../Framework/TestHelpers/src/ReflectometryHelper.cpp
                   ../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
                   ../../Framework/TestHelpers/src/ScopedFileHelper.cpp
                   ../../Framework/TestHelpers/src/TearDownWorld.cpp
                 LINK_LIBS
                   ${CORE_MANTIDLIBS}
                   DataObjects
                   PythonInterfaceCore
                   gmock
                   ${POCO_LIBRARIES}
                 QT4_LINK_LIBS
                   Qwt5
                 MTD_QT_LINK_LIBS
                   MantidScientificInterfacesGeneral
                   MantidScientificInterfacesDirect
                   MantidScientificInterfacesMuon
                   MantidScientificInterfacesISISReflectometry
                   MantidScientificInterfacesMultiDatasetFit
                   MantidQtWidgetsCommon
                 PARENT_DEPENDENCIES
                   GUITests)

set(QT5_TEST_FILES
    test/ALCBaselineModellingModelTest.h
    test/ALCBaselineModellingPresenterTest.h
    test/ALCDataLoadingPresenterTest.h
    test/ALCLatestFileFinderTest.h
    test/ALCPeakFittingModelTest.h
    test/ALCPeakFittingPresenterTest.h
    test/ISISReflectometry/Batch/BatchJobRunnerProcessingTest.h
    test/ISISReflectometry/Batch/BatchJobRunnerProgressBarTest.h
    test/ISISReflectometry/Batch/BatchJobRunnerWorkspacesTest.h
    test/ISISReflectometry/Batch/BatchPresenterTest.h
    test/ISISReflectometry/Batch/GroupProcessingAlgorithmTest.h
    test/ISISReflectometry/Batch/RowProcessingAlgorithmTest.h
    test/ISISReflectometry/MainWindow/MainWindowPresenterTest.h
    test/ISISReflectometry/Options/OptionsDialogPresenterTest.h
    test/ISISReflectometry/Reduction/GroupTest.h
    test/ISISReflectometry/Reduction/ValidateRowTest.h
    test/ISISReflectometry/Reduction/ParseReflectometryStringsTest.h
    test/ISISReflectometry/Reduction/ReductionJobsMergeTest.h
    test/ISISReflectometry/Reduction/ValidatePerThetaDefaultsTest.h
    test/ISISReflectometry/RunsTable/RunsTablePresenterDisplayTest.h
    test/ISISReflectometry/RunsTable/RunsTablePresenterGroupDeletionTest.h
    test/ISISReflectometry/RunsTable/RunsTablePresenterGroupInsertionTest.h
    test/ISISReflectometry/RunsTable/RunsTablePresenterRowInsertionTest.h
    test/ISISReflectometry/RunsTable/RunsTablePresenterRowDeletionTest.h
    test/ISISReflectometry/Event/EventPresenterTest.h
    test/ISISReflectometry/Experiment/ExperimentPresenterTest.h
    test/ISISReflectometry/Experiment/PerThetaDefaultsTableValidatorTest.h
    test/ISISReflectometry/Instrument/InstrumentPresenterTest.h
    test/ISISReflectometry/Runs/RunsPresenterTest.h
    test/ISISReflectometry/Save/SavePresenterTest.h
    test/ISISReflectometry/Common/PlotterTestQt5.h
    test/ISISReflectometry/Common/DecoderTest.h
    test/ISISReflectometry/Common/EncoderTest.h
)

mtd_add_qt_tests(TARGET_NAME MantidQtScientificInterfacesTest
                 QT_VERSION 5
                 SRC ${QT5_TEST_FILES}
                 INCLUDE_DIRS
                   ../../Framework/DataObjects/inc
                   ../../Framework/TestHelpers/inc
                   inc
                 TEST_HELPER_SRCS
                   ../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
                   ../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
                   ../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
                   ../../Framework/TestHelpers/src/ScopedFileHelper.cpp
                   ../../Framework/TestHelpers/src/TearDownWorld.cpp
                 LINK_LIBS
                   ${CORE_MANTIDLIBS}
                   DataObjects
                   PythonInterfaceCore
                   gmock
                   Qt5::Test
                   ${POCO_LIBRARIES}
                   ${Boost_LIBRARIES}
                   ${PYTHON_LIBRARIES}
                 MTD_QT_LINK_LIBS
                   MantidScientificInterfacesISISReflectometry
                   MantidScientificInterfacesMuon
                   MantidQtWidgetsCommon
                   MantidQtWidgetsMplCpp
                 PARENT_DEPENDENCIES
                   GUITests)
