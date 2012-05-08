set ( SRC_FILES
	src/AlgorithmSelectorWidget.cpp
	src/DiagResults.cpp
        src/FindReplaceDialog.cpp
        src/FindDialog.cpp
	src/FitPropertyBrowser.cpp
	src/FunctionBrowser.cpp
	src/ICatAdvancedSearch.cpp
	src/ICatInvestigation.cpp
	src/ICatMyDataSearch.cpp
	src/ICatSearch.cpp
	src/ICatUtils.cpp
	src/InstrumentSelector.cpp
	src/MWDiag.cpp
	src/MWRunFiles.cpp
	src/MultifitSetupDialog.cpp
	src/MuonFitPropertyBrowser.cpp
	src/PropertyHandler.cpp
    src/ProcessingAlgoWidget.cpp
	src/RangeSelector.cpp
	src/RenameParDialog.cpp
	src/SafeQwtPlot.cpp
	src/SaveWorkspaces.cpp
	src/ScriptEditor.cpp
	src/SelectFunctionDialog.cpp
	src/SelectWorkspacesDialog.cpp
	src/SequentialFitDialog.cpp
	src/UserFunctionDialog.cpp
	src/WorkspaceSelector.cpp
	src/pythonCalc.cpp
)

set ( SRC_UNITY_IGNORE_FILES 
    src/ICatAdvancedSearch.cpp
    src/ICatSearch.cpp
)

# Header files with Q_OBJECT that qmake will "moc"
set ( MOC_FILES 
	inc/MantidQtMantidWidgets/AlgorithmSelectorWidget.h
    inc/MantidQtMantidWidgets/DiagResults.h
    inc/MantidQtMantidWidgets/FindReplaceDialog.h
    inc/MantidQtMantidWidgets/FindDialog.h
    inc/MantidQtMantidWidgets/FitPropertyBrowser.h
    inc/MantidQtMantidWidgets/FunctionBrowser.h
    inc/MantidQtMantidWidgets/ICatAdvancedSearch.h
    inc/MantidQtMantidWidgets/ICatInvestigation.h
    inc/MantidQtMantidWidgets/ICatMyDataSearch.h
    inc/MantidQtMantidWidgets/ICatSearch.h
    inc/MantidQtMantidWidgets/ICatUtils.h
    inc/MantidQtMantidWidgets/InstrumentSelector.h
    inc/MantidQtMantidWidgets/MultifitSetupDialog.h
    inc/MantidQtMantidWidgets/MuonFitPropertyBrowser.h
    inc/MantidQtMantidWidgets/MWDiag.h
    inc/MantidQtMantidWidgets/MWRunFiles.h
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
    inc/MantidQtMantidWidgets/UserFunctionDialog.h
    inc/MantidQtMantidWidgets/WorkspaceSelector.h
)

# Add the include files are NOT already in MOC_FILES
set ( INC_FILES
    ${MOC_FILES}
	inc/MantidQtMantidWidgets/WidgetDllOption.h
)

# QtDesigner UI files to process
set ( UI_FILES 
    inc/MantidQtMantidWidgets/ICatAdvancedSearch.ui
    inc/MantidQtMantidWidgets/ICatInvestigation.ui
    inc/MantidQtMantidWidgets/ICatMyDataSearch.ui
    inc/MantidQtMantidWidgets/ICatSearch.ui
    inc/MantidQtMantidWidgets/MultifitSetupDialog.ui
    inc/MantidQtMantidWidgets/MWDiag.ui
    inc/MantidQtMantidWidgets/MWRunFiles.ui
    inc/MantidQtMantidWidgets/ProcessingAlgoWidget.ui
    inc/MantidQtMantidWidgets/RenameParDialog.ui
    inc/MantidQtMantidWidgets/SelectFunctionDialog.ui
    inc/MantidQtMantidWidgets/SequentialFitDialog.ui
    inc/MantidQtMantidWidgets/UserFunctionDialog.ui
)


include_directories ( inc )
include_directories ( ../../QtPropertyBrowser/src )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

find_package ( QScintilla REQUIRED )
include_directories ( ${QSCINTILLA_INCLUDE_DIR} )
add_definitions ( -DQSCINTILLA_DLL )     # Will only have an effect on Windows (as is desired)


# For Windows:
add_definitions ( -DIN_MANTIDQT_MANTIDWIDGETS )
# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidQtMantidWidgets/PrecompiledHeader.h MantidQtMantidWidgets src/PrecompiledHeader.cpp ALL_SRC INC_FILES )
add_library ( MantidWidgets ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

target_link_libraries ( MantidWidgets MantidQtAPI QtPropertyBrowser 
    ${QT_LIBRARIES} ${QWT_LIBRARIES} 
    ${QSCINTILLA_LIBRARIES}
    )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidWidgets ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )
