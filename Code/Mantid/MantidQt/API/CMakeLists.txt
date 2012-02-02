set ( SRC_FILES
	src/AlgorithmDialog.cpp
	src/AlgorithmInputHistory.cpp
	src/GenericDialog.cpp
	src/InterfaceFactory.cpp
	src/InterfaceManager.cpp
	src/ManageUserDirectories.cpp
	src/MantidColorMap.cpp
	src/MantidDialog.cpp
	src/MantidWidget.cpp
	src/PythonRunner.cpp
	src/SyncedCheckboxes.cpp
	src/UserSubWindow.cpp
	src/VatesViewerInterface.cpp
	src/WorkspaceObserver.cpp
	src/MantidQwtIMDWorkspaceData.cpp
	src/MantidQwtMatrixWorkspaceData.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
	inc/MantidQtAPI/AlgorithmDialog.h
	inc/MantidQtAPI/AlgorithmInputHistory.h
	inc/MantidQtAPI/DllOption.h
	inc/MantidQtAPI/FileDialogHandler.h
	inc/MantidQtAPI/GenericDialog.h
	inc/MantidQtAPI/GraphOptions.h
	inc/MantidQtAPI/InterfaceFactory.h
	inc/MantidQtAPI/InterfaceManager.h
	inc/MantidQtAPI/ManageUserDirectories.h
	inc/MantidQtAPI/MantidColorMap.h
	inc/MantidQtAPI/MantidDialog.h
	inc/MantidQtAPI/MantidWidget.h
	inc/MantidQtAPI/MantidQwtIMDWorkspaceData.h
	inc/MantidQtAPI/MantidQwtMatrixWorkspaceData.h
	inc/MantidQtAPI/MantidQwtWorkspaceData.h
	inc/MantidQtAPI/PythonRunner.h
	inc/MantidQtAPI/SyncedCheckboxes.h
	inc/MantidQtAPI/UserSubWindow.h
	inc/MantidQtAPI/VatesViewerInterface.h
	inc/MantidQtAPI/WorkspaceObserver.h
)

set ( MOC_FILES
	inc/MantidQtAPI/AlgorithmDialog.h 
	inc/MantidQtAPI/GenericDialog.h
	inc/MantidQtAPI/ManageUserDirectories.h
	inc/MantidQtAPI/MantidDialog.h
	inc/MantidQtAPI/MantidWidget.h
	inc/MantidQtAPI/PythonRunner.h
	inc/MantidQtAPI/SyncedCheckboxes.h
	inc/MantidQtAPI/UserSubWindow.h
	inc/MantidQtAPI/VatesViewerInterface.h
	inc/MantidQtAPI/WorkspaceObserver.h
	inc/MantidQtAPI/MantidQwtIMDWorkspaceData.h
	inc/MantidQtAPI/MantidQwtMatrixWorkspaceData.h
)

set ( UI_FILES 
	inc/MantidQtAPI/ManageUserDirectories.ui 
)

set( TEST_FILES
   test/MantidColorMapTest.h
)

include_directories ( inc )
include_directories ( ../MantidWidgets/inc )
include_directories ( ../MantidWidgets )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# For Windows:
add_definitions ( -DIN_MANTIDQT_API )
add_definitions ( -DQWT_DLL )
add_definitions ( -DQWT_MAKEDLL )

# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidQtAPI/PrecompiledHeader.h MantidQtAPI src/PrecompiledHeader.cpp ALL_SRC INC_FILES )
add_library ( MantidQtAPI ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

target_link_libraries ( MantidQtAPI ${CORE_MANTIDLIBS} ${QT_LIBRARIES} ${QWT_LIBRARIES} )

###########################################################################
# Testing
###########################################################################

if ( CXXTEST_FOUND )
  if ( GMOCK_FOUND AND GTEST_FOUND )
    cxxtest_add_test ( MantidQtAPITest ${TEST_FILES} ${GMOCK_TEST_FILES} )
    target_link_libraries( MantidQtAPITest MantidQtAPI ${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} )
    add_dependencies( GUITests MantidQtAPITest )
    # Add to the 'UnitTests' group in VS
    set_property( TARGET MantidQtAPITest PROPERTY FOLDER "UnitTests" )
  endif ()
endif ()


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtAPI DESTINATION ${LIB_DIR} )

set ( TEST_FILES
	test/SyncedCheckboxesTest.h
)
