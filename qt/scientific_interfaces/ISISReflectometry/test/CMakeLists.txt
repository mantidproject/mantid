if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-maybe-uninitialized)
endif()

# Testing
set(TEST_FILES
    Instrument/InstrumentOptionDefaultsTest.h
    Instrument/InstrumentPresenterTest.h
    Batch/BatchPresenterTest.h
    Batch/BatchJobManagerProgressBarTest.h
    Batch/BatchJobManagerWorkspacesTest.h
    Batch/GroupProcessingAlgorithmTest.h
    Batch/BatchJobManagerProcessingTest.h
    Batch/RowProcessingAlgorithmTest.h
    Batch/RowPreprocessingAlgorithmTest.h
    Batch/SumBanksAlgorithmTest.h
    Event/EventPresenterTest.h
    MainWindow/MainWindowPresenterTest.h
    RunsTable/RunsTablePresenterRowDeletionTest.h
    RunsTable/RunsTablePresenterGroupInsertionTest.h
    RunsTable/RunsTablePresenterRowInsertionTest.h
    RunsTable/RunsTablePresenterProcessingTest.h
    RunsTable/RunsTablePresenterDisplayTest.h
    RunsTable/RunsTablePresenterGroupDeletionTest.h
    RunsTable/RunsTablePresenterMergeJobsTest.h
    Save/FileSaverTest.h
    Save/SavePresenterTest.h
    Runs/RunsPresenterTest.h
    Runs/CatalogRunNotifierTest.h
    Runs/QtCatalogSearcherTest.h
    Runs/QtSearchModelTest.h
    Runs/SearchResultTest.h
    Options/OptionsDialogPresenterTest.h
    Common/ClipboardTest.h
    Common/DecoderTest.h
    Common/DetectorTest.h
    Common/EncoderTest.h
    Common/PlotterTestQt5.h
    Experiment/ExperimentPresenterTest.h
    Experiment/LookupTableValidatorTest.h
    Experiment/ExperimentOptionDefaultsTest.h
    Reduction/BatchLookupIndexTest.h
    Reduction/LookupTableTest.h
    Reduction/ReductionJobsMergeTest.h
    Reduction/RowTest.h
    Reduction/ParseReflectometryStringsTest.h
    Reduction/ValidateLookupRowTest.h
    Reduction/GroupTest.h
    Reduction/ValidateRowTest.h
    Preview/InstViewModelTest.h
    Preview/PreviewJobManagerTest.h
    Preview/PreviewModelTest.h
    Preview/PreviewPresenterTest.h
)

set(CXXTEST_EXTRA_HEADER_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/InterfacesReflectometryTestInitialization.h)

mtd_add_qt_tests(
  TARGET_NAME MantidScientificInterfacesISISReflectometryTest
  QT_VERSION 5
  SRC ${TEST_FILES}
  INCLUDE_DIRS ../../../../Framework/CurveFitting/inc ../../../../Framework/DataObjects/inc ../GUI/Preview Preview
               Reduction
  LINK_LIBS ${CORE_MANTIDLIBS}
            Mantid::DataObjects
            gmock
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            Mantid::PythonInterfaceCore
            Python::Python
  MTD_QT_LINK_LIBS MantidScientificInterfacesISISReflectometry MantidQtWidgetsCommon MantidQtWidgetsPlotting
                   MantidQtWidgetsInstrumentView MantidQtWidgetsMplCpp MantidQtWidgetsRegionSelector
  PARENT_DEPENDENCIES GUITests
)

add_framework_test_helpers(MantidScientificInterfacesISISReflectometryTestQt5)
