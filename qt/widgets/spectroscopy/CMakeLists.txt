set(SRC_FILES
    src/DataModel.cpp
    src/DataValidationHelper.cpp
    src/FitData.cpp
    src/InelasticInterface.cpp
    src/InelasticTab.cpp
    src/InterfaceUtils.cpp
    src/OutputPlotOptionsModel.cpp
    src/OutputPlotOptionsPresenter.cpp
    src/OutputPlotOptionsView.cpp
    src/RunWidget/RunPresenter.cpp
    src/RunWidget/RunView.cpp
    src/Settings.cpp
    src/SettingsHelper.cpp
    src/SettingsModel.cpp
    src/SettingsPresenter.cpp
    src/SettingsView.cpp
    src/ValidationUtils.cpp
)

set(MOC_FILES
    inc/MantidQtWidgets/Spectroscopy/InelasticInterface.h inc/MantidQtWidgets/Spectroscopy/InelasticTab.h
    inc/MantidQtWidgets/Spectroscopy/OutputPlotOptionsView.h inc/MantidQtWidgets/Spectroscopy/RunWidget/RunView.h
    inc/MantidQtWidgets/Spectroscopy/Settings.h inc/MantidQtWidgets/Spectroscopy/SettingsView.h
)

set(INC_FILES
    inc/MantidQtWidgets/Spectroscopy/DataModel.h
    inc/MantidQtWidgets/Spectroscopy/DataValidationHelper.h
    inc/MantidQtWidgets/Spectroscopy/FitData.h
    inc/MantidQtWidgets/Spectroscopy/IDataModel.h
    inc/MantidQtWidgets/Spectroscopy/InterfaceUtils.h
    inc/MantidQtWidgets/Spectroscopy/ISettingsView.h
    inc/MantidQtWidgets/Spectroscopy/OutputPlotOptionsModel.h
    inc/MantidQtWidgets/Spectroscopy/OutputPlotOptionsPresenter.h
    inc/MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h
    inc/MantidQtWidgets/Spectroscopy/RunWidget/RunPresenter.h
    inc/MantidQtWidgets/Spectroscopy/SettingsHelper.h
    inc/MantidQtWidgets/Spectroscopy/SettingsPresenter.h
    inc/MantidQtWidgets/Spectroscopy/ValidationUtils.h
)

set(UI_FILES
    inc/MantidQtWidgets/Spectroscopy/InterfaceSettings.ui inc/MantidQtWidgets/Spectroscopy/OutputPlotOptions.ui
    inc/MantidQtWidgets/Spectroscopy/RunWidget/RunView.ui
)

set(RES_FILES inc/MantidQtWidgets/Spectroscopy/InterfaceResources.qrc)

# XML is required to parse the settings file
find_package(
  Qt5 ${QT_MIN_VERSION}
  COMPONENTS Concurrent Xml
  REQUIRED
)

# Target
mtd_add_qt_library(
  TARGET_NAME MantidQtWidgetsSpectroscopy
  QT_VERSION 5
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  RES ${RES_FILES}
  DEFS IN_MANTIDQT_SPECTROSCOPY
  INCLUDE_DIRS inc
  LINK_LIBS ${CORE_MANTIDLIBS}
            Mantid::PythonInterfaceCore
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            Python::Python
            Qt5::Concurrent
            Qt5::Xml
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsPlotting MantidQtWidgetsMplCpp MantidQtIcons
  INSTALL_DIR ${WORKBENCH_LIB_DIR}
  OSX_INSTALL_RPATH @loader_path/../MacOS @loader_path/../Frameworks
  LINUX_INSTALL_RPATH "\$ORIGIN/../${WORKBENCH_LIB_DIR}"
)

# Testing target
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  add_subdirectory(test)
endif()
