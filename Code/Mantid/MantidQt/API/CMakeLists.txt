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
  inc/MantidQtAPI/MantidQwtIMDWorkspaceData.h
  inc/MantidQtAPI/MantidQwtMatrixWorkspaceData.h
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
	inc/MantidQtAPI/MantidQwtWorkspaceData.h
	inc/MantidQtAPI/PropertyWidgetFactory.h
)

set ( UI_FILES 
	inc/MantidQtAPI/ManageUserDirectories.ui 
)

set( TEST_FILES
   test/MantidColorMapTest.h
   test/MantidQwtMatrixWorkspaceDataTest.h
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
# add_definitions ( -DQWT_DLL )
# add_definitions ( -DQWT_MAKEDLL )

# Use a precompiled header where they are supported
ADD_PRECOMPILED_HEADER( inc/MantidQtAPI/PrecompiledHeader.h MantidQtAPI src/PrecompiledHeader.cpp ALL_SRC INC_FILES )
add_library ( MantidQtAPI ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

target_link_libraries ( MantidQtAPI ${CORE_MANTIDLIBS} ${QT_LIBRARIES} ${QWT_LIBRARIES} )

###########################################################################
# Testing
###########################################################################

if ( CXXTEST_FOUND )
  if ( GMOCK_FOUND AND GTEST_FOUND )
    # Make the rest of the tests
    include_directories ( ../../Framework/CurveFitting/inc
                          ../../Framework/DataHandling/inc
                          ../../Framework/Nexus/inc
                          ../../Framework/TestHelpers/inc
                          ../../Framework/MDEvents/inc)
    
    # This variable is used within the cxxtest_add_test macro to build these helper classes into the test executable.
    # It will go out of scope at the end of this file so doesn't need un-setting
    set ( TESTHELPER_SRCS ../../Framework/TestHelpers/src/ComponentCreationHelper.cpp 
                        ../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
                        ../../Framework/TestHelpers/src/SANSInstrumentCreationHelper.cpp
                        ../../Framework/TestHelpers/src/MDEventsTestHelper.cpp )
    
    cxxtest_add_test ( MantidQtAPITest ${TEST_FILES} ${GMOCK_TEST_FILES} )
    target_link_libraries( MantidQtAPITest MantidQtAPI MDEvents DataHandling
    						${GMOCK_LIBRARIES} ${GTEST_LIBRARIES} )
    add_dependencies( GUITests MantidQtAPITest )
    
    # Add to the 'UnitTests' group in VS
    set_property( TARGET MantidQtAPITest PROPERTY FOLDER "UnitTests" )
  endif ()
endif ()

###########################################################################
# DEMO/GUI TESTING APPLICATIONS
###########################################################################

if ( WIN32 )
# Does not work on windows for some reason
else ()

include_directories ( ../../Framework/MDEvents/inc )
include_directories ( ../../Framework/DataObjects/inc )
include_directories ( ../../Framework/Algorithms/inc )
include_directories ( ../../Framework/API/inc )
include_directories ( ../../Framework/DataHandling/inc )
include_directories ( ../../Framework/TestHelpers/inc )
include_directories ( ../../Framework/Geometry/inc )

# Extra, optional targets for demoing GUI elements
add_executable ( PropertyWidgetsDemo EXCLUDE_FROM_ALL ${WIN_CONSOLE} MACOSX_BUNDLE ${ALL_SRC} test/main_PropertyWidgetsDemo.cpp  
                           ${INC_FILES} ${QTIPLOT_C_SRC} ${UI_HDRS} ${RES_FILES} ${MANTID_RC_FILE} )
target_link_libraries ( PropertyWidgetsDemo 
                       ${CORE_MANTIDLIBS} MantidQtAPI MantidWidgets  Geometry
                       QtPropertyBrowser ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${QWT_LIBRARIES} )

# Extra, optional targets for demoing GUI elements
add_executable ( GenericDialogDemo EXCLUDE_FROM_ALL ${WIN_CONSOLE} MACOSX_BUNDLE ${ALL_SRC} test/GenericDialogDemo.cpp  
                           ${INC_FILES} ${QTIPLOT_C_SRC} ${UI_HDRS} ${RES_FILES} ${MANTID_RC_FILE} )
target_link_libraries ( GenericDialogDemo 
                       ${CORE_MANTIDLIBS} MantidQtAPI MantidWidgets  Geometry
                       QtPropertyBrowser ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${QWT_LIBRARIES} )
                       
endif () # (not for windows)


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtAPI DESTINATION ${LIB_DIR} )

set ( TEST_FILES
	test/AlgorithmRunnerTest.h
)
