set ( SRC_FILES src/DiagResults.cpp
                src/ICatAdvancedSearch.cpp
                src/ICatInvestigation.cpp
                src/ICatMyDataSearch.cpp
                src/ICatSearch.cpp
                src/ICatUtils.cpp
                src/InstrumentSelector.cpp
                src/MWDiag.cpp
                src/MWRunFiles.cpp
                src/pythonCalc.cpp
                src/RangeSelector.cpp
                src/RenameParDialog.cpp
                src/SaveWorkspaces.cpp
                src/UserFunctionDialog.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES inc/MantidQtMantidWidgets/DiagResults.h
                inc/MantidQtMantidWidgets/ICatAdvancedSearch.h
                inc/MantidQtMantidWidgets/ICatInvestigation.h
                inc/MantidQtMantidWidgets/ICatMyDataSearch.h
                inc/MantidQtMantidWidgets/ICatSearch.h
                inc/MantidQtMantidWidgets/ICatUtils.h
                inc/MantidQtMantidWidgets/InstrumentSelector.h
                inc/MantidQtMantidWidgets/MWDiag.h
                inc/MantidQtMantidWidgets/MWRunFiles.h
                inc/MantidQtMantidWidgets/pythonCalc.h
                inc/MantidQtMantidWidgets/RangeSelector.h
                inc/MantidQtMantidWidgets/RenameParDialog.h
                inc/MantidQtMantidWidgets/SaveWorkspaces.h
                inc/MantidQtMantidWidgets/UserFunctionDialog.h
                inc/MantidQtMantidWidgets/WidgetDllOption.h
)

set ( MOC_FILES inc/MantidQtMantidWidgets/DiagResults.h
                inc/MantidQtMantidWidgets/ICatAdvancedSearch.h
                inc/MantidQtMantidWidgets/ICatInvestigation.h
                inc/MantidQtMantidWidgets/ICatMyDataSearch.h
                inc/MantidQtMantidWidgets/ICatSearch.h
                inc/MantidQtMantidWidgets/ICatUtils.h
                inc/MantidQtMantidWidgets/InstrumentSelector.h
                inc/MantidQtMantidWidgets/MWDiag.h
                inc/MantidQtMantidWidgets/MWRunFiles.h
                inc/MantidQtMantidWidgets/pythonCalc.h
                inc/MantidQtMantidWidgets/RangeSelector.h
                inc/MantidQtMantidWidgets/RenameParDialog.h
                inc/MantidQtMantidWidgets/SaveWorkspaces.h
                inc/MantidQtMantidWidgets/UserFunctionDialog.h
)

set ( UI_FILES inc/MantidQtMantidWidgets/ICatAdvancedSearch.ui
               inc/MantidQtMantidWidgets/ICatInvestigation.ui
               inc/MantidQtMantidWidgets/ICatMyDataSearch.ui
               inc/MantidQtMantidWidgets/ICatSearch.ui
               inc/MantidQtMantidWidgets/MWDiag.ui
               inc/MantidQtMantidWidgets/MWRunFiles.ui
               inc/MantidQtMantidWidgets/RenameParDialog.ui
               inc/MantidQtMantidWidgets/UserFunctionDialog.ui
)

include_directories ( inc )
include_directories ( ${QWT_INCLUDE_DIR} )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# For Windows:
add_definitions ( -DIN_MANTIDQT_MANTIDWIDGETS )

add_library ( MantidWidgets ${SRC_FILES} ${INC_FILES} ${MOCCED_FILES} ${UI_HDRS} )
# Add to the 'MantidQt' group in VS
set_property ( TARGET MantidWidgets PROPERTY FOLDER "MantidQt" )

target_link_libraries ( MantidWidgets MantidQtAPI ${QT_LIBRARIES} ${QWT_LIBRARIES} )