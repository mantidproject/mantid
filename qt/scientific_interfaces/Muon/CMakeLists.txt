# Qt 5 implementation
set(SRC_FILES
    ALCBaselineModellingModel.cpp
    ALCBaselineModellingPresenter.cpp
    ALCBaselineModellingView.cpp
    ALCDataLoadingPresenter.cpp
    ALCDataLoadingView.cpp
    ALCInterface.cpp
    ALCLatestFileFinder.cpp
    ALCPeakFittingModel.cpp
    ALCPeakFittingPresenter.cpp
    ALCPeakFittingView.cpp
    MuonAnalysisHelper.cpp
)

set(MOC_FILES
    ALCBaselineModellingPresenter.h
    ALCBaselineModellingView.h
    ALCInterface.h
    ALCPeakFittingPresenter.h
    IALCBaselineModellingView.h
    IALCBaselineModellingModel.h
    IALCDataLoadingView.h
    IALCPeakFittingView.h
    IALCPeakFittingModel.h
    MuonAnalysisHelper.h
)

set(INC_FILES
    ALCBaselineModellingModel.h
    ALCBaselineModellingPresenter.h
    ALCBaselineModellingView.h
    ALCDataLoadingPresenter.h
    ALCDataLoadingView.h
    ALCInterface.h
    ALCLatestFileFinder.h
    ALCPeakFittingModel.h
    ALCPeakFittingPresenter.h
    ALCPeakFittingView.h
    DllConfig.h
    IALCBaselineModellingModel.h
    IALCBaselineModellingView.h
    IALCBaselineModellingPresenter.h
    IALCDataLoadingView.h
    IALCPeakFittingModel.h
    IALCPeakFittingView.h
)

set(UI_FILES ALCBaselineModellingView.ui ALCDataLoadingView.ui ALCInterface.ui ALCPeakFittingView.ui)

mtd_add_qt_library(
  TARGET_NAME MantidScientificInterfacesMuon
  QT_VERSION 5
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  DEFS IN_MANTIDQT_MUONINTERFACE PRECOMPILED PrecompiledHeader.h
  LINK_LIBS ${CORE_MANTIDLIBS} ${POCO_LIBRARIES} ${Boost_LIBRARIES} ${JSONCPP_LIBRARIES}
  MTD_QT_LINK_LIBS MantidQtWidgetsCommon MantidQtWidgetsMplCpp MantidQtWidgetsPlotting
  INSTALL_DIR_BASE ${WORKBENCH_PLUGINS_DIR}
  OSX_INSTALL_RPATH @loader_path/../../MacOS @loader_path/../../plugins/qt5
  LINUX_INSTALL_RPATH "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt5/"
)

# Testing target
if(MANTID_FRAMEWORK_LIB STREQUAL "BUILD")
  add_subdirectory(test)
endif()
