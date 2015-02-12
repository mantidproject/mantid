set ( SRC_FILES
	src/AlgorithmSelectorWidget.cpp
	src/CatalogHelper.cpp
	src/CatalogSearch.cpp
	src/CatalogSelector.cpp
	src/CheckboxHeader.cpp
	src/DataSelector.cpp
	src/DiagResults.cpp
	src/FilenameDialogEditor.cpp
	src/FindDialog.cpp
	src/FindReplaceDialog.cpp
	src/FitPropertyBrowser.cpp
	src/FormulaDialogEditor.cpp
	src/FitOptionsBrowser.cpp
	src/FunctionBrowser.cpp
	src/HintingLineEdit.cpp
	src/InstrumentSelector.cpp
  src/IndirectInstrumentConfig.cpp
        src/MantidHelpWindow.cpp
	src/MWDiag.cpp
	src/MWRunFiles.cpp
	src/MessageDisplay.cpp
	src/MultifitSetupDialog.cpp
	src/MuonFitPropertyBrowser.cpp
	src/MuonSequentialFitDialog.cpp
	src/PeakPicker.cpp
	src/ProcessingAlgoWidget.cpp
	src/PropertyHandler.cpp
	src/RangeSelector.cpp
	src/RenameParDialog.cpp
	src/SafeQwtPlot.cpp
	src/SaveWorkspaces.cpp
	src/ScriptEditor.cpp
	src/SelectFunctionDialog.cpp
	src/SelectWorkspacesDialog.cpp
        src/SequentialFitDialog.cpp
	src/SlicingAlgorithmDialog.cpp
	src/StringDialogEditor.cpp
	src/StringEditorFactory.cpp
	src/UserFunctionDialog.cpp
	src/WorkspaceEditorFactory.cpp
	src/WorkspaceSelector.cpp
        src/pqHelpWindow.cxx
	src/pythonCalc.cpp
	src/LineEditWithClear.cpp
)

# Header files with Q_OBJECT that qmake will "moc"
set ( MOC_FILES 
	  inc/MantidQtMantidWidgets/AlgorithmSelectorWidget.h
	  inc/MantidQtMantidWidgets/CheckboxHeader.h
	  inc/MantidQtMantidWidgets/DataSelector.h
    inc/MantidQtMantidWidgets/DiagResults.h
	inc/MantidQtMantidWidgets/FilenameDialogEditor.h
	inc/MantidQtMantidWidgets/FormulaDialogEditor.h
    inc/MantidQtMantidWidgets/FindReplaceDialog.h
    inc/MantidQtMantidWidgets/FindDialog.h
    inc/MantidQtMantidWidgets/FitPropertyBrowser.h
    inc/MantidQtMantidWidgets/FitOptionsBrowser.h
    inc/MantidQtMantidWidgets/FunctionBrowser.h
    inc/MantidQtMantidWidgets/HintingLineEdit.h
    inc/MantidQtMantidWidgets/CatalogSearch.h
    inc/MantidQtMantidWidgets/CatalogSelector.h
    inc/MantidQtMantidWidgets/InstrumentSelector.h
    inc/MantidQtMantidWidgets/IndirectInstrumentConfig.h
    inc/MantidQtMantidWidgets/MantidHelpWindow.h
    inc/MantidQtMantidWidgets/MessageDisplay.h
    inc/MantidQtMantidWidgets/MultifitSetupDialog.h
    inc/MantidQtMantidWidgets/MuonFitPropertyBrowser.h
    inc/MantidQtMantidWidgets/MuonSequentialFitDialog.h
    inc/MantidQtMantidWidgets/MWDiag.h
    inc/MantidQtMantidWidgets/MWRunFiles.h
    inc/MantidQtMantidWidgets/PeakPicker.h
    inc/MantidQtMantidWidgets/pqHelpWindow.h
    inc/MantidQtMantidWidgets/PropertyHandler.h
    inc/MantidQtMantidWidgets/ProcessingAlgoWidget.h
    inc/MantidQtMantidWidgets/pythonCalc.h
    inc/MantidQtMantidWidgets/RangeSelector.h
    inc/MantidQtMantidWidgets/RenameParDialog.h
    inc/MantidQtMantidWidgets/SafeQwtPlot.h
    inc/MantidQtMantidWidgets/SaveWorkspaces.h
    inc/MantidQtMantidWidgets/ScriptEditor.h
    inc/MantidQtMantidWidgets/SelectFunctionDialog.h
    inc/MantidQtMantidWidgets/SelectWorkspacesDialog.h
    inc/MantidQtMantidWidgets/SequentialFitDialog.h
    inc/MantidQtMantidWidgets/SlicingAlgorithmDialog.h
    inc/MantidQtMantidWidgets/StringDialogEditor.h
    inc/MantidQtMantidWidgets/StringEditorFactory.h
    inc/MantidQtMantidWidgets/UserFunctionDialog.h
    inc/MantidQtMantidWidgets/WorkspaceEditorFactory.h
    inc/MantidQtMantidWidgets/WorkspaceSelector.h
    inc/MantidQtMantidWidgets/LineEditWithClear.h
)

# Add the include files are NOT already in MOC_FILES
set ( INC_FILES
	${MOC_FILES}
	inc/MantidQtMantidWidgets/AlgorithmHintStrategy.h
	inc/MantidQtMantidWidgets/CatalogHelper.h
	inc/MantidQtMantidWidgets/WidgetDllOption.h
	inc/MantidQtMantidWidgets/HintStrategy.h
)

# QtDesigner UI files to process
set ( UI_FILES 
	  inc/MantidQtMantidWidgets/DataSelector.ui
    inc/MantidQtMantidWidgets/CatalogSearch.ui
    inc/MantidQtMantidWidgets/CatalogSelector.ui
    inc/MantidQtMantidWidgets/IndirectInstrumentConfig.ui
    inc/MantidQtMantidWidgets/MWDiag.ui
    inc/MantidQtMantidWidgets/MWRunFiles.ui
    inc/MantidQtMantidWidgets/MultifitSetupDialog.ui
    inc/MantidQtMantidWidgets/MuonSequentialFitDialog.ui
    inc/MantidQtMantidWidgets/ProcessingAlgoWidget.ui
    inc/MantidQtMantidWidgets/RenameParDialog.ui
    inc/MantidQtMantidWidgets/SelectFunctionDialog.ui
    inc/MantidQtMantidWidgets/SequentialFitDialog.ui
    inc/MantidQtMantidWidgets/SlicingAlgorithmDialog.ui
    inc/MantidQtMantidWidgets/UserFunctionDialog.ui
    inc/MantidQtMantidWidgets/pqHelpWindow.ui
)

# Python unit tests
set ( TEST_PY_FILES
  test/MWRunFilesTest.py
)

set ( TEST_FILES
  AlgorithmHintStrategyTest.h
)

find_package (Qt4 REQUIRED QtHelp QtWebKit QtNetwork QUIET)
include(${QT_USE_FILE})

include_directories ( ../../QtPropertyBrowser/src )
qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 

find_package ( QScintilla REQUIRED )
include_directories ( ${QSCINTILLA_INCLUDE_DIR} )
add_definitions ( -DQSCINTILLA_DLL )     # Will only have an effect on Windows (as is desired)


# For Windows:
add_definitions ( -DIN_MANTIDQT_MANTIDWIDGETS -DQT_QTPROPERTYBROWSER_IMPORT )
# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidQtMantidWidgets/PrecompiledHeader.h ALL_SRC )
add_library ( MantidWidgets ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidWidgets PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()

target_link_libraries ( MantidWidgets MantidQtAPI QtPropertyBrowser 
    ${QT_LIBRARIES} ${QWT_LIBRARIES} 
    ${QSCINTILLA_LIBRARIES}
    )

###########################################################################
# Testing
###########################################################################

add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidWidgets ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
