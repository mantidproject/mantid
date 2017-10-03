set ( TEST_FILES
	ALCBaselineModellingModelTest.h
	ALCBaselineModellingPresenterTest.h
	ALCDataLoadingPresenterTest.h
	ALCLatestFileFinderTest.h
	ALCPeakFittingModelTest.h
	ALCPeakFittingPresenterTest.h
	EnggDiffFittingPresenterTest.h
	EnggDiffractionPresenterTest.h
	IO_MuonGroupingTest.h
	MDFLogValueFinderTest.h
	MeasurementItemTest.h
	MuonAnalysisDataLoaderTest.h
	MuonAnalysisFitDataPresenterTest.h
	MuonAnalysisFitFunctionPresenterTest.h
	MuonAnalysisHelperTest.h
	MuonAnalysisResultTableCreatorTest.h
	ReflDataProcessorPresenterTest.h
	ReflEventPresenterTest.h
	ReflEventTabPresenterTest.h
	ReflLegacyTransferStrategyTest.h
	ReflMainWindowPresenterTest.h
	ReflMeasureTransferStrategyTest.h
	ReflNexusMeasurementItemSourceTest.h
	ReflRunsTabPresenterTest.h
	ReflSaveTabPresenterTest.h
	ReflSettingsPresenterTest.h
	ReflSettingsTabPresenterTest.h
	UserInputValidatorTest.h
)

if ( CXXTEST_FOUND )
  include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )
  include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../Muon )


  include_directories( ../../../Framework/TestHelpers/inc ../../../Framework/DataObjects/inc )

  set ( TESTHELPER_SRCS ../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
                        ../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
                        ../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
                        ../../../Framework/TestHelpers/src/ScopedFileHelper.cpp
                        ../../../Framework/TestHelpers/src/TearDownWorld.cpp
      )

  cxxtest_add_test ( ScientificInterfacesTest ${TEST_FILES} ${GMOCK_TEST_FILES} )
  target_link_libraries( ScientificInterfacesTest LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidScientificInterfacesGeneral
            MantidScientificInterfacesMuon
            MantidScientificInterfacesISISReflectometry
            MantidScientificInterfacesEnggDiffraction
            MantidScientificInterfacesMultiDatasetFit
            MantidQtWidgetsCommon
            API
            DataObjects
            Geometry
            HistogramData
            Types
            Kernel
            ${Boost_LIBRARIES}
            ${POCO_LIBRARIES}
            ${QWT_LIBRARIES}
            ${QT_LIBRARIES}
            ${GMOCK_LIBRARIES}
            ${GTEST_LIBRARIES} )

  add_dependencies( ScientificInterfacesTest MDAlgorithms )
  # Test data
  add_dependencies( ScientificInterfacesTest StandardTestData )
  add_dependencies( GUITests ScientificInterfacesTest )
  # Add to the 'UnitTests' group in VS
  set_property( TARGET ScientificInterfacesTest PROPERTY FOLDER "UnitTests" )

endif ()
