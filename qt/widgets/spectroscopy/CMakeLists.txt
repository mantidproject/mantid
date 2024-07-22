set(SRC_FILES
    src/DataModel.cpp
    src/DataValidationHelper.cpp
    src/FitData.cpp
    src/InelasticInterface.cpp
    src/InelasticTab.cpp
    src/InterfaceUtils.cpp
    src/OutputWidget/OutputPlotOptionsModel.cpp
    src/OutputWidget/OutputPlotOptionsPresenter.cpp
    src/OutputWidget/OutputPlotOptionsView.cpp
    src/RunWidget/RunPresenter.cpp
    src/RunWidget/RunView.cpp
    src/SettingsWidget/Settings.cpp
    src/SettingsWidget/SettingsHelper.cpp
    src/SettingsWidget/SettingsModel.cpp
    src/SettingsWidget/SettingsPresenter.cpp
    src/SettingsWidget/SettingsView.cpp
    src/ValidationUtils.cpp
)

set(MOC_FILES
    inc/MantidQtWidgets/Spectroscopy/InelasticInterface.h
    inc/MantidQtWidgets/Spectroscopy/InelasticTab.h
    inc/MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsView.h
    inc/MantidQtWidgets/Spectroscopy/RunWidget/RunView.h
    inc/MantidQtWidgets/Spectroscopy/SettingsWidget/Settings.h
    inc/MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsView.h
)

set(INC_FILES
    inc/MantidQtWidgets/Spectroscopy/DataModel.h
    inc/MantidQtWidgets/Spectroscopy/DataValidationHelper.h
    inc/MantidQtWidgets/Spectroscopy/FitData.h
    inc/MantidQtWidgets/Spectroscopy/IDataModel.h
    inc/MantidQtWidgets/Spectroscopy/InterfaceUtils.h
    inc/MantidQtWidgets/Spectroscopy/MockObjects.h
    inc/MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsModel.h
    inc/MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsPresenter.h
    inc/MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h
    inc/MantidQtWidgets/Spectroscopy/RunWidget/RunPresenter.h
    inc/MantidQtWidgets/Spectroscopy/SettingsWidget/ISettingsView.h
    inc/MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h
    inc/MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsPresenter.h
    inc/MantidQtWidgets/Spectroscopy/ValidationUtils.h
)

set(UI_FILES
    inc/MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptions.ui
    inc/MantidQtWidgets/Spectroscopy/RunWidget/RunView.ui
    inc/MantidQtWidgets/Spectroscopy/SettingsWidget/InterfaceSettings.ui
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
