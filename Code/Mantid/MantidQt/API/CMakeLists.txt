set ( SRC_FILES
	src/AlgorithmDialog.cpp
	src/AlgorithmInputHistory.cpp
	src/AlgorithmPropertiesWidget.cpp
	src/AlgorithmRunner.cpp
	src/BoolPropertyWidget.cpp
	src/FilePropertyWidget.cpp
	src/GenericDialog.cpp
	src/InterfaceFactory.cpp
	src/InterfaceManager.cpp
	src/ManageUserDirectories.cpp
	src/MantidColorMap.cpp
	src/MantidDialog.cpp
	src/MantidQwtIMDWorkspaceData.cpp
	src/MantidQwtMatrixWorkspaceData.cpp
	src/MantidWidget.cpp
	src/OptionsPropertyWidget.cpp
	src/PropertyWidget.cpp
	src/PropertyWidgetFactory.cpp
	src/PythonRunner.cpp
	src/SyncedCheckboxes.cpp
	src/TextPropertyWidget.cpp
	src/UserSubWindow.cpp
	src/VatesViewerInterface.cpp
	src/WorkspaceObserver.cpp
)

set ( MOC_FILES
  inc/MantidQtAPI/AlgorithmDialog.h 
  inc/MantidQtAPI/AlgorithmRunner.h 
  inc/MantidQtAPI/AlgorithmPropertiesWidget.h
  inc/MantidQtAPI/BoolPropertyWidget.h
  inc/MantidQtAPI/FilePropertyWidget.h
  inc/MantidQtAPI/GenericDialog.h
  inc/MantidQtAPI/ManageUserDirectories.h
  inc/MantidQtAPI/MantidDialog.h
  inc/MantidQtAPI/MantidWidget.h
  inc/MantidQtAPI/OptionsPropertyWidget.h
  inc/MantidQtAPI/PropertyWidget.h
  inc/MantidQtAPI/PythonRunner.h
  inc/MantidQtAPI/SyncedCheckboxes.h
  inc/MantidQtAPI/TextPropertyWidget.h
  inc/MantidQtAPI/UserSubWindow.h
  inc/MantidQtAPI/VatesViewerInterface.h
  inc/MantidQtAPI/WorkspaceObserver.h
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
	${MOC_FILES}
	inc/MantidQtAPI/AlgorithmInputHistory.h
	inc/MantidQtAPI/AlgorithmRunner.h
	inc/MantidQtAPI/DllOption.h
	inc/MantidQtAPI/FileDialogHandler.h
	inc/MantidQtAPI/GraphOptions.h
	inc/MantidQtAPI/InterfaceFactory.h
	inc/MantidQtAPI/InterfaceManager.h
	inc/MantidQtAPI/MantidColorMap.h
    inc/MantidQtAPI/MantidQwtIMDWorkspaceData.h
    inc/MantidQtAPI/MantidQwtMatrixWorkspaceData.h
	inc/MantidQtAPI/MantidQwtWorkspaceData.h
	inc/MantidQtAPI/PropertyWidgetFactory.h
)

set ( UI_FILES 
	inc/MantidQtAPI/ManageUserDirectories.ui 
)

set( TEST_FILES
   MantidColorMapTest.h
   MantidQwtMatrixWorkspaceDataTest.h
)

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )

# For Windows:
add_definitions ( -DIN_MANTIDQT_API )
# add_definitions ( -DQWT_DLL )
# add_definitions ( -DQWT_MAKEDLL )

# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidQtAPI/PrecompiledHeader.h MantidQtAPI src/PrecompiledHeader.cpp ALL_SRC INC_FILES )
add_library ( MantidQtAPI ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

target_link_libraries ( MantidQtAPI ${CORE_MANTIDLIBS} ${QT_LIBRARIES} ${QWT_LIBRARIES} )

###########################################################################
# Testing
###########################################################################

add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtAPI ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )

