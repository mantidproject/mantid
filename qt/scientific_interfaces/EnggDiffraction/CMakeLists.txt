set ( SRC_FILES
	EnggDiffFittingModel.cpp
	EnggDiffFittingPresenter.cpp
	EnggDiffFittingViewQtWidget.cpp
	EnggDiffGSASFittingModel.cpp
	EnggDiffGSASFittingPresenter.cpp
	EnggDiffGSASFittingViewQtWidget.cpp
	EnggDiffGSASFittingWorker.cpp
	EnggDiffMultiRunFittingQtWidget.cpp
	EnggDiffMultiRunFittingWidgetAdder.cpp
	EnggDiffMultiRunFittingWidgetModel.cpp
	EnggDiffMultiRunFittingWidgetPresenter.cpp
	EnggDiffractionPresenter.cpp
	EnggDiffractionViewQtGUI.cpp
	EnggVanadiumCorrectionsModel.cpp
	GSASIIRefineFitPeaksOutputProperties.cpp
	GSASIIRefineFitPeaksParameters.cpp
	RunLabel.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( INC_FILES
	EnggDiffCalibSettings.h
	EnggDiffFittingModel.h
	EnggDiffFittingPresWorker.h
	EnggDiffFittingPresenter.h
	EnggDiffFittingViewQtWidget.h
	EnggDiffGSASFittingModel.h
	EnggDiffGSASFittingPresenter.h
	EnggDiffGSASFittingViewQtWidget.h
	EnggDiffGSASFittingWorker.h
	EnggDiffGSASRefinementMethod.h
	EnggDiffMultiRunFittingQtWidget.h
	EnggDiffMultiRunFittingWidgetAdder.h
	EnggDiffMultiRunFittingWidgetModel.h
	EnggDiffMultiRunFittingWidgetPresenter.h
	EnggDiffractionPresWorker.h
	EnggDiffractionPresenter.h
	EnggDiffractionViewQtGUI.h
	EnggVanadiumCorrectionsModel.h
	GSASIIRefineFitPeaksOutputProperties.cpp
	GSASIIRefineFitPeaksParameters.h
	IEnggDiffFittingModel.h
	IEnggDiffFittingPresenter.h
	IEnggDiffGSASFittingModel.h
	IEnggDiffGSASFittingPresenter.h
	IEnggDiffGSASFittingView.h
	IEnggDiffMultiRunFittingWidgetAdder.h
	IEnggDiffMultiRunFittingWidgetModel.h
	IEnggDiffMultiRunFittingWidgetOwner.h
	IEnggDiffMultiRunFittingWidgetPresenter.h
	IEnggDiffMultiRunFittingWidgetView.h
	IEnggDiffractionPresenter.h
	IEnggDiffractionView.h
	IEnggVanadiumCorrectionsModel.h
	RunLabel.h
	RunMap.h
)

set ( MOC_FILES
    EnggDiffFittingPresenter.h
    EnggDiffFittingPresWorker.h
    EnggDiffFittingViewQtWidget.h
    EnggDiffGSASFittingModel.h
    EnggDiffGSASFittingViewQtWidget.h
    EnggDiffGSASFittingWorker.h
    EnggDiffractionPresenter.h
    EnggDiffractionPresWorker.h
    EnggDiffractionViewQtGUI.h
    EnggDiffMultiRunFittingQtWidget.h
)

set ( UI_FILES
   EnggDiffractionQtGUI.ui
   EnggDiffractionQtTabCalib.ui
   EnggDiffractionQtTabFocus.ui
   EnggDiffractionQtTabPreproc.ui
   EnggDiffractionQtTabFitting.ui
   EnggDiffractionQtTabGSAS.ui
   EnggDiffractionQtTabSettings.ui
   EnggDiffMultiRunFittingWidget.ui
)

mtd_add_qt_library (TARGET_NAME MantidScientificInterfacesEnggDiffraction
  QT_VERSION 4
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  DEFS IN_MANTIDQT_ENGGDIFFRACTION
  PRECOMPILED PrecompiledHeader.h
  INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
  SYSTEM_INCLUDE_DIRS
    ${Boost_INCLUDE_DIRS}
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
    MantidQtWidgetsLegacyQwt
    MantidScientificInterfacesMuon
  INSTALL_DIR_BASE
    ${PLUGINS_DIR}
  OSX_INSTALL_RPATH
    @loader_path/../../Contents/MacOS
    @loader_path/../../plugins/qt4
  LINUX_INSTALL_RPATH
    "\$ORIGIN/../../${LIB_DIR};\$ORIGIN/../../plugins/qt4/"

)
