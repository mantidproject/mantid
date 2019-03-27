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
                   src/globals.cpp
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
                   src/MdPlottingCmapsProvider.cpp
                   src/MenuWithToolTips.cpp
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
                   src/Plot3DDialog.cpp
                   src/Plot.cpp
                   src/PlotCurve.cpp
                   src/PlotDialog.cpp
                   src/PlotWizard.cpp
                   src/PluginFit.cpp
                   src/PolynomFitDialog.cpp
                   src/PolynomialFit.cpp
                   src/Process.cpp
                   src/ProjectRecovery.cpp
                   src/ProjectSaveView.cpp
                   src/ProjectSerialiser.cpp
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
                   src/UserFunction.cpp
                   src/VectorCurve.cpp
                   src/WindowFactory.cpp
                   src/origin/OPJFile.cpp
                   src/plot2D/ImageSymbol.cpp
                   src/analysis/fft2D.cpp
                   src/lib/src/CollapsiveGroupBox.cpp
                   src/lib/src/ColorBox.cpp
                   src/lib/src/ColorButton.cpp
                   src/lib/src/ColorMapEditor.cpp
                   src/lib/src/ExtensibleFileDialog.cpp
                   src/lib/src/LineNumberDisplay.cpp
                   src/lib/src/PatternBox.cpp
                   src/lib/src/PenStyleBox.cpp
                   src/lib/src/SymbolBox.cpp
                   src/lib/src/SymbolDialog.cpp
                   src/lib/src/TextFormatButtons.cpp
                   src/lib/3rdparty/qtcolorpicker/src/qtcolorpicker.cpp
                   src/ProjectRecoveryGUIs/ProjectRecoveryModel.cpp
                   src/ProjectRecoveryGUIs/ProjectRecoveryPresenter.cpp
                   src/ProjectRecoveryGUIs/ProjectRecoveryView.cpp
                   src/ProjectRecoveryGUIs/RecoveryFailureView.cpp
                   src/ProjectRecoveryGUIs/RecoveryThread.cpp
)

set ( QTIPLOT_C_SRC src/zlib123/minigzip.c )

# Suppress compiler warning on file that isn't ours
if ( CMAKE_COMPILER_IS_GNUCXX )
  set_source_files_properties ( src/lib/3rdparty/qtcolorpicker/src/qtcolorpicker.cpp
                                PROPERTIES COMPILE_FLAGS -Wno-cast-qual )
endif ()

set ( MANTID_SRCS  src/Mantid/AlgorithmDockWidget.cpp
				   src/Mantid/AlgorithmMonitor.cpp
                   src/Mantid/ErrorBarSettings.cpp
                   src/Mantid/FirstTimeSetup.cpp
                   src/Mantid/FitParameterTie.cpp
                   src/Mantid/IFunctionWrapper.cpp
                   src/Mantid/ImportWorkspaceDlg.cpp
                   src/Mantid/InputHistory.cpp
				   src/Mantid/LabelToolLogValuesDialog.cpp
                   src/Mantid/ManageCustomMenus.cpp
                   src/Mantid/ManageInterfaceCategories.cpp
                   src/Mantid/MantidAbout.cpp
                   src/Mantid/MantidApplication.cpp
                   src/Mantid/MantidCurve.cpp
                   src/Mantid/MantidMatrix.cpp
                   src/Mantid/MantidMatrixCurve.cpp
		   src/Mantid/MantidMatrixDxExtensionHandler.cpp
		   src/Mantid/MantidMatrixExtensionRequest.cpp
                   src/Mantid/MantidMatrixFunction.cpp
		   src/Mantid/MantidMatrixModel.cpp
                   src/Mantid/MantidMDCurve.cpp
                   src/Mantid/MantidMDCurveDialog.cpp
                   src/Mantid/MantidMatrixDialog.cpp
                   src/Mantid/MantidPlotUtilities.cpp
                   src/Mantid/MantidSampleLogDialog.cpp
                   src/Mantid/MantidSampleMaterialDialog.cpp
                   src/Mantid/MantidSurfaceContourPlotGenerator.cpp
                   src/Mantid/MantidUI.cpp
                   src/Mantid/MantidTable.cpp
                   src/Mantid/PeakPickerTool.cpp
                   src/Mantid/Preferences.cpp
                   src/Mantid/RemoveErrorsDialog.cpp
				   src/Mantid/SampleLogDialogBase.cpp
                   src/Mantid/UserFitFunctionDialog.cpp
                   src/Mantid/InstrumentWidget/InstrumentWindow.cpp
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
                   src/MdPlottingCmapsProvider.h
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
                   src/Plot3DDialog.h
                   src/PlotCurve.h
                   src/PlotDialog.h
                   src/Plot.h
                   src/PlotToolInterface.h
                   src/PlotWizard.h
                   src/PluginFit.h
                   src/PolynomFitDialog.h
                   src/PolynomialFit.h
                   src/Process.h
                   src/ProjectRecovery.h
                   src/ProjectSerialiser.h
                   src/ProjectSaveView.h
                   src/PythonScript.h
                   src/PythonScripting.h
                   src/QwtBarCurve.h
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
                   src/UserFunction.h
                   src/VectorCurve.h
                   src/WindowFactory.h
                   src/analysis/fft2D.h
                   src/origin/OPJFile.h
                   src/plot2D/ImageSymbol.h
                   src/lib/include/CollapsiveGroupBox.h
                   src/lib/include/ColorBox.h
                   src/lib/include/ColorButton.h
                   src/lib/include/ColorMapEditor.h
                   src/lib/include/ExtensibleFileDialog.h
                   src/lib/include/LineNumberDisplay.h
                   src/lib/include/PatternBox.h
                   src/lib/include/PenStyleBox.h
                   src/lib/include/SymbolBox.h
                   src/lib/include/SymbolDialog.h
                   src/lib/include/TextFormatButtons.h
                   src/lib/3rdparty/qtcolorpicker/src/qtcolorpicker.h
                   src/ProjectRecoveryGUIs/ProjectRecoveryModel.h
                   src/ProjectRecoveryGUIs/ProjectRecoveryPresenter.h
                   src/ProjectRecoveryGUIs/ProjectRecoveryView.h
                   src/ProjectRecoveryGUIs/RecoveryFailureView.h
                   src/ProjectRecoveryGUIs/RecoveryThread.h
)

set ( MANTID_HDRS  src/Mantid/AlgorithmMonitor.h
                   src/Mantid/ErrorBarSettings.h
                   src/Mantid/FirstTimeSetup.h
                   src/Mantid/FitParameterTie.h
                   src/Mantid/IFunctionWrapper.h
                   src/Mantid/ImportWorkspaceDlg.h
		   src/Mantid/IMantidMatrixExtensionHandler.h
                   src/Mantid/InputHistory.h
				   src/Mantid/LabelToolLogValuesDialog.h
                   src/Mantid/ManageCustomMenus.h
                   src/Mantid/ManageInterfaceCategories.h
                   src/Mantid/MantidAbout.h
                   src/Mantid/MantidApplication.h
                   src/Mantid/MantidCurve.h
                   src/Mantid/MantidMatrixCurve.h
		   src/Mantid/MantidMatrixDxExtensionHandler.h
		   src/Mantid/MantidMatrixExtensionRequest.h
		   src/Mantid/MantidMatrixModel.h
		   src/Mantid/MantidMatrixNullExtensionHandler.h
		   src/Mantid/MantidMatrixTabExtension.h
                   src/Mantid/MantidMDCurve.h
                   src/Mantid/MantidMDCurveDialog.h
                   src/Mantid/MantidMatrixDialog.h
                   src/Mantid/MantidMatrix.h
                   src/Mantid/MantidMatrixFunction.h
                   src/Mantid/MantidPlotUtilities.h
                   src/Mantid/MantidSampleLogDialog.h
                   src/Mantid/MantidSampleMaterialDialog.h
		   src/Mantid/MantidSurfaceContourPlotGenerator.h
                   src/Mantid/MantidUI.h
                   src/Mantid/MantidTable.h
                   src/Mantid/PeakPickerTool.h
                   src/Mantid/Preferences.h
                   src/Mantid/RemoveErrorsDialog.h
				   src/Mantid/SampleLogDialogBase.h
                   src/Mantid/UserFitFunctionDialog.h
                   src/Mantid/InstrumentWidget/InstrumentWindow.h
)

###########################################################################
# Add Qt to the include path
###########################################################################

include ( UseQt4 )

###########################################################################
# Do the sip generation.
###########################################################################

include_directories ( ${PYTHON_INCLUDE_PATH} )

# We need to manually add all the headers that are in qti.sip
# so that the dependencies are known to CMake
set ( QTI_SIP_HDRS src/MdiSubWindow.h
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
)

set( SRC_UNITY_IGNORE_FILES )

###########################################################################
# Sip generated files
###########################################################################
set ( SIP_SPEC ${CMAKE_CURRENT_SOURCE_DIR}/src/qti.sip )
set ( SIP_SRC_AUTO sip_qtipart0.cpp )

set ( MANTIDQTPYTHON_SIP_INCLUDES -I ${CMAKE_CURRENT_BINARY_DIR}/../qt/python/mantidqtpython -I ${CMAKE_CURRENT_SOURCE_DIR}/../qt/python/mantidqtpython )
add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SIP_SRC_AUTO}
                     COMMAND ${SIP_EXECUTABLE}
                          -I ${PYQT4_SIP_DIR} ${MANTIDQTPYTHON_SIP_INCLUDES}  ${PYQT4_SIP_FLAGS}
                          -c ${CMAKE_CURRENT_BINARY_DIR} -j1 -w -o
                          ${SIP_SPEC}
                     DEPENDS ${SIP_INCLUDE_DIR}/sip.h src/qti.sip ${QTI_SIP_HDRS} ../qt/python/mantidqtpython/mantidqtpython_def.sip
                     COMMENT "Generating python bindings using sip"
)

# Needed for sip.h header that can end up in a different place to to the main Python include directory
include_directories ( SYSTEM ${SIP_INCLUDE_DIR} )
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
                        src/MenuWithToolTips.h
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
                        src/ProjectSaveView.h
                        src/ProjectSerialiser.h
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
                        src/lib/include/ExtensibleFileDialog.h
                        src/lib/include/LineNumberDisplay.h
                        src/lib/include/PatternBox.h
                        src/lib/include/PenStyleBox.h
                        src/lib/include/SymbolBox.h
                        src/lib/include/SymbolDialog.h
                        src/lib/include/TextFormatButtons.h
                        src/lib/3rdparty/qtcolorpicker/src/qtcolorpicker.h
)

set ( MANTID_MOC_FILES src/Mantid/AlgorithmDockWidget.h
					   src/Mantid/AlgorithmMonitor.h
                       src/Mantid/ErrorBarSettings.h
                       src/Mantid/FirstTimeSetup.h
                       src/Mantid/IFunctionWrapper.h
                       src/Mantid/ImportWorkspaceDlg.h
					   src/Mantid/LabelToolLogValuesDialog.h
                       src/Mantid/ManageCustomMenus.h
                       src/Mantid/ManageInterfaceCategories.h
                       src/Mantid/MantidAbout.h
                       src/Mantid/MantidApplication.h
                       src/Mantid/MantidCurve.h
                       src/Mantid/MantidMatrixCurve.h
                       src/Mantid/MantidMDCurve.h
                       src/Mantid/MantidMDCurveDialog.h
                       src/Mantid/MantidMatrixDialog.h
                       src/Mantid/MantidMatrix.h
                       src/Mantid/MantidMatrixFunction.h
		               src/Mantid/MantidMatrixModel.h
                       src/Mantid/MantidSampleLogDialog.h
                       src/Mantid/MantidSampleMaterialDialog.h
                       src/Mantid/MantidUI.h
                       src/Mantid/MantidTable.h
                       src/Mantid/PeakPickerTool.h
                       src/Mantid/RemoveErrorsDialog.h
					   src/Mantid/SampleLogDialogBase.h
                       src/Mantid/UserFitFunctionDialog.h
                       src/Mantid/InstrumentWidget/InstrumentWindow.h
                       src/ProjectRecoveryGUIs/ProjectRecoveryView.h
                       src/ProjectRecoveryGUIs/RecoveryFailureView.h
                       src/ProjectRecoveryGUIs/RecoveryThread.h
)

set ( UI_FILES src/SendToProgramDialog.ui
               src/ProjectSave.ui
               src/Mantid/FirstTimeSetup.ui
               src/Mantid/UserFitFunctionDialog.ui
               src/Mantid/MantidAbout.ui
               src/Mantid/RemoveErrorsDialog.ui
               src/Mantid/ManageCustomMenus.ui
               src/Mantid/ManageInterfaceCategories.ui
               src/Mantid/MantidMDCurveDialog.ui
               src/Mantid/MantidSampleMaterialDialog.ui
               src/ProjectRecoveryGUIs/ProjectRecoveryWidget.ui
               src/ProjectRecoveryGUIs/RecoveryFailure.ui
)

qt4_wrap_cpp ( MOCCED_FILES ${QTIPLOT_MOC_FILES} ${MANTID_MOC_FILES} )
# Call separate function on third-party code that expects moc output to have certain name
qt4_generate_moc ( src/lib/3rdparty/qtcolorpicker/src/qtcolorpicker.cpp
                   ${CMAKE_CURRENT_BINARY_DIR}/qtcolorpicker.moc )
set_source_files_properties ( ${CMAKE_CURRENT_BINARY_DIR}/qtcolorpicker.moc
                             PROPERTIES HEADER_FILE_ONLY true )
set ( MOCCED_FILES ${MOCCED_FILES} ${CMAKE_CURRENT_BINARY_DIR}/qtcolorpicker.moc )

set ( SRC_FILES ${QTIPLOT_SRCS} ${MANTID_SRCS} ${SIP_SRC_AUTO} )
set ( INC_FILES ${QTIPLOT_HDRS} ${MANTID_HDRS} )
if ( WIN32 )
  set ( MANIFEST_FILES MantidPlot.manifest )
endif ()

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} ${MANIFEST_FILES} )

# Use a precompiled header where they are supported
enable_precompiled_headers( src/PrecompiledHeader.h ALL_SRC )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )

# The generated ui headers will go here:
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

###########################################################################
# Internal icon links
###########################################################################

qt4_add_resources ( RES_FILES ${PROJECT_SOURCE_DIR}/images/images.qrc )
qt4_add_resources ( RES_FILES ${PROJECT_SOURCE_DIR}/images//fonts/fonts.qrc )
qt4_add_resources ( RES_FILES ${CMAKE_CURRENT_SOURCE_DIR}/icons/icons.qrc )
qt4_add_resources ( RES_FILES ${PROJECT_SOURCE_DIR}/scripts/ErrorReporter/errorreporter.qrc )

###########################################################################
# Add the dependencies
###########################################################################

if(MAKE_VATES)
  include( ${PARAVIEW_USE_FILE} )
endif()

include_directories ( SYSTEM ${MUPARSER_INCLUDE_DIR} )
include_directories ( ${ZLIB_INCLUDE_DIRS} )

include_directories ( SYSTEM ${QWT5_INCLUDE_DIR} )

find_package ( QwtPlot3d REQUIRED )
include_directories ( SYSTEM ${QWTPLOT3D_INCLUDE_DIR} )

find_package ( QScintillaQt4 REQUIRED )

###########################################################################
# Now create the target and add its dependencies and flags
###########################################################################

add_definitions ( -DSCRIPTING_MUPARSER )
add_definitions ( -DSCRIPTING_PYTHON )
add_definitions ( -DQSCINTILLA_DLL )     # Will only have an effect on Windows (as is desired)
if ( MSVC )
  add_definitions ( -DQ_COMPILER_INITIALIZER_LISTS )
endif ()

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
  set ( MANTID_RC_FILE ${CMAKE_SOURCE_DIR}/images/MantidPlot.icns )
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
  qtiplot.py
)
copy_files_to_dir ( "${MTDPLOTPY_FILES}"
                            ${CMAKE_CURRENT_SOURCE_DIR}/pymantidplot
                            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/pymantidplot
                            MTDPLOT_INSTALL_FILES )
set( MTDPLOTPYMPL_FILES
  __init__.py
  backend_mtdqt4agg.py
)
copy_files_to_dir ( "${MTDPLOTPYMPL_FILES}"
                            ${CMAKE_CURRENT_SOURCE_DIR}/pymantidplot/mpl
                            ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/pymantidplot/mpl
                            MTDPLOTMPL_INSTALL_FILES )

# IPython scripts
set( IPY_FILES
  __init__.py
  mantid_ipython_widget.py
)
copy_files_to_dir ( "${IPY_FILES}"
                     ${CMAKE_CURRENT_SOURCE_DIR}/ipython_widget
                     ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/ipython_widget
                     IPYTHON_INSTALL_FILES )

###########################################################################
# MantidPlot executable
###########################################################################

add_executable ( MantidPlot ${WIN_CONSOLE} MACOSX_BUNDLE ${ALL_SRC} src/main.cpp
							${INC_FILES} ${QTIPLOT_C_SRC} ${UI_HDRS}
							${RES_FILES} ${MANTID_RC_FILE}
							${PYTHON_INSTALL_FILES} ${MTDPLOT_INSTALL_FILES} ${MTDPLOTMPL_INSTALL_FILES}
                                                        ${CONFIG_RESET_SCRIPT_FILE} ${IPYTHON_INSTALL_FILES}
)

target_include_directories ( MantidPlot PRIVATE
  src
  src/lib/include
  src/lib/3rdparty/qtcolorpicker/src
  ../qt/widgets/common/inc
  ../qt/widgets/instrumentview/inc
  ../qt/widgets/sliceviewer/inc
  ../qt/widgets/spectrumviewer/inc
  ../qt/widgets/factory/inc
  ../Framework/PythonInterface/inc
)

# Library dependencies
target_link_libraries ( MantidPlot LINK_PRIVATE
  ${TCMALLOC_LIBRARIES_LINKTIME}
  ${CORE_MANTIDLIBS}
  PythonInterfaceCore
  MantidQtWidgetsCommonQt4
  MantidQtWidgetsLegacyQwtQt4
  MantidQtWidgetsFactoryQt4
  MantidQtWidgetsInstrumentViewQt4
  MantidQtWidgetsSliceViewerQt4
  MantidQtWidgetsSpectrumViewerQt4
  ${Boost_LIBRARIES}
  ${POCO_LIBRARIES}
  ${GSL_LIBRARIES}
  ${MUPARSER_LIBRARIES}
  ${QT_LIBRARIES}
  Qwt5
  ${QWTPLOT3D_LIBRARIES}
  Qt4::Qscintilla
  ${PYTHON_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${OPENGL_glu_LIBRARY}
  ${OPENGL_gl_LIBRARY}
)
if (WIN32)
  set_target_properties ( MantidPlot PROPERTIES COMPILE_DEFINITIONS "PSAPI_VERSION=1"
                                                CXX_CLANG_TIDY "" )
  target_link_libraries ( MantidPlot PRIVATE Psapi.lib )
else()
  set_target_properties ( MantidPlot PROPERTIES CXX_CLANG_TIDY "" )
endif ()

if(MAKE_VATES)
  target_include_directories( MantidPlot SYSTEM PRIVATE ${PARAVIEW_INCLUDE_DIRS} )
  target_link_libraries ( MantidPlot LINK_PRIVATE vtkPVClientServerCoreRendering vtkPVServerManagerRendering ${vtkjsoncpp_LIBRARIES})
endif()

# Plugin dependencies
add_dependencies( MantidPlot mantidqtpython CompilePyUI )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(MantidPlot PROPERTIES INSTALL_RPATH "@executable_path;@executable_path/../Libraries")
elseif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  if(MAKE_VATES)
    set_target_properties(MantidPlot PROPERTIES
                          INSTALL_RPATH "\$ORIGIN/../${LIB_DIR};\$ORIGIN/../${LIB_DIR}/paraview-${ParaView_VERSION_MAJOR}.${ParaView_VERSION_MINOR}")
  else()
    set_target_properties(MantidPlot PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
  endif()
endif ()

set_target_properties ( MantidPlot PROPERTIES FOLDER "Qt4" )

###########################################################################
# Custom Info.plist file for OS X
###########################################################################
if( APPLE )
  set ( MAC_BUNDLE_EXECUTABLE Mantid_osx_launcher )
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
  set ( MAC_BUNDLE_ICON MantidPlot.icns )
  set ( MAC_BUNDLE_NAME MantidPlot )
  configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/../installers/MacInstaller/Info.plist.in
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
    MantidPlotMatplotlibTest.py
    MantidPlotProjectSerialiseTest.py
    MantidPlotProxiesTest.py
    MantidPlotPythonImportTest.py
    MantidPlotFoldersTest.py
    MantidPlotMdiSubWindowTest.py
    MantidPlotTiledWindowTest.py
    MantidPlotInputArgsCheck.py
    MantidPlotProjectRecovery.py
    TSVSerialiserTest.py
)

if ( MAKE_VATES )
 list  ( APPEND MANTIDPLOT_TEST_PY_FILES MantidPlotPVPythonTest.py )
endif ()


# Screenshots destination
if ("$ENV{WORKSPACE}" STREQUAL "")
  set (SCREENSHOTS_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/screenshots" )
else ()
  set (SCREENSHOTS_DIR "$ENV{WORKSPACE}/screenshots" )
endif ()

# Add an environment property MANTID_SOURCE so that the script can find the source of xmlrunner
if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )
  set ( _python_path $ENV{PYTHONPATH};${PYTHON_XMLRUNNER_DIR} )
  # cmake list separator and Windows environment separator are the same so escape the cmake one
  string ( REPLACE ";" "\\;" _python_path "${_python_path}" )
else()
  set ( _python_path $ENV{PYTHONPATH}:${PYTHON_XMLRUNNER_DIR} )
endif ()
list ( APPEND _test_environment "PYTHONPATH=${_python_path}" )
list ( APPEND _test_environment "MANTID_SOURCE=${CMAKE_SOURCE_DIR}" )
list ( APPEND _test_environment "MANTID_SCREENSHOT_REPORT=${SCREENSHOTS_DIR}" )
# Make a ctest target for each file
foreach  ( PYFILE ${MANTIDPLOT_TEST_PY_FILES} )
  # Add the test. Name of test = name of the file
  add_test ( NAME ${PYFILE} COMMAND $<TARGET_FILE:MantidPlot> -xq ${CMAKE_CURRENT_SOURCE_DIR}/test/${PYFILE} )
  # Set the previously-built environment
  set_tests_properties ( ${PYFILE} PROPERTIES
    RUN_SERIAL 1
    ENVIRONMENT "${_test_environment}"
    TIMEOUT ${TESTING_TIMEOUT} )
endforeach ()

add_dependencies( GUITests MantidPlot )

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
foreach(PY_FILE ${MTDPLOTPYMPL_FILES} )
  install ( FILES pymantidplot/mpl/${PY_FILE} DESTINATION ${BIN_DIR}/pymantidplot/mpl )
endforeach()
foreach(PY_FILE ${IPY_FILES} )
  install ( FILES ipython_widget/${PY_FILE} DESTINATION ${BIN_DIR}/ipython_widget )
endforeach()
install ( FILES ${CONFIG_RESET_SCRIPT} DESTINATION ${BIN_DIR} )

# file make_package.rb contains hard-coded path to the libraries
# this causes fail of the installation with macports
# therefore MACPORTS option is introduced
if ( APPLE )
  if (OSX_VERSION VERSION_LESS 10.9)
    configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/FixBundle.cmake.in
                   ${CMAKE_CURRENT_BINARY_DIR}/FixBundle.cmake
                   @ONLY )

    install ( SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/FixBundle.cmake )
  elseif (MACPORTS)
        install ( FILES  package_python_macports.py DESTINATION MantidPlot.app/ )
        configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/FixMacportsBundle.cmake.in
                   ${CMAKE_CURRENT_BINARY_DIR}/FixMacportsBundle.cmake
                   @ONLY )
        install ( SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/FixMacportsBundle.cmake )
  else ()
    configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/make_package.rb.in
                   ${CMAKE_CURRENT_BINARY_DIR}/make_package.rb
                   @ONLY )
    install ( FILES  ${CMAKE_CURRENT_BINARY_DIR}/make_package.rb DESTINATION MantidPlot.app/ )
    configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/FixMavericksBundle.cmake.in
                   ${CMAKE_CURRENT_BINARY_DIR}/FixMavericksBundle.cmake
                   @ONLY )
    install ( SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/FixMavericksBundle.cmake )
  endif ()
endif ()
