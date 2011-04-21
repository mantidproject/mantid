# NOTE that there are a lot of places in this CMakeLists where files need to 
# be explicitly listed - we not only have to add the sources and headers,
# but also the files that need to be made known to Qt and sip.

###########################################################################
# Add the source files
###########################################################################

set ( QTIPLOT_SRCS src/ApplicationWindow.cpp
                   src/ArrowMarker.cpp
                   src/AssociationsDialog.cpp
                   src/AxesDialog.cpp
                   src/Bar.cpp
                   src/BoxCurve.cpp
                   src/CanvasPicker.cpp
                   src/ColorBox.cpp
                   src/ColorButton.cpp
                   src/ColorMapDialog.cpp
                   src/ColorMapEditor.cpp
                   src/Cone3D.cpp
                   src/ConfigDialog.cpp
                   src/ContourLinesEditor.cpp
                   src/Convolution.cpp
                   src/Correlation.cpp
                   src/CurveRangeDialog.cpp
                   src/CurvesDialog.cpp
                   src/CustomActionDialog.cpp
                   src/DataPickerTool.cpp
                   src/DataSetDialog.cpp
                   src/Differentiation.cpp
                   src/DoubleSpinBox.cpp
                   src/ErrDialog.cpp
                   src/ExpDecayDialog.cpp
                   src/ExponentialFit.cpp
                   src/ExportDialog.cpp
                   src/ExtensibleFileDialog.cpp
                   src/FFT.cpp
                   src/FFTDialog.cpp
                   src/FFTFilter.cpp
                   src/Filter.cpp
                   src/FilterDialog.cpp
                   src/FindDialog.cpp
                   src/Fit.cpp
                   src/FitDialog.cpp
                   src/fit_gsl.cpp
                   src/FitModelHandler.cpp
                   src/Folder.cpp
                   src/FunctionCurve.cpp
                   src/FunctionDialog.cpp
                   src/Graph3D.cpp
                   src/Graph.cpp
                   src/Grid.cpp
                   src/ImageDialog.cpp
                   src/ImageExportDialog.cpp
                   src/ImageMarker.cpp
                   src/ImportASCIIDialog.cpp
                   src/importOPJ.cpp
                   src/IntDialog.cpp
                   src/Integration.cpp
                   src/Interpolation.cpp
                   src/InterpolationDialog.cpp
                   src/LayerDialog.cpp
                   src/LegendWidget.cpp
                   src/LineDialog.cpp
                   src/LineProfileTool.cpp
                   src/LogisticFit.cpp
                   src/main.cpp
                   src/MatrixCommand.cpp
                   src/Matrix.cpp
                   src/MatrixDialog.cpp
                   src/MatrixModel.cpp
                   src/MatrixSizeDialog.cpp
                   src/MatrixValuesDialog.cpp
                   src/MdiSubWindow.cpp
                   src/MultiLayer.cpp
                   src/MultiPeakFit.cpp
                   src/MultiPeakFitTool.cpp
                   src/muParserScript.cpp
                   src/muParserScripting.cpp
                   src/MyParser.cpp
                   src/NonLinearFit.cpp
                   src/Note.cpp
                   src/nrutil.cpp
                   src/OpenProjectDialog.cpp
                   src/PatternBox.cpp
                   src/PenStyleBox.cpp
                   src/pixmaps.cpp
                   src/Plot3DDialog.cpp
                   src/Plot.cpp
                   src/PlotCurve.cpp
                   src/PlotDialog.cpp
                   src/PlotEnrichement.cpp
                   src/PlotWizard.cpp
                   src/PluginFit.cpp
                   src/PolynomFitDialog.cpp
                   src/PolynomialFit.cpp
                   src/PythonScript.cpp
                   src/PythonScripting.cpp
                   src/QwtBarCurve.cpp
                   src/QwtErrorPlotCurve.cpp
                   src/QwtHistogram.cpp
                   src/QwtPieCurve.cpp
                   src/RangeSelectorTool.cpp
                   src/RenameWindowDialog.cpp
                   src/ScaleDraw.cpp
                   src/ScalePicker.cpp
                   src/ScreenPickerTool.cpp
                   src/Script.cpp
                   src/Scripted.cpp
                   src/ScriptEdit.cpp
                   src/ScriptEditor.cpp
                   src/ScriptingEnv.cpp
                   src/ScriptingLangDialog.cpp
                   src/ScriptingWindow.cpp
                   src/ScriptManagerWidget.cpp
                   src/SelectionMoveResizer.cpp
                   src/SetColValuesDialog.cpp
                   src/SigmoidalFit.cpp
                   src/SmoothCurveDialog.cpp
                   src/SmoothFilter.cpp
                   src/SortDialog.cpp
                   src/Spectrogram.cpp
                   src/SurfaceDialog.cpp
                   src/SymbolBox.cpp
                   src/SymbolDialog.cpp
                   src/Table.cpp
                   src/TableDialog.cpp
                   src/TableStatistics.cpp
                   src/TextDialog.cpp
                   src/TextEditor.cpp
                   src/TextFormatButtons.cpp
                   src/TitlePicker.cpp
                   src/TranslateCurveTool.cpp
                   src/UserFunction.cpp
                   src/VectorCurve.cpp
                   src/origin/OPJFile.cpp
                   src/plot2D/ScaleEngine.cpp
                   src/analysis/fft2D.cpp
                   src/zlib123/minigzip.c
)

set ( MANTID_SRCS  src/Mantid/AbstractMantidLog.cpp
                   src/Mantid/AlgMonitor.cpp
                   src/Mantid/AlgorithmHistoryWindow.cpp
                   src/Mantid/ExecuteAlgorithm.cpp
				   src/Mantid/FirstTimeSetup.cpp
                   src/Mantid/FitParameterTie.cpp
                   src/Mantid/FitPropertyBrowser.cpp
                   src/Mantid/IFunctionWrapper.cpp
                   src/Mantid/ImportWorkspaceDlg.cpp
                   src/Mantid/InputHistory.cpp
                   src/Mantid/LoadDAEDlg.cpp
                   src/Mantid/LoadRawDlg.cpp
                   src/Mantid/ManageCustomMenus.cpp
                   src/Mantid/MantidAbout.cpp
                   src/Mantid/MantidApplication.cpp
                   src/Mantid/MantidCurve.cpp
                   src/Mantid/MantidDock.cpp
                   src/Mantid/MantidLog.cpp
                   src/Mantid/MantidMatrix.cpp
                   src/Mantid/MantidMatrixDialog.cpp
                   src/Mantid/MantidSampleLogDialog.cpp
                   src/Mantid/MantidUI.cpp
                   src/Mantid/PeakPickerTool.cpp
                   src/Mantid/Preferences.cpp
                   src/Mantid/PropertyHandler.cpp
                   src/Mantid/RemoveErrorsDialog.cpp
                   src/Mantid/SelectWorkspacesDialog.cpp
                   src/Mantid/SequentialFitDialog.cpp
                   src/Mantid/UserFitFunctionDialog.cpp
                   src/Mantid/WorkspaceObserver.cpp
                   src/Mantid/InstrumentWidget/BinDialog.cpp
                   src/Mantid/InstrumentWidget/CompAssemblyActor.cpp
                   src/Mantid/InstrumentWidget/GL3DWidget.cpp
                   src/Mantid/InstrumentWidget/GLActorCollection.cpp
                   src/Mantid/InstrumentWidget/GLActor.cpp
                   src/Mantid/InstrumentWidget/GLColor.cpp
                   src/Mantid/InstrumentWidget/GLGroupPickBox.cpp
                   src/Mantid/InstrumentWidget/GLObject.cpp
                   src/Mantid/InstrumentWidget/GLTrackball.cpp
                   src/Mantid/InstrumentWidget/GLViewport.cpp
                   src/Mantid/InstrumentWidget/ICompAssemblyActor.cpp
                   src/Mantid/InstrumentWidget/Instrument3DWidget.cpp
                   src/Mantid/InstrumentWidget/InstrumentActor.cpp
                   src/Mantid/InstrumentWidget/InstrumentTreeModel.cpp
                   src/Mantid/InstrumentWidget/InstrumentTreeWidget.cpp
                   src/Mantid/InstrumentWidget/InstrumentWindow.cpp
                   src/Mantid/InstrumentWidget/InstrumentWindowRenderTab.cpp
                   src/Mantid/InstrumentWidget/InstrumentWindowPickTab.cpp
                   src/Mantid/InstrumentWidget/MantidColorMap.cpp
                   src/Mantid/InstrumentWidget/ColorMapWidget.cpp
                   src/Mantid/InstrumentWidget/MantidObject.cpp
                   src/Mantid/InstrumentWidget/ObjCompAssemblyActor.cpp
                   src/Mantid/InstrumentWidget/ObjComponentActor.cpp
                   src/Mantid/InstrumentWidget/OpenGLError.cpp
                   src/Mantid/InstrumentWidget/RectangularDetectorActor.cpp
                   src/Mantid/InstrumentWidget/TexObject.cpp
                   src/Mantid/InstrumentWidget/UnwrappedCylinder.cpp
                   src/Mantid/InstrumentWidget/UnwrappedSphere.cpp
                   src/Mantid/InstrumentWidget/UnwrappedSurface.cpp
                   src/Mantid/InstrumentWidget/OneCurvePlot.cpp
                   src/Mantid/InstrumentWidget/CollapsiblePanel.cpp
                   src/Mantid/InstrumentWidget/DetSelector.cpp
                   src/Mantid/InstrumentWidget/XIntegrationControl.cpp
)

###########################################################################
# Add the headers (so they show up in Visual Studio solutions)
###########################################################################

set ( QTIPLOT_HDRS src/ApplicationWindow.h
                   src/ArrowMarker.h
                   src/AssociationsDialog.h
                   src/AxesDialog.h
                   src/Bar.h
                   src/BoxCurve.h
                   src/CanvasPicker.h
                   src/ColorBox.h
                   src/ColorButton.h
                   src/ColorMapDialog.h
                   src/ColorMapEditor.h
                   src/Cone3D.h
                   src/ConfigDialog.h
                   src/ContourLinesEditor.h
                   src/Convolution.h
                   src/Correlation.h
                   src/cursors.h
                   src/CurveRangeDialog.h
                   src/CurvesDialog.h
                   src/CustomActionDialog.h
                   src/customevents.h
                   src/DataPickerTool.h
                   src/DataSetDialog.h
                   src/Differentiation.h
                   src/DoubleSpinBox.h
                   src/ErrDialog.h
                   src/ExpDecayDialog.h
                   src/ExponentialFit.h
                   src/ExportDialog.h
                   src/ExtensibleFileDialog.h
                   src/FFTDialog.h
                   src/FFTFilter.h
                   src/FFT.h
                   src/FilterDialog.h
                   src/Filter.h
                   src/FindDialog.h
                   src/FitDialog.h
                   src/fit_gsl.h
                   src/Fit.h
                   src/FitModelHandler.h
                   src/Folder.h
                   src/FunctionCurve.h
                   src/FunctionDialog.h
                   src/globals.h
                   src/Graph3D.h
                   src/Graph.h
                   src/GraphOptions.h
                   src/Grid.h
                   src/ImageDialog.h
                   src/ImageExportDialog.h
                   src/ImageMarker.h
                   src/ImportASCIIDialog.h
                   src/importOPJ.h
                   src/IntDialog.h
                   src/Integration.h
                   src/InterpolationDialog.h
                   src/Interpolation.h
                   src/LayerDialog.h
                   src/LegendWidget.h
                   src/LineDialog.h
                   src/LineProfileTool.h
                   src/LogisticFit.h
                   src/MatrixCommand.h
                   src/MatrixDialog.h
                   src/Matrix.h
                   src/MatrixModel.h
                   src/MatrixSizeDialog.h
                   src/MatrixValuesDialog.h
                   src/MdiSubWindow.h
                   src/MultiLayer.h
                   src/MultiPeakFit.h
                   src/MultiPeakFitTool.h
                   src/muParserScript.h
                   src/muParserScripting.h
                   src/MyParser.h
                   src/NonLinearFit.h
                   src/Note.h
                   src/nrutil.h
                   src/OpenProjectDialog.h
                   src/PatternBox.h
                   src/PenStyleBox.h
                   src/pixmaps.h
                   src/Plot3DDialog.h
                   src/PlotCurve.h
                   src/PlotDialog.h
                   src/PlotEnrichement.h
                   src/Plot.h
                   src/PlotToolInterface.h
                   src/PlotWizard.h
                   src/PluginFit.h
                   src/PolynomFitDialog.h
                   src/PolynomialFit.h
                   src/PythonScript.h
                   src/PythonScripting.h
                   src/PythonSystemHeader.h
                   src/QwtBarCurve.h
                   src/qwt_compat.h
                   src/QwtErrorPlotCurve.h
                   src/QwtHistogram.h
                   src/QwtPieCurve.h
                   src/RangeSelectorTool.h
                   src/RenameWindowDialog.h
                   src/resource.h
                   src/ScaleDraw.h
                   src/ScalePicker.h
                   src/ScreenPickerTool.h
                   src/Scripted.h
                   src/ScriptEdit.h
                   src/ScriptEditor.h
                   src/Script.h
                   src/ScriptingEnv.h
                   src/ScriptingLangDialog.h
                   src/ScriptingWindow.h
                   src/ScriptManagerWidget.h
                   src/SelectionMoveResizer.h
                   src/SetColValuesDialog.h
                   src/SigmoidalFit.h
                   src/SmoothCurveDialog.h
                   src/SmoothFilter.h
                   src/SortDialog.h
                   src/Spectrogram.h
                   src/SurfaceDialog.h
                   src/SymbolBox.h
                   src/SymbolDialog.h
                   src/TableDialog.h
                   src/Table.h
                   src/TableStatistics.h
                   src/TextDialog.h
                   src/TextEditor.h
                   src/TextFormatButtons.h
                   src/TitlePicker.h
                   src/TranslateCurveTool.h
                   src/UserFunction.h
                   src/VectorCurve.h
                   src/analysis/fft2D.h
                   src/origin/OPJFile.h
                   src/plot2D/ScaleEngine.h
)

set ( MANTID_HDRS  src/Mantid/AbstractMantidLog.h
                   src/Mantid/AlgMonitor.h
                   src/Mantid/AlgorithmHistoryWindow.h
                   src/Mantid/ExecuteAlgorithm.h
				   src/Mantid/FirstTimeSetup.h
                   src/Mantid/FitParameterTie.h
                   src/Mantid/FitPropertyBrowser.h
                   src/Mantid/IFunctionWrapper.h
                   src/Mantid/ImportWorkspaceDlg.h
                   src/Mantid/InputHistory.h
                   src/Mantid/LoadDAEDlg.h
                   src/Mantid/LoadRawDlg.h
                   src/Mantid/ManageCustomMenus.h
                   src/Mantid/MantidAbout.h
                   src/Mantid/MantidApplication.h
                   src/Mantid/MantidCurve.h
                   src/Mantid/MantidDock.h
                   src/Mantid/MantidLog.h
                   src/Mantid/MantidMatrixDialog.h
                   src/Mantid/MantidMatrix.h
                   src/Mantid/MantidSampleLogDialog.h
                   src/Mantid/MantidUI.h
                   src/Mantid/PeakPickerTool.h
                   src/Mantid/Preferences.h
                   src/Mantid/PropertyHandler.h
                   src/Mantid/RemoveErrorsDialog.h
                   src/Mantid/SelectWorkspacesDialog.h
                   src/Mantid/SequentialFitDialog.h
                   src/Mantid/UserFitFunctionDialog.h
                   src/Mantid/WorkspaceObserver.h
                   src/Mantid/InstrumentWidget/BinDialog.h
                   src/Mantid/InstrumentWidget/CompAssemblyActor.h
                   src/Mantid/InstrumentWidget/GL3DWidget.h
                   src/Mantid/InstrumentWidget/GLActorCollection.h
                   src/Mantid/InstrumentWidget/GLActor.h
                   src/Mantid/InstrumentWidget/GLColor.h
                   src/Mantid/InstrumentWidget/GLGroupPickBox.h
                   src/Mantid/InstrumentWidget/GLObject.h
                   src/Mantid/InstrumentWidget/GLTrackball.h
                   src/Mantid/InstrumentWidget/GLViewport.h
                   src/Mantid/InstrumentWidget/ICompAssemblyActor.h
                   src/Mantid/InstrumentWidget/Instrument3DWidget.h
                   src/Mantid/InstrumentWidget/InstrumentActor.h
                   src/Mantid/InstrumentWidget/InstrumentTreeModel.h
                   src/Mantid/InstrumentWidget/InstrumentTreeWidget.h
                   src/Mantid/InstrumentWidget/InstrumentWindow.h
                   src/Mantid/InstrumentWidget/InstrumentWindowRenderTab.h
                   src/Mantid/InstrumentWidget/InstrumentWindowPickTab.h
                   src/Mantid/InstrumentWidget/MantidColorMap.h
                   src/Mantid/InstrumentWidget/ColorMapWidget.h
                   src/Mantid/InstrumentWidget/MantidObject.h
                   src/Mantid/InstrumentWidget/ObjCompAssemblyActor.h
                   src/Mantid/InstrumentWidget/ObjComponentActor.h
                   src/Mantid/InstrumentWidget/OpenGLError.h
                   src/Mantid/InstrumentWidget/RectangularDetectorActor.h
                   src/Mantid/InstrumentWidget/TexObject.h
                   src/Mantid/InstrumentWidget/UnwrappedCylinder.h
                   src/Mantid/InstrumentWidget/UnwrappedSphere.h
                   src/Mantid/InstrumentWidget/UnwrappedSurface.h
                   src/Mantid/InstrumentWidget/OneCurvePlot.h
                   src/Mantid/InstrumentWidget/CollapsiblePanel.h
                   src/Mantid/InstrumentWidget/DetSelector.h
                   src/Mantid/InstrumentWidget/XIntegrationControl.h
)

###########################################################################
# Do the sip generation.
###########################################################################

find_package ( PyQt4 REQUIRED )
find_package ( SIP REQUIRED )
separate_arguments ( PYQT4_SIP_FLAGS )

set ( SIP_SPEC ${CMAKE_CURRENT_SOURCE_DIR}/src/qti.sip )
set ( SIP_GEN_SRC sipqtipart0.cpp )
set ( SIP_SRC_WRAP src/sipqti.cpp )

# We need to manually add all the headers that are in qti.sip
# so that the dependencies are known to CMake
set ( SIP_HDRS src/MdiSubWindow.h
               src/Table.h
               src/Matrix.h
               src/ArrowMarker.h
               src/ImageMarker.h
               src/LegendWidget.h
               src/Grid.h
               src/GraphOptions.h
               src/Graph.h
               src/LegendWidget.h
               src/MultiLayer.h
               src/Note.h
               src/Graph3D.h
               src/ApplicationWindow.h
               src/Fit.h
               src/ExponentialFit.h
               src/SigmoidalFit.h
               src/LogisticFit.h
               src/MultiPeakFit.h
               src/NonLinearFit.h
               src/PluginFit.h
               src/PolynomialFit.h
               src/Filter.h
               src/Differentiation.h
               src/Integration.h
               src/Interpolation.h
               src/SmoothFilter.h
               src/FFTFilter.h
               src/FFT.h
               src/Correlation.h
               src/Convolution.h
               src/PythonScripting.h
               src/PythonScript.h
               src/Folder.h
               src/Mantid/MantidUI.h
               src/Mantid/MantidMatrix.h
               src/Mantid/InstrumentWidget/InstrumentWindow.h
)

set( SRC_UNITY_IGNORE_FILES )

add_custom_command ( OUTPUT ${SIP_GEN_SRC}
                     COMMAND ${SIP_EXECUTABLE}
                          -I ${PYQT4_SIP_DIR} ${PYQT4_SIP_FLAGS}
                          -c ${CMAKE_CURRENT_BINARY_DIR} -j1 -w 
                          ${SIP_SPEC}
                     DEPENDS src/qti.sip ${SIP_HDRS}
                     COMMENT "Generating python bindings using sip"
)

# Needed for sip.h header that can end up in a different place to to the main Python include directory
include_directories ( ${SIP_INCLUDE_DIR} )
# Needed for sip generated files to find includes in src
include_directories ( ${CMAKE_CURRENT_SOURCE_DIR} )

###########################################################################
# Specify the files that we need to pass to Qt macros
###########################################################################

set ( QTIPLOT_MOC_FILES src/ApplicationWindow.h
                        src/AssociationsDialog.h
                        src/AxesDialog.h
                        src/CanvasPicker.h
                        src/ColorBox.h
                        src/ColorButton.h
                        src/ColorMapDialog.h
                        src/ColorMapEditor.h
                        src/ConfigDialog.h
                        src/ContourLinesEditor.h
                        src/Convolution.h
                        src/Correlation.h
                        src/CurveRangeDialog.h
                        src/CurvesDialog.h
                        src/CustomActionDialog.h
                        src/DataPickerTool.h
                        src/DataSetDialog.h
                        src/Differentiation.h
                        src/DoubleSpinBox.h
                        src/ErrDialog.h
                        src/ExpDecayDialog.h
                        src/ExponentialFit.h
                        src/ExportDialog.h
                        src/ExtensibleFileDialog.h
                        src/FFTDialog.h
                        src/FFTFilter.h
                        src/FFT.h
                        src/FilterDialog.h
                        src/Filter.h
                        src/FindDialog.h
                        src/FitDialog.h
                        src/Fit.h
                        src/Folder.h
                        src/FunctionDialog.h
                        src/Graph3D.h
                        src/Graph.h
                        src/ImageDialog.h
                        src/ImageExportDialog.h
                        src/ImportASCIIDialog.h
                        src/IntDialog.h
                        src/Integration.h
                        src/InterpolationDialog.h
                        src/Interpolation.h
                        src/LayerDialog.h
                        src/LegendWidget.h
                        src/LineDialog.h
                        src/LineProfileTool.h
                        src/LogisticFit.h
                        src/MatrixDialog.h
                        src/Matrix.h
                        src/MatrixModel.h
                        src/MatrixSizeDialog.h
                        src/MatrixValuesDialog.h
                        src/MdiSubWindow.h
                        src/MultiLayer.h
                        src/MultiPeakFit.h
                        src/MultiPeakFitTool.h
                        src/muParserScript.h
                        src/muParserScripting.h
                        src/NonLinearFit.h
                        src/Note.h
                        src/OpenProjectDialog.h
                        src/PatternBox.h
                        src/PenStyleBox.h
                        src/Plot3DDialog.h
                        src/PlotCurve.h
                        src/PlotDialog.h
                        src/Plot.h
                        src/PlotWizard.h
                        src/PluginFit.h
                        src/PolynomFitDialog.h
                        src/PolynomialFit.h
                        src/PythonScript.h
                        src/PythonScripting.h
                        src/QwtPieCurve.h
                        src/RangeSelectorTool.h
                        src/RenameWindowDialog.h
                        src/ScalePicker.h
                        src/ScreenPickerTool.h
                        src/ScriptEdit.h
                        src/ScriptEditor.h
                        src/Script.h
                        src/ScriptingEnv.h
                        src/ScriptingLangDialog.h
                        src/ScriptingWindow.h
                        src/ScriptManagerWidget.h
                        src/SelectionMoveResizer.h
                        src/SetColValuesDialog.h
                        src/SigmoidalFit.h
                        src/SmoothCurveDialog.h
                        src/SmoothFilter.h
                        src/SortDialog.h
                        src/SurfaceDialog.h
                        src/SymbolBox.h
                        src/SymbolDialog.h
                        src/TableDialog.h
                        src/Table.h
                        src/TableStatistics.h
                        src/TextDialog.h
                        src/TextEditor.h
                        src/TextFormatButtons.h
                        src/TitlePicker.h
                        src/TranslateCurveTool.h
)

set ( MANTID_MOC_FILES src/Mantid/AlgMonitor.h
                       src/Mantid/AlgorithmHistoryWindow.h
                       src/Mantid/ExecuteAlgorithm.h
					   src/Mantid/FirstTimeSetup.h
                       src/Mantid/FitPropertyBrowser.h
                       src/Mantid/IFunctionWrapper.h
                       src/Mantid/ImportWorkspaceDlg.h
                       src/Mantid/LoadDAEDlg.h
                       src/Mantid/LoadRawDlg.h
                       src/Mantid/ManageCustomMenus.h
                       src/Mantid/MantidAbout.h
                       src/Mantid/MantidApplication.h
                       src/Mantid/MantidCurve.h
                       src/Mantid/MantidDock.h
                       src/Mantid/MantidLog.h
                       src/Mantid/MantidMatrixDialog.h
                       src/Mantid/MantidMatrix.h
                       src/Mantid/MantidSampleLogDialog.h
                       src/Mantid/MantidUI.h
                       src/Mantid/PeakPickerTool.h
                       src/Mantid/PropertyHandler.h
                       src/Mantid/RemoveErrorsDialog.h
                       src/Mantid/SelectWorkspacesDialog.h
                       src/Mantid/SequentialFitDialog.h
                       src/Mantid/UserFitFunctionDialog.h
                       src/Mantid/WorkspaceObserver.h
                       src/Mantid/InstrumentWidget/BinDialog.h
                       src/Mantid/InstrumentWidget/GL3DWidget.h
                       src/Mantid/InstrumentWidget/Instrument3DWidget.h
                       src/Mantid/InstrumentWidget/InstrumentTreeModel.h
                       src/Mantid/InstrumentWidget/InstrumentTreeWidget.h
                       src/Mantid/InstrumentWidget/InstrumentWindow.h
                       src/Mantid/InstrumentWidget/InstrumentWindowRenderTab.h
                       src/Mantid/InstrumentWidget/InstrumentWindowPickTab.h
                       src/Mantid/InstrumentWidget/ColorMapWidget.h
                       src/Mantid/InstrumentWidget/OneCurvePlot.h
                       src/Mantid/InstrumentWidget/CollapsiblePanel.h
                       src/Mantid/InstrumentWidget/XIntegrationControl.h
)

set ( UI_FILES src/Mantid/FirstTimeSetup.ui
               src/Mantid/UserFitFunctionDialog.ui
               src/Mantid/MantidAbout.ui
               src/Mantid/RemoveErrorsDialog.ui
               src/Mantid/SequentialFitDialog.ui
               src/Mantid/ManageCustomMenus.ui
)

qt4_wrap_cpp ( MOCCED_FILES ${QTIPLOT_MOC_FILES} ${MANTID_MOC_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )
# The generated ui headers will go here:
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )
# Internal icon links
qt4_add_resources ( RES_FILES ${PROJECT_SOURCE_DIR}/Images/images.qrc )

###########################################################################
# Add the dependencies
###########################################################################

include_directories ( ${MUPARSER_INCLUDE_DIR} )
include_directories ( ${ZLIB_INCLUDE_DIRS} )

include_directories ( ${QWT_INCLUDE_DIR} )

find_package ( QwtPlot3d REQUIRED )
include_directories ( ${QWTPLOT3D_INCLUDE_DIR} )

find_package ( QScintilla REQUIRED )
include_directories ( ${QSCINTILLA_INCLUDE_DIR} )

###########################################################################
# This section deals with creating the MantidPlotReleaseDate.h header
###########################################################################

if ( Subversion_FOUND )
  # extract working copy information for SOURCE_DIR into RELDATE_XXX variables
  Subversion_WC_INFO(${PROJECT_SOURCE_DIR} RELDATE)
else ()
  # Just use a dummy version number and print a warning
  message ( "Subversion not found - using 0 for revision number in MantidPlotReleaseDate.h" )
  set ( RELDATE_WC_LAST_CHANGED_REV 0 )
endif ()
# Create the header with the revision number in
configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/src/Mantid/MantidPlotReleaseDate.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/MantidPlotReleaseDate.h 
)

###########################################################################
# Now create the target and add its dependencies and flags
###########################################################################

add_definitions ( -DSCRIPTING_MUPARSER )
add_definitions ( -DSCRIPTING_PYTHON )
add_definitions ( -DQSCINTILLA_DLL )     # Will only have an effect on Windows (as is desired)

include_directories ( src )

set ( SRC_FILES ${QTIPLOT_SRCS} ${MANTID_SRCS} ${SIP_SRC_WRAP} )
set ( INC_FILES ${QTIPLOT_HDRS} ${MANTID_HDRS} )

###########################################################################
# Windows console mode
###########################################################################
# On Windows we need to distinguish between release and debug modes with 
# respect to the console window, i.e we don't want the console in release 
# modes. On VS2010 this requires the WIN32 argument and the additional 
# linker arguments as we don't use WinMain as an entry point.
if( WIN32 )
   set ( EXE_FLAG WIN32 )
   # Also include the icon's rc file so that it is embedded in the exe
   set ( MANTID_RC_FILE icons/MantidPlotDesktop.rc )
endif( WIN32 )

if ( APPLE )
  set ( MANTID_RC_FILE ${CMAKE_SOURCE_DIR}/Images/MantidPlot.icns )
  set_source_files_properties(MANTID_RC_FILE PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif ()

add_executable ( MantidPlot MACOSX_BUNDLE ${EXE_FLAG} ${SRC_FILES} ${INC_FILES} 
                            ${MOCCED_FILES} ${UI_HDRS} ${RES_FILES} 
							${MANTID_RC_FILE}
)
# Clean up
set ( EXE_FLAG ) 
# Library dependencies
target_link_libraries ( MantidPlot 
                        ${CORE_MANTIDLIBS} MantidQtAPI MantidWidgets
                        QtPropertyBrowser ${QT_LIBRARIES} 
                        ${QWT_LIBRARIES} ${QWTPLOT3D_LIBRARIES}
                        ${QSCINTILLA_LIBRARIES}
                        ${PYTHON_LIBRARIES}
                        ${ZLIB_LIBRARIES} 
)
###########################################################################
# Console and entry point flags for Windows
###########################################################################
if( WIN32 )
   set_target_properties( MantidPlot PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup" )
   set_target_properties( MantidPlot PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE" )
   set_target_properties( MantidPlot PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup" )
   set_target_properties( MantidPlot PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE" )
   set_target_properties( MantidPlot PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup" )
   set_target_properties( MantidPlot PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif( WIN32 )

###########################################################################
# Now copy the required python files into the build destination
###########################################################################

set ( PYTHON_INSTALL_FILES qtiplotrc.py qtiUtil.py mantidplot.py mantidplotrc.py )
foreach ( PYFILE ${PYTHON_INSTALL_FILES} )
  add_custom_command ( TARGET MantidPlot POST_BUILD 
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                         ${CMAKE_CURRENT_SOURCE_DIR}/${PYFILE}
                         ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} 
  )
endforeach ( PYFILE ${PYTHON_INSTALL_FILES} )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidPlot RUNTIME DESTINATION ${BIN_DIR} 
                             BUNDLE DESTINATION .
)
install ( FILES ${PYTHON_INSTALL_FILES} DESTINATION ${BIN_DIR} )

if ( APPLE )
  configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/FixBundle.cmake.in
                   ${CMAKE_CURRENT_BINARY_DIR}/FixBundle.cmake
                   @ONLY
  )

  install ( SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/FixBundle.cmake )
endif ()
