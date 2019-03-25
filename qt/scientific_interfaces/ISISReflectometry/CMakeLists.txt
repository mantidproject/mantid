set(SRC_FILES
    MeasurementItem.cpp
    InstrumentOptionDefaults.cpp
    InstrumentParameters.cpp
    ExperimentOptionDefaults.cpp
    QtReflEventTabView.cpp
    QtReflEventView.cpp
    QtReflMainWindowView.cpp
    QtReflRunsTabView.cpp
    QtReflSaveTabView.cpp
    QtReflSettingsTabView.cpp
    QtReflSettingsView.cpp
    ReflCatalogSearcher.cpp
    ReflAsciiSaver.cpp
    IReflAsciiSaver.cpp
    ReflAutoreduction.cpp
    ReflDataProcessorPresenter.cpp
    ReflEventPresenter.cpp
    ReflEventTabPresenter.cpp
    ReflFromStdStringMap.cpp
    ReflGenericDataProcessorPresenterFactory.cpp
    ReflLegacyTransferStrategy.cpp
    ReflMainWindowPresenter.cpp
    ReflMeasureTransferStrategy.cpp
    ReflNexusMeasurementItemSource.cpp
    ReflRunsTabPresenter.cpp
    ReflSaveTabPresenter.cpp
    ReflSearchModel.cpp
    ReflSettingsPresenter.cpp
    ReflSettingsTabPresenter.cpp
    ReflTableSchema.cpp
    TransferResults.cpp
    GetInstrumentParameter.cpp)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to
# find it.
set(INC_FILES
    DllConfig.h
    InstrumentParameters.h
    IReflEventPresenter.h
    IReflEventTabPresenter.h
    IReflEventView.h
    IReflMainWindowPresenter.h
    IReflMainWindowView.h
    IReflRunsTabPresenter.h
    IReflRunsTabView.h
    IReflSaveTabPresenter.h
    IReflSaveTabView.h
    IReflSearcher.h
    IReflSettingsPresenter.h
    IReflSettingsTabPresenter.h
    IReflSettingsView.h
    MeasurementItem.h
    PrecompiledHeader.h
    QtReflEventTabView.h
    QtReflEventView.h
    QtReflMainWindowView.h
    QtReflRunsTabView.h
    QtReflSaveTabView.h
    QtReflSettingsTabView.h
    QtReflSettingsView.h
    QWidgetGroup.h
    ReflAutoreduction.h
    ReflCatalogSearcher.h
    ReflAsciiSaver.h
    IReflAsciiSaver.h
    ReflDataProcessorMainPresenter.h
    ReflDataProcessorPresenter.h
    ReflEventPresenter.h
    ReflEventTabPresenter.h
    ReflFromStdStringMap.h
    ReflGenericDataProcessorPresenterFactory.h
    ReflLegacyTransferStrategy.h
    ReflMainWindowPresenter.h
    ReflMeasureTransferStrategy.h
    ReflMeasurementItemSource.h
    ReflNexusMeasurementItemSource.h
    ReflRunsTabPresenter.h
    ReflSaveTabPresenter.h
    ReflSearchModel.h
    ReflSettingsPresenter.h
    ReflSettingsTabPresenter.h
    ReflTableSchema.h
    ReflTransferStrategy.h
    TransferResults.h
    GetInstrumentParameter.h
    ExperimentOptionDefaults.h
    InstrumentOptionDefaults.h
    First.h)

set(MOC_FILES
    ReflSearchModel.h
    QtReflEventTabView.h
    QtReflEventView.h
    QtReflRunsTabView.h
    QtReflSaveTabView.h
    QtReflSettingsTabView.h
    QtReflSettingsView.h
    QtReflMainWindowView.h)

set(UI_FILES
    ReflEventTabWidget.ui
    ReflEventWidget.ui
    ReflMainWindowWidget.ui
    ReflRunsTabWidget.ui
    ReflSaveTabWidget.ui
    ReflSettingsTabWidget.ui
    ReflSettingsWidget.ui
    ReflWindow.ui)

mtd_add_qt_library(TARGET_NAME MantidScientificInterfacesISISReflectometry
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
                     "\$ORIGIN/../../${LIB_DIR}")
