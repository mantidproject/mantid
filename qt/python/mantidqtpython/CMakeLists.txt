# Python bindings for legacy mantidqtpython library This is a Qt4-only module
# and serves to support MantidPlot alone.
include(SipQtTargetFunctions)

set(WIDGETS_DIR ../../widgets)
set(FACTORY_INC ${WIDGETS_DIR}/factory/inc)
set(WIDGETS_COMMON_INC ${WIDGETS_DIR}/common/inc)
set(DATAPROCESSOR_INC
    ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/DataProcessorUI)
set(INSTRUMENT_VIEW_INC ${WIDGETS_DIR}/instrumentview/inc)
set(SLICEVIEWER_INC ${WIDGETS_DIR}/sliceviewer/inc)
set(REFVIEWER_INC ${WIDGETS_DIR}/refdetectorview/inc)
set(COMMON_SIP_DIR ../common_sip)
set(
  HEADER_DEPENDS
  ${COMMON_SIP_DIR}/SIPVector.h
  ${COMMON_SIP_DIR}/string.sip
  ${COMMON_SIP_DIR}/vector.sip
  ../../../Framework/PythonInterface/core/inc/MantidPythonInterface/core/VersionCompat.h
  ${FACTORY_INC}/MantidQtWidgets/Factory/WidgetFactory.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/WorkspaceObserver.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/GraphOptions.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/AlgorithmDialog.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/UserSubWindow.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/InterfaceManager.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/MantidDesktopServices.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/MessageDisplay.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/MWRunFiles.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/WidgetScrollbarDecorator.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/FitPropertyBrowser.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/SlitCalculator.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/ManageUserDirectories.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/HintStrategy.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/Hint.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/Batch/Row.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/Batch/RowLocation.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/Batch/JobTreeView.h
  ${WIDGETS_COMMON_INC}/MantidQtWidgets/Common/Batch/JobTreeViewSignalAdapter.h
  ${SLICEVIEWER_INC}/MantidQtWidgets/SliceViewer/SliceViewerWindow.h
  ${SLICEVIEWER_INC}/MantidQtWidgets/SliceViewer/LineViewer.h
  ${SLICEVIEWER_INC}/MantidQtWidgets/SliceViewer/PeaksPresenter.h
  ${SLICEVIEWER_INC}/MantidQtWidgets/SliceViewer/ProxyCompositePeaksPresenter.h
  ${SLICEVIEWER_INC}/MantidQtWidgets/SliceViewer/PeaksPresenter.h
  ${SLICEVIEWER_INC}/MantidQtWidgets/SliceViewer/SliceViewer.h
  ${REFVIEWER_INC}/MantidQtWidgets/RefDetectorView/RefIVConnections.h
  ${REFVIEWER_INC}/MantidQtWidgets/RefDetectorView/RefMatrixWSImageView.h
  ${DATAPROCESSOR_INC}/WhiteList.h
  ${DATAPROCESSOR_INC}/PreprocessMap.h
  ${DATAPROCESSOR_INC}/ProcessingAlgorithm.h
  ${DATAPROCESSOR_INC}/PostprocessingAlgorithm.h
  ${DATAPROCESSOR_INC}/QDataProcessorWidget.h
  ${DATAPROCESSOR_INC}/DataProcessorMainPresenter.h
  ${DATAPROCESSOR_INC}/AppendRowCommand.h
  ${DATAPROCESSOR_INC}/AppendGroupCommand.h
  ${DATAPROCESSOR_INC}/ClearSelectedCommand.h
  ${DATAPROCESSOR_INC}/CopySelectedCommand.h
  ${DATAPROCESSOR_INC}/CutSelectedCommand.h
  ${DATAPROCESSOR_INC}/DeleteGroupCommand.h
  ${DATAPROCESSOR_INC}/DeleteRowCommand.h
  ${DATAPROCESSOR_INC}/ExpandCommand.h
  ${DATAPROCESSOR_INC}/ExportTableCommand.h
  ${DATAPROCESSOR_INC}/GroupRowsCommand.h
  ${DATAPROCESSOR_INC}/ImportTableCommand.h
  ${DATAPROCESSOR_INC}/NewTableCommand.h
  ${DATAPROCESSOR_INC}/OpenTableCommand.h
  ${DATAPROCESSOR_INC}/OptionsCommand.h
  ${DATAPROCESSOR_INC}/PasteSelectedCommand.h
  ${DATAPROCESSOR_INC}/PlotGroupCommand.h
  ${DATAPROCESSOR_INC}/PlotRowCommand.h
  ${DATAPROCESSOR_INC}/ProcessCommand.h
  ${DATAPROCESSOR_INC}/SaveTableCommand.h
  ${DATAPROCESSOR_INC}/SaveTableAsCommand.h
  ${DATAPROCESSOR_INC}/SeparatorCommand.h
  ${DATAPROCESSOR_INC}/WorkspaceCommand.h
  ${INSTRUMENT_VIEW_INC}/MantidQtWidgets/InstrumentView/InstrumentWidgetTab.h
  ${INSTRUMENT_VIEW_INC}/MantidQtWidgets/InstrumentView/InstrumentWidget.h
  ${INSTRUMENT_VIEW_INC}/MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h
  ${INSTRUMENT_VIEW_INC}/MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h
  ${INSTRUMENT_VIEW_INC}/MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h
  ${INSTRUMENT_VIEW_INC}/MantidQtWidgets/InstrumentView/InstrumentWidgetTreeTab.h
  )

mtd_add_sip_module(MODULE_NAME mantidqtpython
                   TARGET_NAME mantidqtpython
                   SIP_SRCS mantidqtpython_def.sip
                   HEADER_DEPS
                     ${HEADER_DEPENDS}
                   INCLUDE_DIRS
                     ${CMAKE_CURRENT_LIST_DIR}
                     ${COMMON_SIP_DIR}
                     ${PYTHON_INCLUDE_PATH}
                     ${SIP_INCLUDE_DIR}
                     ../../../Framework/PythonInterface/core/inc
                   PYQT_VERSION 4
                   LINK_LIBS
                     ${TCMALLOC_LIBRARIES_LINKTIME}
                     MantidQtWidgetsCommonQt4
                     MantidQtWidgetsLegacyQwtQt4
                     MantidQtWidgetsSliceViewerQt4
                     MantidQtWidgetsFactoryQt4
                     MantidQtWidgetsRefDetectorViewQt4
                     MantidQtWidgetsSpectrumViewerQt4
                     MantidQtWidgetsInstrumentViewQt4
                     ${CORE_MANTIDLIBS}
                     ${POCO_LIBRARIES}
                     ${Boost_LIBRARIES}
                     Qt4::QtCore
                     Qt4::QtGui
                     Qt4::QtOpenGL
                     Qwt5
                     ${PYTHON_LIBRARIES}
                   FOLDER Qt4
                   OSX_INSTALL_RPATH
                     @loader_path/../Contents/MacOS
                   LINUX_INSTALL_RPATH
                     "\$ORIGIN/../${LIB_DIR}")

if(MSVC)
  # Windows: Python library name needs to end in .pyd for Windows Debug python
  # library expects imported module names to end in _d For a debug build copy
  # the special PyQt4 debug build to the bin directory
  add_custom_command(
    TARGET mantidqtpython
	POST_BUILD
    COMMAND if 1==$<CONFIG:Debug> ${CMAKE_COMMAND} -E copy_directory
            ${PYTHON_DIR}/msvc-site-packages/debug/PyQt4
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/PyQt4
    COMMENT "Copying debug PyQt4 to bin")
endif()

# Installation settings
install(TARGETS mantidqtpython DESTINATION ${BIN_DIR})
