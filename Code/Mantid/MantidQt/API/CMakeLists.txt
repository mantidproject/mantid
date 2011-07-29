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
                src/WorkspaceObserver.cpp
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
                inc/MantidQtAPI/WorkspaceObserver.h
)

set ( MOC_FILES inc/MantidQtAPI/AlgorithmDialog.h 
                inc/MantidQtAPI/GenericDialog.h
                inc/MantidQtAPI/ManageUserDirectories.h
                inc/MantidQtAPI/MantidDialog.h
                inc/MantidQtAPI/MantidWidget.h
                inc/MantidQtAPI/PythonRunner.h
                inc/MantidQtAPI/UserSubWindow.h
                inc/MantidQtAPI/WorkspaceObserver.h
)

set ( UI_FILES inc/MantidQtAPI/ManageUserDirectories.ui 
)

include_directories ( inc )
include_directories ( ${QWT_INCLUDE_DIR} )
include_directories ( ../MantidWidgets/inc )
include_directories ( ../MantidWidgets )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )
# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidQtAPI/PrecompiledHeader.h MantidQtAPI MantidQtAPI ALL_SRC INC_FILES )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# For Windows:
add_definitions ( -DIN_MANTIDQT_API )

add_library ( MantidQtAPI ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

target_link_libraries ( MantidQtAPI ${CORE_MANTIDLIBS} ${QT_LIBRARIES} ${QWT_LIBRARIES} )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtAPI DESTINATION ${LIB_DIR} )
