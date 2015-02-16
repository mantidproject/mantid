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
                   src/AxisDetails.cpp
                   src/Bar.cpp
                   src/BoxCurve.cpp
                   src/CanvasPicker.cpp
                   src/ColorMapDialog.cpp
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
                   src/DockedWindow.cpp
                   src/ErrDialog.cpp
                   src/ExpDecayDialog.cpp
                   src/ExponentialFit.cpp
                   src/ExportDialog.cpp
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
                   src/FloatingWindow.cpp
                   src/Folder.cpp
                   src/FunctionCurve.cpp
                   src/FunctionDialog.cpp
                   src/Graph3D.cpp
                   src/Graph.cpp
                   src/Grid.cpp
                   src/GridDetails.cpp
                   src/ImageDialog.cpp
                   src/ImageExportDialog.cpp
                   src/ImageMarker.cpp
                   src/ImportASCIIDialog.cpp
                   src/importOPJ.cpp
                   src/IntDialog.cpp
                   src/Integration.cpp
                   src/Interpolation.cpp
                   src/InterpolationDialog.cpp
                   src/LabelTool.cpp
                   src/LayerDialog.cpp
                   src/LegendWidget.cpp
                   src/LineDialog.cpp
                   src/LineProfileTool.cpp
                   src/LogisticFit.cpp
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
                   src/pixmaps.cpp
                   src/Plot3DDialog.cpp
                   src/Plot.cpp
                   src/PlotCurve.cpp
                   src/PlotDialog.cpp
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
                   src/ScaleDetails.cpp
                   src/ScaleDraw.cpp
                   src/ScalePicker.cpp
                   src/ScreenPickerTool.cpp
                   src/Script.cpp
                   src/ScriptCode.cpp
                   src/Scripted.cpp
                   src/ScriptingEnv.cpp
                   src/ScriptingLangDialog.cpp
                   src/ScriptingWindow.cpp
                   src/ScriptFileInterpreter.cpp
                   src/MultiTabScriptInterpreter.cpp
                   src/ScriptOutputDisplay.cpp
                   src/SelectionMoveResizer.cpp
                   src/SendToProgramDialog.cpp
                   src/SetColValuesDialog.cpp
                   src/SigmoidalFit.cpp
                   src/SmoothCurveDialog.cpp
                   src/SmoothFilter.cpp
                   src/SortDialog.cpp
                   src/Spectrogram.cpp
                   src/SurfaceDialog.cpp
                   src/Table.cpp
                   src/TableDialog.cpp
                   src/TableStatistics.cpp
                   src/TextDialog.cpp
                   src/TextEditor.cpp
                   src/TextFileIO.cpp
                   src/TiledWindow.cpp
                   src/TitlePicker.cpp
                   src/TranslateCurveTool.cpp
                   src/TSVSerialiser.cpp
                   src/UserFunction.cpp
                   src/VectorCurve.cpp
                   src/origin/OPJFile.cpp
                   src/plot2D/ImageSymbol.cpp
                   src/plot2D/ScaleEngine.cpp
                   src/analysis/fft2D.cpp
                   src/lib/src/CollapsiveGroupBox.cpp
                   src/lib/src/ColorBox.cpp
                   src/lib/src/ColorButton.cpp
                   src/lib/src/ColorMapEditor.cpp
                   src/lib/src/DoubleSpinBox.cpp
                   src/lib/src/ExtensibleFileDialog.cpp
                   src/lib/src/LineNumberDisplay.cpp
                   src/lib/src/PatternBox.cpp
                   src/lib/src/PenStyleBox.cpp
                   src/lib/src/SymbolBox.cpp
                   src/lib/src/SymbolDialog.cpp
                   src/lib/src/TextFormatButtons.cpp
                   src/lib/3rdparty/qtcolorpicker/src/qtcolorpicker.cpp
)

set ( QTIPLOT_C_SRC src/zlib123/minigzip.c )

# Suppress compiler warning on file that isn't ours
if ( CMAKE_COMPILER_IS_GNUCXX )
  set_source_files_properties ( src/lib/3rdparty/qtcolorpicker/src/qtcolorpicker.cpp
                                PROPERTIES COMPILE_FLAGS -Wno-cast-qual )
endif ()

set ( MANTID_SRCS  src/Mantid/AlgorithmMonitor.cpp
                   src/Mantid/AlgorithmHistoryWindow.cpp
                   src/Mantid/ErrorBarSettings.cpp
                   src/Mantid/FirstTimeSetup.cpp
                   src/Mantid/FitParameterTie.cpp
                   src/Mantid/FlowLayout.cpp
                   src/Mantid/IFunctionWrapper.cpp
                   src/Mantid/ImportWorkspaceDlg.cpp
                   src/Mantid/InputHistory.cpp
                   src/Mantid/LoadDAEDlg.cpp
                   src/Mantid/ManageCustomMenus.cpp
                   src/Mantid/ManageInterfaceCategories.cpp
                   src/Mantid/MantidAbout.cpp
                   src/Mantid/MantidApplication.cpp
                   src/Mantid/MantidCurve.cpp
                   src/Mantid/MantidDock.cpp
                   src/Mantid/MantidMatrix.cpp
                   src/Mantid/MantidMatrixCurve.cpp
                   src/Mantid/MantidMatrixFunction.cpp
                   src/Mantid/MantidMDCurve.cpp
                   src/Mantid/MantidMDCurveDialog.cpp
                   src/Mantid/MantidMatrixDialog.cpp
                   src/Mantid/MantidSampleLogDialog.cpp
                   src/Mantid/MantidSampleMaterialDialog.cpp
                   src/Mantid/MantidUI.cpp
                   src/Mantid/MantidTable.cpp
                   src/Mantid/MantidWSIndexDialog.cpp
                   src/Mantid/PeakPickerTool.cpp
                   src/Mantid/Preferences.cpp
                   src/Mantid/RemoveErrorsDialog.cpp
                   src/Mantid/SetUpParaview.cpp
                   src/Mantid/UserFitFunctionDialog.cpp
                   src/Mantid/WorkspaceIcons.cpp
                   src/Mantid/InstrumentWidget/BinDialog.cpp
                   src/Mantid/InstrumentWidget/CompAssemblyActor.cpp
                   src/Mantid/InstrumentWidget/ComponentActor.cpp
                   src/Mantid/InstrumentWidget/SampleActor.cpp
                   src/Mantid/InstrumentWidget/GLActorCollection.cpp
                   src/Mantid/InstrumentWidget/GLActor.cpp
                   src/Mantid/InstrumentWidget/GLActorVisitor.cpp
                   src/Mantid/InstrumentWidget/GLColor.cpp
                   src/Mantid/InstrumentWidget/GLObject.cpp
                   src/Mantid/InstrumentWidget/ICompAssemblyActor.cpp
                   src/Mantid/InstrumentWidget/InputController.cpp
                   src/Mantid/InstrumentWidget/InstrumentActor.cpp
                   src/Mantid/InstrumentWidget/InstrumentTreeModel.cpp
                   src/Mantid/InstrumentWidget/InstrumentTreeWidget.cpp
                   src/Mantid/InstrumentWidget/InstrumentWindow.cpp
                   src/Mantid/InstrumentWidget/InstrumentWindowTab.cpp
                   src/Mantid/InstrumentWidget/InstrumentWindowRenderTab.cpp
                   src/Mantid/InstrumentWidget/InstrumentWindowPickTab.cpp
                   src/Mantid/InstrumentWidget/InstrumentWindowMaskTab.cpp
                   src/Mantid/InstrumentWidget/InstrumentWindowTreeTab.cpp
                   src/Mantid/InstrumentWidget/MantidGLWidget.cpp
                   src/Mantid/InstrumentWidget/ColorMapWidget.cpp
                   src/Mantid/InstrumentWidget/ObjCompAssemblyActor.cpp
                   src/Mantid/InstrumentWidget/ObjComponentActor.cpp
                   src/Mantid/InstrumentWidget/OpenGLError.cpp
                   src/Mantid/InstrumentWidget/RectangularDetectorActor.cpp
                   src/Mantid/InstrumentWidget/UnwrappedCylinder.cpp
                   src/Mantid/InstrumentWidget/UnwrappedSphere.cpp
                   src/Mantid/InstrumentWidget/UnwrappedSurface.cpp
                   src/Mantid/InstrumentWidget/RotationSurface.cpp
                   src/Mantid/InstrumentWidget/PanelsSurface.cpp
                   src/Mantid/InstrumentWidget/ProjectionSurface.cpp
                   src/Mantid/InstrumentWidget/Projection3D.cpp
                   src/Mantid/InstrumentWidget/PeakMarker2D.cpp
                   src/Mantid/InstrumentWidget/PeakOverlay.cpp
                   src/Mantid/InstrumentWidget/RectF.cpp
                   src/Mantid/InstrumentWidget/OneCurvePlot.cpp
                   src/Mantid/InstrumentWidget/CollapsiblePanel.cpp
                   src/Mantid/InstrumentWidget/XIntegrationControl.cpp
                   src/Mantid/InstrumentWidget/Shape2D.cpp
                   src/Mantid/InstrumentWidget/Shape2DCollection.cpp
                   src/Mantid/InstrumentWidget/SimpleWidget.cpp
                   src/Mantid/InstrumentWidget/Viewport.cpp
                   src/Mantid/InstrumentWidget/DetXMLFile.cpp
                   src/Mantid/InstrumentWidget/UCorrectionDialog.cpp
)

###########################################################################
# Add the headers (so they show up in Visual Studio solutions)
###########################################################################

set ( QTIPLOT_HDRS src/ApplicationWindow.h
                   src/ArrowMarker.h
                   src/AssociationsDialog.h
                   src/AxesDialog.h
                   src/AxisDetails.h
                   src/Bar.h
                   src/BoxCurve.h
                   src/CanvasPicker.h
                   src/ColorMapDialog.h
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
                   src/DockedWindow.h
                   src/ErrDialog.h
                   src/ExpDecayDialog.h
                   src/ExponentialFit.h
                   src/ExportDialog.h
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
                   src/FloatingWindow.h
                   src/Folder.h
                   src/FunctionCurve.h
                   src/FunctionDialog.h
                   src/globals.h
                   src/Graph3D.h
                   src/Graph.h
                   src/Grid.h
                   src/GridDetails.h
                   src/ImageDialog.h
                   src/ImageExportDialog.h
                   src/ImageMarker.h
                   src/ImportASCIIDialog.h
                   src/importOPJ.h
                   src/IntDialog.h
                   src/Integration.h
                   src/InterpolationDialog.h
                   src/Interpolation.h
                   src/LabelTool.h
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
                   src/pixmaps.h
                   src/Plot3DDialog.h
                   src/PlotCurve.h
                   src/PlotDialog.h
                   src/Plot.h
                   src/PlotToolInterface.h
                   src/PlotWizard.h
                   src/PluginFit.h
                   src/PolynomFitDialog.h
                   src/PolynomialFit.h
                   src/PythonScript.h
                   src/PythonScripting.h
                   src/PythonSystemHeader.h
                   src/PythonThreading.h
                   src/QwtBarCurve.h
                   src/qwt_compat.h
                   src/QwtErrorPlotCurve.h
                   src/QwtHistogram.h
                   src/QwtPieCurve.h
                   src/RangeSelectorTool.h
                   src/RenameWindowDialog.h
                   src/resource.h
                   src/ScaleDetails.h
                   src/ScaleDraw.h
                   src/ScalePicker.h
                   src/ScreenPickerTool.h
                   src/Scripted.h
                   src/Script.h
                   src/ScriptCode.h
                   src/ScriptingEnv.h
                   src/ScriptingLangDialog.h
                   src/ScriptingWindow.h
                   src/MultiTabScriptInterpreter.h
                   src/ScriptOutputDisplay.h
                   src/ScriptFileInterpreter.h
                   src/SelectionMoveResizer.h
                   src/SendToProgramDialog.h
                   src/SetColValuesDialog.h
                   src/SigmoidalFit.h
                   src/SmoothCurveDialog.h
                   src/SmoothFilter.h
                   src/SortDialog.h
                   src/Spectrogram.h
                   src/SurfaceDialog.h
                   src/TableDialog.h
                   src/Table.h
                   src/TableStatistics.h
                   src/TextDialog.h
                   src/TextEditor.h
                   src/TextFileIO.h
                   src/TiledWindow.h
                   src/TitlePicker.h
                   src/TranslateCurveTool.h
                   src/TSVSerialiser.h
                   src/UserFunction.h
                   src/VectorCurve.h
                   src/analysis/fft2D.h
                   src/origin/OPJFile.h
                   src/plot2D/ImageSymbol.h
                   src/plot2D/ScaleEngine.h
                   src/lib/include/CollapsiveGroupBox.h
                   src/lib/include/ColorBox.h
                   src/lib/include/ColorButton.h
                   src/lib/include/ColorMapEditor.h
                   src/lib/include/DoubleSpinBox.h
                   src/lib/include/ExtensibleFileDialog.h
                   src/lib/include/LineNumberDisplay.h
                   src/lib/include/PatternBox.h
                   src/lib/include/PenStyleBox.h
                   src/lib/include/SymbolBox.h
                   src/lib/include/SymbolDialog.h
                   src/lib/include/TextFormatButtons.h
                   src/lib/3rdparty/qtcolorpicker/src/qtcolorpicker.h
)

set ( MANTID_HDRS  src/Mantid/AlgorithmMonitor.h
                   src/Mantid/AlgorithmHistoryWindow.h
                   src/Mantid/ErrorBarSettings.h
                   src/Mantid/FirstTimeSetup.h
                   src/Mantid/FitParameterTie.h
                   src/Mantid/FlowLayout.h
                   src/Mantid/IFunctionWrapper.h
                   src/Mantid/ImportWorkspaceDlg.h
                   src/Mantid/InputHistory.h
                   src/Mantid/LoadDAEDlg.h
                   src/Mantid/ManageCustomMenus.h
                   src/Mantid/ManageInterfaceCategories.h
                   src/Mantid/MantidAbout.h
                   src/Mantid/MantidApplication.h
                   src/Mantid/MantidCurve.h
                   src/Mantid/MantidDock.h
                   src/Mantid/MantidMatrixCurve.h
                   src/Mantid/MantidMDCurve.h
                   src/Mantid/MantidMDCurveDialog.h
                   src/Mantid/MantidMatrixDialog.h
                   src/Mantid/MantidMatrix.h
                   src/Mantid/MantidMatrixFunction.h
                   src/Mantid/MantidAlgorithmMetatype.h
                   src/Mantid/MantidSampleLogDialog.h
                   src/Mantid/MantidSampleMaterialDialog.h
                   src/Mantid/MantidUI.h
                   src/Mantid/MantidWSIndexDialog.h
                   src/Mantid/MantidTable.h
                   src/Mantid/PeakPickerTool.h
                   src/Mantid/Preferences.h
                   src/Mantid/RemoveErrorsDialog.h
                   src/Mantid/SetUpParaview.h
                   src/Mantid/UserFitFunctionDialog.h
                   src/Mantid/WorkspaceIcons.h
                   src/Mantid/InstrumentWidget/BinDialog.h
                   src/Mantid/InstrumentWidget/CompAssemblyActor.h
                   src/Mantid/InstrumentWidget/ComponentActor.h
                   src/Mantid/InstrumentWidget/SampleActor.h
                   src/Mantid/InstrumentWidget/GLActorCollection.h
                   src/Mantid/InstrumentWidget/GLActor.h
                   src/Mantid/InstrumentWidget/GLActorVisitor.h
                   src/Mantid/InstrumentWidget/GLColor.h
                   src/Mantid/InstrumentWidget/GLObject.h
                   src/Mantid/InstrumentWidget/ICompAssemblyActor.h
                   src/Mantid/InstrumentWidget/InputController.h
                   src/Mantid/InstrumentWidget/InstrumentActor.h
                   src/Mantid/InstrumentWidget/InstrumentTreeModel.h
                   src/Mantid/InstrumentWidget/InstrumentTreeWidget.h
                   src/Mantid/InstrumentWidget/InstrumentWindow.h
                   src/Mantid/InstrumentWidget/InstrumentWindowTab.h
                   src/Mantid/InstrumentWidget/InstrumentWindowRenderTab.h
                   src/Mantid/InstrumentWidget/InstrumentWindowPickTab.h
                   src/Mantid/InstrumentWidget/InstrumentWindowMaskTab.h
                   src/Mantid/InstrumentWidget/InstrumentWindowTreeTab.h
                   src/Mantid/InstrumentWidget/MantidGLWidget.h
                   src/Mantid/InstrumentWidget/ColorMapWidget.h
                   src/Mantid/InstrumentWidget/ObjCompAssemblyActor.h
                   src/Mantid/InstrumentWidget/ObjComponentActor.h
                   src/Mantid/InstrumentWidget/OpenGLError.h
                   src/Mantid/InstrumentWidget/RectangularDetectorActor.h
                   src/Mantid/InstrumentWidget/UnwrappedCylinder.h
                   src/Mantid/InstrumentWidget/UnwrappedSphere.h
                   src/Mantid/InstrumentWidget/UnwrappedSurface.h
                   src/Mantid/InstrumentWidget/RotationSurface.h
                   src/Mantid/InstrumentWidget/PeakMarker2D.h
                   src/Mantid/InstrumentWidget/PeakOverlay.h
                   src/Mantid/InstrumentWidget/PanelsSurface.h
                   src/Mantid/InstrumentWidget/ProjectionSurface.h
                   src/Mantid/InstrumentWidget/Projection3D.h
                   src/Mantid/InstrumentWidget/RectF.h
                   src/Mantid/InstrumentWidget/OneCurvePlot.h
                   src/Mantid/InstrumentWidget/CollapsiblePanel.h
                   src/Mantid/InstrumentWidget/XIntegrationControl.h
                   src/Mantid/InstrumentWidget/Shape2D.h
                   src/Mantid/InstrumentWidget/Shape2DCollection.h
                   src/Mantid/InstrumentWidget/SimpleWidget.h
                   src/Mantid/InstrumentWidget/Viewport.h
                   src/Mantid/InstrumentWidget/DetXMLFile.h
                   src/Mantid/InstrumentWidget/UCorrectionDialog.h
                   src/Mantid/IProjectSerialisable.h
)

###########################################################################
# Add Qt to the include path
###########################################################################

include ( UseSystemQt4 )

###########################################################################
# Do the sip generation.
###########################################################################

include_directories ( ${PYTHON_INCLUDE_PATH} )

set ( SIP_SPEC ${CMAKE_CURRENT_SOURCE_DIR}/src/qti.sip )
set ( SIP_SRC_IN ${CMAKE_CURRENT_SOURCE_DIR}/src/sipqti.cpp.in )
set ( SIP_SRC ${CMAKE_CURRENT_BINARY_DIR}/sipqti.cpp )
set ( SIP_SRC_AUTO sip_qtipart0.cpp )


# We need to manually add all the headers that are in qti.sip
# so that the dependencies are known to CMake
set ( SIP_HDRS src/MdiSubWindow.h
               src/Table.h
               src/Matrix.h
               src/ArrowMarker.h
               src/ImageMarker.h
               src/LegendWidget.h
               src/Grid.h
               src/Graph.h
               src/MultiLayer.h
               src/Note.h
               src/Graph3D.h
               src/ApplicationWindow.h
               src/Spectrogram.h
               src/PythonScripting.h
               src/PythonScript.h
               src/Folder.h
               src/plot2D/ImageSymbol.h
               src/Mantid/ErrorBarSettings.h
               src/Mantid/MantidUI.h
               src/Mantid/MantidMatrix.h
               src/Mantid/InstrumentWidget/InstrumentWindow.h
)

set( SRC_UNITY_IGNORE_FILES )

###########################################################################
# Sip generated files
###########################################################################

# The code generated by sip causes compiler warnings therefore the
# generated file is wrapped by ${SIP_SRC} and these warnings are
# disabled. In order for VS2010 to to this correctly the second
# custom command below is required along with the committed
# src/sipqti.cpp.rule file.
add_custom_command ( OUTPUT ${SIP_SRC_AUTO}
                     COMMAND ${SIP_EXECUTABLE}
                          -I ${PYQT4_SIP_DIR} -I ${CMAKE_CURRENT_SOURCE_DIR}/../MantidQt/Python ${PYQT4_SIP_FLAGS}
                          -c ${CMAKE_CURRENT_BINARY_DIR} -j1 -w -o
                          ${SIP_SPEC}
                     DEPENDS src/qti.sip ${SIP_HDRS}
                     COMMENT "Generating python bindings using sip"
)

add_custom_command ( OUTPUT ${SIP_SRC}
                     COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${SIP_SRC_IN} ${SIP_SRC}
                     DEPENDS ${SIP_SRC_AUTO}
                     COMMENT ""
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
                        src/AxisDetails.h
                        src/CanvasPicker.h
                        src/ColorMapDialog.h
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
                        src/DockedWindow.h
                        src/ErrDialog.h
                        src/ExpDecayDialog.h
                        src/ExponentialFit.h
                        src/ExportDialog.h
                        src/FFTDialog.h
                        src/FFTFilter.h
                        src/FFT.h
                        src/FilterDialog.h
                        src/Filter.h
                        src/FindDialog.h
                        src/FitDialog.h
                        src/Fit.h
                        src/FloatingWindow.h
                        src/Folder.h
                        src/FunctionDialog.h
                        src/Graph3D.h
                        src/Graph.h
                        src/Grid.h
                        src/GridDetails.h
                        src/ImageDialog.h
                        src/ImageExportDialog.h
                        src/ImportASCIIDialog.h
                        src/IntDialog.h
                        src/Integration.h
                        src/InterpolationDialog.h
                        src/Interpolation.h
                        src/LabelTool.h
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
                   	src/ScaleDetails.h
                        src/ScalePicker.h
                        src/ScreenPickerTool.h
                        src/Script.h
                        src/ScriptingEnv.h
                        src/ScriptingLangDialog.h
                        src/ScriptingWindow.h
                        src/MultiTabScriptInterpreter.h
                        src/ScriptOutputDisplay.h
                        src/ScriptFileInterpreter.h
                        src/SelectionMoveResizer.h
                        src/SendToProgramDialog.h
                        src/SetColValuesDialog.h
                        src/SigmoidalFit.h
                        src/SmoothCurveDialog.h
                        src/SmoothFilter.h
                        src/Spectrogram.h
                        src/SortDialog.h
                        src/SurfaceDialog.h
                        src/TableDialog.h
                        src/Table.h
                        src/TableStatistics.h
                        src/TextDialog.h
                        src/TextEditor.h
                        src/TiledWindow.h
                        src/TitlePicker.h
                        src/TranslateCurveTool.h
                        src/lib/include/CollapsiveGroupBox.h
                        src/lib/include/ColorBox.h
                        src/lib/include/ColorButton.h
                        src/lib/include/ColorMapEditor.h
                        src/lib/include/DoubleSpinBox.h
                        src/lib/include/ExtensibleFileDialog.h
                        src/lib/include/LineNumberDisplay.h
                        src/lib/include/PatternBox.h
                        src/lib/include/PenStyleBox.h
                        src/lib/include/SymbolBox.h
                        src/lib/include/SymbolDialog.h
                        src/lib/include/TextFormatButtons.h
                        src/lib/3rdparty/qtcolorpicker/src/qtcolorpicker.h
)

set ( MANTID_MOC_FILES src/Mantid/AlgorithmMonitor.h
                       src/Mantid/AlgorithmHistoryWindow.h
                       src/Mantid/ErrorBarSettings.h
                       src/Mantid/FirstTimeSetup.h
                       src/Mantid/IFunctionWrapper.h
                       src/Mantid/ImportWorkspaceDlg.h
                       src/Mantid/LoadDAEDlg.h
                       src/Mantid/ManageCustomMenus.h
                       src/Mantid/ManageInterfaceCategories.h
                       src/Mantid/MantidAbout.h
                       src/Mantid/MantidApplication.h
                       src/Mantid/MantidCurve.h
                       src/Mantid/MantidDock.h
                       src/Mantid/MantidMatrixCurve.h
                       src/Mantid/MantidMDCurve.h
                       src/Mantid/MantidMDCurveDialog.h
                       src/Mantid/MantidMatrixDialog.h
                       src/Mantid/MantidMatrix.h
                       src/Mantid/MantidMatrixFunction.h
                       src/Mantid/MantidSampleLogDialog.h
                       src/Mantid/MantidSampleMaterialDialog.h
                       src/Mantid/MantidUI.h
                       src/Mantid/MantidWSIndexDialog.h
                       src/Mantid/MantidTable.h
                       src/Mantid/PeakPickerTool.h
                       src/Mantid/RemoveErrorsDialog.h
                       src/Mantid/SetUpParaview.h
                       src/Mantid/UserFitFunctionDialog.h
                       src/Mantid/InstrumentWidget/XIntegrationControl.h
                       src/Mantid/InstrumentWidget/BinDialog.h
                       src/Mantid/InstrumentWidget/InputController.h
                       src/Mantid/InstrumentWidget/InstrumentTreeModel.h
                       src/Mantid/InstrumentWidget/InstrumentTreeWidget.h
                       src/Mantid/InstrumentWidget/InstrumentWindow.h
                       src/Mantid/InstrumentWidget/InstrumentWindowTab.h
                       src/Mantid/InstrumentWidget/InstrumentWindowRenderTab.h
                       src/Mantid/InstrumentWidget/InstrumentWindowPickTab.h
                       src/Mantid/InstrumentWidget/InstrumentWindowMaskTab.h
                       src/Mantid/InstrumentWidget/InstrumentWindowTreeTab.h
                       src/Mantid/InstrumentWidget/ColorMapWidget.h
                       src/Mantid/InstrumentWidget/MantidGLWidget.h
                       src/Mantid/InstrumentWidget/OneCurvePlot.h
                       src/Mantid/InstrumentWidget/CollapsiblePanel.h
                       src/Mantid/InstrumentWidget/InstrumentActor.h
                       src/Mantid/InstrumentWidget/PeakOverlay.h
                       src/Mantid/InstrumentWidget/ProjectionSurface.h
                       src/Mantid/InstrumentWidget/Projection3D.h
                       src/Mantid/InstrumentWidget/Shape2DCollection.h
                       src/Mantid/InstrumentWidget/UnwrappedSurface.h
					   src/Mantid/InstrumentWidget/UCorrectionDialog.h
)

set ( UI_FILES src/SendToProgramDialog.ui
               src/Mantid/FirstTimeSetup.ui
               src/Mantid/UserFitFunctionDialog.ui
               src/Mantid/MantidAbout.ui
               src/Mantid/RemoveErrorsDialog.ui
               src/Mantid/SetUpParaview.ui
               src/Mantid/ManageCustomMenus.ui
               src/Mantid/ManageInterfaceCategories.ui
               src/Mantid/MantidMDCurveDialog.ui
               src/Mantid/MantidSampleMaterialDialog.ui
      			   src/Mantid/InstrumentWidget/UCorrectionDialog.ui
)

qt4_wrap_cpp ( MOCCED_FILES ${QTIPLOT_MOC_FILES} ${MANTID_MOC_FILES} )
# Call separate function on third-party code that expects moc output to have certain name
qt4_generate_moc ( src/lib/3rdparty/qtcolorpicker/src/qtcolorpicker.cpp
                   ${CMAKE_CURRENT_BINARY_DIR}/qtcolorpicker.moc )
set_source_files_properties ( ${CMAKE_CURRENT_BINARY_DIR}/qtcolorpicker.moc
                             PROPERTIES HEADER_FILE_ONLY true )
set ( MOCCED_FILES ${MOCCED_FILES} ${CMAKE_CURRENT_BINARY_DIR}/qtcolorpicker.moc )

set ( SRC_FILES ${QTIPLOT_SRCS} ${MANTID_SRCS} ${SIP_SRC} )
set ( INC_FILES ${QTIPLOT_HDRS} ${MANTID_HDRS} )
set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

# Use a precompiled header where they are supported
enable_precompiled_headers( src/PrecompiledHeader.h ALL_SRC )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )

# The generated ui headers will go here:
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

###########################################################################
# Internal icon links
###########################################################################

qt4_add_resources ( RES_FILES ${PROJECT_SOURCE_DIR}/Images/images.qrc )
qt4_add_resources ( RES_FILES ${PROJECT_SOURCE_DIR}/Images//fonts/fonts.qrc )
qt4_add_resources ( RES_FILES ${CMAKE_CURRENT_SOURCE_DIR}/icons/icons.qrc )
qt4_add_resources ( RES_FILES ${PROJECT_SOURCE_DIR}/MantidQt/SliceViewer/icons/SliceViewerIcons.qrc )
qt4_add_resources ( RES_FILES ${PROJECT_SOURCE_DIR}/MantidQt/CustomInterfaces/icons/CustomInterfacesIcons.qrc )
qt4_add_resources ( RES_FILES ${PROJECT_SOURCE_DIR}/Vates/VatesSimpleGui/ViewWidgets/icons/ViewWidgetsIcons.qrc )

###########################################################################
# Add the dependencies
###########################################################################

include_directories ( SYSTEM ${MUPARSER_INCLUDE_DIR} )
include_directories ( ${ZLIB_INCLUDE_DIRS} )

include_directories ( SYSTEM ${QWT_INCLUDE_DIR} )

find_package ( QwtPlot3d REQUIRED )
include_directories ( SYSTEM ${QWTPLOT3D_INCLUDE_DIR} )

find_package ( QScintilla REQUIRED )
include_directories ( ${QSCINTILLA_INCLUDE_DIR} )

###########################################################################
# Now create the target and add its dependencies and flags
###########################################################################

add_definitions ( -DSCRIPTING_MUPARSER )
add_definitions ( -DSCRIPTING_PYTHON )
add_definitions ( -DQSCINTILLA_DLL )     # Will only have an effect on Windows (as is desired)

include_directories ( src )
include_directories ( src/lib/include )
include_directories ( src/lib/3rdparty/qtcolorpicker/src )

include_directories ( ../QtPropertyBrowser/src )
include_directories ( ../MantidQt/API/inc )
include_directories ( ../MantidQt/MantidWidgets/inc )
include_directories ( ../MantidQt/SliceViewer/inc )
include_directories ( ../MantidQt/SpectrumViewer/inc )
include_directories ( ../MantidQt/Factory/inc )
# ui_ files end up in these places
include_directories ( ${CMAKE_BINARY_DIR}/MantidQt/API )
include_directories ( ${CMAKE_BINARY_DIR}/MantidQt/MantidWidgets )
include_directories ( ${CMAKE_BINARY_DIR}/MantidQt/SliceViewer )


###########################################################################
# Application icon files
###########################################################################
if( WIN32 )
   set ( MANTID_RC_FILE icons/MantidPlotDesktop.rc )
   if ( CONSOLE )
     set ( WIN_CONSOLE )
   else ()
     set( WIN_CONSOLE WIN32 )
   endif( CONSOLE )
endif( WIN32 )

if ( APPLE )
  set ( MANTID_RC_FILE ${CMAKE_SOURCE_DIR}/Images/MantidPlot.icns )
  set_source_files_properties(MANTID_RC_FILE PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif ()

###########################################################################
# Config reset scripts
###########################################################################

if ( WIN32 )
  set ( CONFIG_RESET_SCRIPT mantid_reset_settings.bat )
else ()
  set ( CONFIG_RESET_SCRIPT mantid_reset_settings.sh )
endif ()

copy_files_to_dir ( "${CONFIG_RESET_SCRIPT}"
                     ${CMAKE_CURRENT_SOURCE_DIR}
                     ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
                     CONFIG_RESET_SCRIPT_FILE )

###########################################################################
# Required Python config files
###########################################################################

# Top-level python scripts
set( PY_FILES mantidplot.py mantidplotrc.py mantid_qt_settings_editor.py)
copy_files_to_dir ( "${PY_FILES}"
                            ${CMAKE_CURRENT_SOURCE_DIR}
                            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
                            PYTHON_INSTALL_FILES )
# mantidplot package
set( MTDPLOTPY_FILES
  __init__.py
  proxies.py
)
copy_files_to_dir ( "${MTDPLOTPY_FILES}"
                            ${CMAKE_CURRENT_SOURCE_DIR}/pymantidplot
                            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/pymantidplot
                            MTDPLOT_INSTALL_FILES )
                            
set( FUTURE_FILES
  __init__.py
  pyplot.py
)
copy_files_to_dir ( "${FUTURE_FILES}"
                            ${CMAKE_CURRENT_SOURCE_DIR}/pymantidplot/future
                            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/pymantidplot/future
                            MTDPLOT_FUTURE_INSTALL_FILES )

# IPython scripts
set( IPY_FILES
  __init__.py
  mantid_ipython_widget.py
)
copy_files_to_dir ( "${IPY_FILES}"
                     ${CMAKE_CURRENT_SOURCE_DIR}/ipython_widget
                     ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/ipython_widget
                     IPYTHON_INSTALL_FILES )

# SysMon scripts
set( SYSMON_FILES
  __init__.py
  _version.py
  config.py
  sysmon.py
  sysmon_tools.py
  ui_sysmon.py
  ui_sysmon.ui
)
copy_files_to_dir ( "${SYSMON_FILES}"
                     ${CMAKE_CURRENT_SOURCE_DIR}/SysMon
                     ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/SysMon
                     SYSMON_INSTALL_FILES )

###########################################################################
# MantidPlot executable
###########################################################################

add_executable ( MantidPlot ${WIN_CONSOLE} MACOSX_BUNDLE ${ALL_SRC} src/main.cpp
							${INC_FILES} ${QTIPLOT_C_SRC} ${UI_HDRS}
							${RES_FILES} ${MANTID_RC_FILE}
							${PYTHON_INSTALL_FILES} ${MTDPLOT_INSTALL_FILES} ${CONFIG_RESET_SCRIPT_FILE} ${MTDPLOT_FUTURE_INSTALL_FILES} ${IPYTHON_INSTALL_FILES} ${SYSMON_INSTALL_FILES}
)

# Library dependencies
target_link_libraries ( MantidPlot
                        ${CORE_MANTIDLIBS}
                        MantidQtAPI MantidWidgets MantidQtSliceViewer MantidQtFactory
                        MantidQtSpectrumViewer
                        QtPropertyBrowser ${QT_LIBRARIES}
                        ${QWT_LIBRARIES} ${QWTPLOT3D_LIBRARIES}
                        ${QSCINTILLA_LIBRARIES}
                        ${PYTHON_LIBRARIES}
                        ${ZLIB_LIBRARIES}
)
# Plugin dependencies
add_dependencies( MantidPlot mantidqtpython )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(MantidPlot PROPERTIES INSTALL_RPATH "@executable_path/")
endif ()

###########################################################################
# Custom Info.plist file for OS X
###########################################################################
if( APPLE )
  # Setting the CFBundleIdentifer attribute on OS X 10.6 (SL) and before
  # causes problems with trying to install the package on the same
  # machine as it was built. It tries to relocate the package to the build directory
  # because the bundle identifiers match. Mountain Lion requires the attribute
  # to avoid crashes when accessing certain system dialogs from Qt.
  # The ideal would be to pass the --no-relocate
  # option to packagemaker but we can't control that so only set the
  # identifier for 10.7 and above
  #
  # This can disappear when if/when we move to a drag-n-drop package.
  if (OSX_VERSION VERSION_GREATER 10.7 OR OSX_VERSION VERSION_EQUAL 10.7)
    set ( MAC_BUNDLE_IDENTIFIER "org.mantidproject.MantidPlot" )
  else()
    set ( MAC_BUNDLE_IDENTIFIER "" )
  endif()

  configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/../Installers/MacInstaller/Info.plist.in
                   ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
                   @ONLY )

  set_target_properties( MantidPlot PROPERTIES MACOSX_BUNDLE_INFO_PLIST
                         ${CMAKE_CURRENT_BINARY_DIR}/Info.plist )
endif()

###########################################################################
# Entry point flag for Windows to ensure we always link to standard main
###########################################################################
if( WIN32 )
   set_target_properties( MantidPlot PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup" )
   if ( CONSOLE )
     set_target_properties( MantidPlot PROPERTIES COMPILE_DEFINITIONS "_CONSOLE" )
   endif ( CONSOLE )
endif( WIN32 )

###########################################################################
# MantidPlot Python Unit Tests
###########################################################################

# List of .py files than must be run WITHIN MantidPlot.
set ( MANTIDPLOT_TEST_PY_FILES
    MantidPlotAlgorithmDialogTest.py
    MantidPlotInstrumentViewTest.py
    MantidPlotSliceViewerTest.py
    MantidPlot1DPlotTest.py
    MantidPlot2DPlotTest.py
    MantidPlotProxiesTest.py
    MantidPlotPythonImportTest.py
    MantidPlotFoldersTest.py
    MantidPlotMdiSubWindowTest.py
    MantidPlotTiledWindowTest.py
    MantidPlotInputArgsCheck.py
    MantidPlotFuturePyplotGeneralTest.py
)

if ( 0 )
	message (STATUS "Your CMAKE_SYSTEM string is '${CMAKE_SYSTEM}'")
	message (STATUS "Your CMAKE_SYSTEM_NAME string is '${CMAKE_SYSTEM_NAME}'")
	message (STATUS "Your CMAKE_BINARY_DIR string is '${CMAKE_BINARY_DIR}'")
	message (STATUS "Your BIN_DIR string is '${BIN_DIR}'")
	message (STATUS "Your CMAKE_LIBRARY_OUTPUT_DIRECTORY string is '${CMAKE_LIBRARY_OUTPUT_DIRECTORY}'")
	message (STATUS "Your CMAKE_CFG_INTDIR string is '${CMAKE_CFG_INTDIR}'")
	message (STATUS "Your CMAKE_CURRENT_BINARY_DIR string is '${CMAKE_CURRENT_BINARY_DIR}'")
endif ()


# Check system version
if ( ${CMAKE_SYSTEM} MATCHES ".*\\.el5" )
  	# This is RHEL5. GUI tests hang on this platform for some reason
  	message ( "RHEL5 hangs on GUI tests. Disabling MantidPlot python tests." )
else ()
	# Find the path to the built MantidPlot app. Need different paths for MAC for some reason.
	set (MANTIDPLOT_PATH  "${CMAKE_BINARY_DIR}/${BIN_DIR}/MantidPlot" )

	# Screenshots destination
	if ("$ENV{WORKSPACE}" STREQUAL "")
		set (SCREENSHOTS_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/screenshots" )
	else ()
		set (SCREENSHOTS_DIR "$ENV{WORKSPACE}/screenshots" )
	endif ()

	# Add an environment property MANTID_SOURCE so that the script can find the source of xmlrunner
	set (TEST_ENVIRONMENT "MANTID_SOURCE=${CMAKE_SOURCE_DIR};PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/test;MANTID_SCREENSHOT_REPORT=${SCREENSHOTS_DIR}" )

	# Fixes for Darwin (MacOS)
	if ( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		set (MANTIDPLOT_PATH  "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${BIN_DIR}/MantidPlot" )
		# Append to the path and set MANTIDPATH for MacOS' MantidPlot to run
		set (TEST_ENVIRONMENT "PATH=$ENV{PATH}:${CMAKE_LIBRARY_OUTPUT_DIRECTORY};MANTIDPATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY};MANTID_SOURCE=${CMAKE_SOURCE_DIR};MANTID_SCREENSHOT_REPORT=${SCREENSHOTS_DIR}" )
	endif ()

	# Make a ctest target for each file
	foreach  ( PYFILE ${MANTIDPLOT_TEST_PY_FILES} )
	 	# Add the test. Name of test = name of the file
		add_test ( ${PYFILE} ${MANTIDPLOT_PATH} -xq ${CMAKE_CURRENT_SOURCE_DIR}/test/${PYFILE} )
		# Set the previously-built environment
		set_tests_properties ( ${PYFILE} PROPERTIES
                           RUN_SERIAL 1
                           ENVIRONMENT "${TEST_ENVIRONMENT}"
                           TIMEOUT ${TESTING_TIMEOUT} )
	endforeach ()

    add_dependencies( GUITests MantidPlot )
endif()

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidPlot RUNTIME DESTINATION ${BIN_DIR}
                             BUNDLE DESTINATION .
)

# Ship required files. Explicit so some can be disabled easily if necessary
install ( FILES ${PY_FILES} DESTINATION ${BIN_DIR} )
foreach(PY_FILE ${MTDPLOTPY_FILES} )
  install ( FILES pymantidplot/${PY_FILE} DESTINATION ${BIN_DIR}/pymantidplot )
endforeach()
foreach(FUT_PY_FILE ${FUTURE_FILES} )
  install ( FILES pymantidplot/future/${FUT_PY_FILE} DESTINATION ${BIN_DIR}/pymantidplot/future )
endforeach()
foreach(PY_FILE ${IPY_FILES} )
  install ( FILES ipython_widget/${PY_FILE} DESTINATION ${BIN_DIR}/ipython_widget )
endforeach()
foreach(PY_FILE ${SYSMON_FILES} )
  install ( FILES SysMon/${PY_FILE} DESTINATION ${BIN_DIR}/SysMon )
endforeach()
install ( FILES ${CONFIG_RESET_SCRIPT} DESTINATION ${BIN_DIR} )

# file make_package.rb contains hard-coded path to the libraries
# this causes fail of the installation with macports
# therefore MACPORTS option is introduced
if ( APPLE )
  if (OSX_VERSION VERSION_LESS 10.9 OR MACPORTS)
  configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/FixBundle.cmake.in
                   ${CMAKE_CURRENT_BINARY_DIR}/FixBundle.cmake
                   @ONLY )

  install ( SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/FixBundle.cmake )
  else ()
    install ( FILES  make_package.rb DESTINATION MantidPlot.app/ )
    configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/FixMavericksBundle.cmake.in
                   ${CMAKE_CURRENT_BINARY_DIR}/FixMavericksBundle.cmake
                   @ONLY )
    install ( SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/FixMavericksBundle.cmake )
  endif ()
endif ()
