set ( SRC_FILES src/AlgorithmDialog.cpp
                src/AlgorithmInputHistory.cpp
                src/GenericDialog.cpp
                src/InterfaceFactory.cpp
                src/InterfaceManager.cpp
                src/ManageUserDirectories.cpp
                src/MantidDialog.cpp
                src/MantidWidget.cpp
                src/PythonRunner.cpp
                src/UserSubWindow.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES inc/MantidQtAPI/AlgorithmDialog.h
                inc/MantidQtAPI/AlgorithmInputHistory.h
                inc/MantidQtAPI/DllOption.h
                inc/MantidQtAPI/FileDialogHandler.h
                inc/MantidQtAPI/GenericDialog.h
                inc/MantidQtAPI/InterfaceFactory.h
                inc/MantidQtAPI/InterfaceManager.h
                inc/MantidQtAPI/ManageUserDirectories.h
                inc/MantidQtAPI/MantidDialog.h
                inc/MantidQtAPI/MantidWidget.h
                inc/MantidQtAPI/PythonRunner.h
                inc/MantidQtAPI/UserSubWindow.h
)

set ( MOC_FILES inc/MantidQtAPI/AlgorithmDialog.h 
                inc/MantidQtAPI/GenericDialog.h
                inc/MantidQtAPI/ManageUserDirectories.h
                inc/MantidQtAPI/MantidDialog.h
                inc/MantidQtAPI/MantidWidget.h
                inc/MantidQtAPI/PythonRunner.h
                inc/MantidQtAPI/UserSubWindow.h
)

set ( UI_FILES inc/MantidQtAPI/ManageUserDirectories.ui )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# For Windows:
add_definitions ( -DIN_MANTIDQT_API )

add_library ( MantidQtAPI ${SRC_FILES} ${INC_FILES} ${MOCCED_FILES} ${UI_HDRS} )
# Add to the 'MantidQt' group in VS
set_property ( TARGET MantidQtAPI PROPERTY FOLDER "MantidQt" )

target_link_libraries ( MantidQtAPI ${CORE_MANTIDLIBS} ${QT_LIBRARIES} )