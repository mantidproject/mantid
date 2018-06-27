set ( SRC_FILES
    Reduction/Group.cpp
    Reduction/RangeInQ.cpp
    Reduction/ReductionWorkspaces.cpp
    Reduction/Row.cpp
    Reduction/SlicedReductionWorkspaces.cpp
    Reduction/ReductionJobs.cpp
    Reduction/ValidateRow.cpp
    Presenters/BatchPresenter.cpp
    Presenters/BatchPresenterFactory.cpp
    Presenters/RegexRowFilter.cpp
    Presenters/RowLocation.cpp
    Views/BatchView.cpp
    MeasurementItem.cpp
    InstrumentOptionDefaults.cpp
    InstrumentParameters.cpp
    ExperimentOptionDefaults.cpp
    QtReflEventTabView.cpp
    QtReflMainWindowView.cpp
    QtReflBatchView.cpp
    QtReflRunsTabView.cpp
    QtReflSaveTabView.cpp
    QtReflSettingsTabView.cpp
    ReflCatalogSearcher.cpp
    ReflAsciiSaver.cpp
    IReflAsciiSaver.cpp
    ReflAutoreduction.cpp
    ReflEventTabPresenter.cpp
    ReflFromStdStringMap.cpp
    ReflMainWindowPresenter.cpp
    ReflBatchPresenter.cpp
    ReflNexusMeasurementItemSource.cpp
    ReflRunsTabPresenter.cpp
    ReflSaveTabPresenter.cpp
    ReflSearchModel.cpp
    ReflSettingsTabPresenter.cpp
    ReflTableSchema.cpp
    TransferResults.cpp
    GetInstrumentParameter.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( INC_FILES
    DllConfig.h
    Views/IBatchView.h
    Views/BatchView.h
    Presenters/RegexRowFilter.h
    Presenters/RowLocation.h
    Presenters/BatchPresenter.h
    Presenters/BatchPresenterFactory.h
    InstrumentParameters.h
    IReflEventTabPresenter.h
    IReflMainWindowPresenter.h
    IReflBatchPresenter.h
    IReflMainWindowView.h
    IReflBatchView.h
    IReflRunsTabPresenter.h
    IReflRunsTabView.h
    IReflSaveTabPresenter.h
    IReflSaveTabView.h
    IReflSearcher.h
    IReflSettingsTabPresenter.h
    MeasurementItem.h
    PrecompiledHeader.h
    QtReflEventTabView.h
    QtReflMainWindowView.h
    QtReflBatchView.h
    QtReflRunsTabView.h
    QtReflSaveTabView.h
    QtReflSettingsTabView.h
    QWidgetGroup.h
    ReflAutoreduction.h
    ReflCatalogSearcher.h
    ReflAsciiSaver.h
    IReflAsciiSaver.h
    ReflEventTabPresenter.h
    ReflFromStdStringMap.h
    ReflMainWindowPresenter.h
    ReflBatchPresenter.h
    ReflMeasurementItemSource.h
    ReflNexusMeasurementItemSource.h
    ReflRunsTabPresenter.h
    ReflSaveTabPresenter.h
    ReflSearchModel.h
    ReflSettingsTabPresenter.h
    ReflTableSchema.h
    TransferResults.h
    GetInstrumentParameter.h
    ExperimentOptionDefaults.h
    InstrumentOptionDefaults.h
    First.h
)

set ( MOC_FILES
    ReflSearchModel.h
    QtReflEventTabView.h
    QtReflRunsTabView.h
    QtReflSaveTabView.h
    QtReflSettingsTabView.h
    QtReflMainWindowView.h
    QtReflBatchView.h
    Views/BatchView.h
)

set ( UI_FILES
   ReflEventTabWidget.ui
   ReflMainWindowWidget.ui
   ReflBatchWidget.ui
   ReflRunsTabWidget.ui
   ReflSaveTabWidget.ui
   ReflSettingsTabWidget.ui
   ReflWindow.ui
   Views/BatchView.ui
)

mtd_add_qt_library (TARGET_NAME MantidScientificInterfacesISISReflectometry
  QT_VERSION 4
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  DEFS IN_MANTIDQT_ISISREFLECTOMETRY
  PRECOMPILED PrecompiledHeader.h
  INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${JSONCPP_LIBRARIES}
  QT4_LINK_LIBS
    Qwt5
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
  INSTALL_DIR_BASE
      ${PLUGINS_DIR}
  OSX_INSTALL_RPATH
    @loader_path/../../Contents/MacOS
  LINUX_INSTALL_RPATH
    "\$ORIGIN/../../${LIB_DIR}"
)
