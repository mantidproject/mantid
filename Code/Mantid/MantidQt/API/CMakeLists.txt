set ( SRC_FILES
  src/AlgorithmDialog.cpp
  src/AlgorithmInputHistory.cpp
  src/AlgorithmPropertiesWidget.cpp
  src/AlgorithmRunner.cpp
  src/BatchAlgorithmRunner.cpp
  src/BoolPropertyWidget.cpp
  src/FilePropertyWidget.cpp
  src/GenericDialog.cpp
  src/HelpWindow.cpp
  src/InterfaceFactory.cpp
  src/InterfaceManager.cpp
  src/ManageUserDirectories.cpp
  src/MantidColorMap.cpp
  src/MantidDialog.cpp
  src/MantidHelpInterface.cpp
  src/MantidQwtIMDWorkspaceData.cpp
  src/MantidWidget.cpp
  src/MdConstants.cpp
  src/MdSettings.cpp
  src/Message.cpp
  src/OptionsPropertyWidget.cpp
  src/PlotAxis.cpp
  src/PropertyWidget.cpp
  src/PropertyWidgetFactory.cpp
  src/PythonRunner.cpp
  src/QtSignalChannel.cpp
  src/QwtRasterDataMD.cpp
  src/QwtWorkspaceBinData.cpp
  src/QwtWorkspaceSpectrumData.cpp
  src/SignalRange.cpp
  src/ScriptRepositoryView.cpp
  src/RepoModel.cpp
  src/SyncedCheckboxes.cpp
  src/TextPropertyWidget.cpp
  src/UserSubWindow.cpp
  src/VatesViewerInterface.cpp
  src/WorkspaceObserver.cpp
  src/SelectionNotificationService.cpp
)

set ( MOC_FILES
  inc/MantidQtAPI/AlgorithmDialog.h 
  inc/MantidQtAPI/AlgorithmRunner.h 
  inc/MantidQtAPI/AlgorithmPropertiesWidget.h
  inc/MantidQtAPI/BatchAlgorithmRunner.h 
  inc/MantidQtAPI/BoolPropertyWidget.h
  inc/MantidQtAPI/FilePropertyWidget.h
  inc/MantidQtAPI/GenericDialog.h
  inc/MantidQtAPI/ManageUserDirectories.h
  inc/MantidQtAPI/MantidDialog.h
  inc/MantidQtAPI/MantidHelpInterface.h
  inc/MantidQtAPI/MantidWidget.h
  inc/MantidQtAPI/OptionsPropertyWidget.h
  inc/MantidQtAPI/PropertyWidget.h
  inc/MantidQtAPI/PythonRunner.h
  inc/MantidQtAPI/QtSignalChannel.h
  inc/MantidQtAPI/ScriptRepositoryView.h
  inc/MantidQtAPI/RepoTreeView.h
  inc/MantidQtAPI/RepoModel.h
  inc/MantidQtAPI/SyncedCheckboxes.h
  inc/MantidQtAPI/TextPropertyWidget.h
  inc/MantidQtAPI/UserSubWindow.h
  inc/MantidQtAPI/VatesViewerInterface.h
  inc/MantidQtAPI/WorkspaceObserver.h
  inc/MantidQtAPI/SelectionNotificationService.h
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES
  ${MOC_FILES}
  inc/MantidQtAPI/AlgorithmInputHistory.h
  inc/MantidQtAPI/AlgorithmRunner.h
  inc/MantidQtAPI/BatchAlgorithmRunner.h
  inc/MantidQtAPI/DllOption.h
  inc/MantidQtAPI/FileDialogHandler.h
  inc/MantidQtAPI/GraphOptions.h
  inc/MantidQtAPI/HelpWindow.h
  inc/MantidQtAPI/InterfaceFactory.h
  inc/MantidQtAPI/InterfaceManager.h
  inc/MantidQtAPI/PlotAxis.h
  inc/MantidQtAPI/MantidColorMap.h
  inc/MantidQtAPI/MantidQwtIMDWorkspaceData.h
  inc/MantidQtAPI/MantidQwtWorkspaceData.h
  inc/MantidQtAPI/MdConstants.h
  inc/MantidQtAPI/MdSettings.h
  inc/MantidQtAPI/PropertyWidgetFactory.h
  inc/MantidQtAPI/QwtRasterDataMD.h
  inc/MantidQtAPI/QwtWorkspaceBinData.h
  inc/MantidQtAPI/QwtWorkspaceSpectrumData.h
  inc/MantidQtAPI/ScriptRepositoryView.h
  inc/MantidQtAPI/SelectionNotificationService.h
  inc/MantidQtAPI/SignalRange.h
)

set ( UI_FILES 
  inc/MantidQtAPI/ManageUserDirectories.ui
  inc/MantidQtAPI/ScriptRepositoryView.ui 
)

set( TEST_FILES
   BatchAlgorithmRunnerTest.h
   InterfaceManagerTest.h
   MantidColorMapTest.h
   PlotAxisTest.h
   QwtWorkspaceBinDataTest.h
   QwtWorkspaceSpectrumDataTest.h
   SelectionNotificationServiceTest.h
   SignalRangeTest.h
)

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )

# For Windows:
# add_definitions ( -DIN_MANTIDQT_API )
# add_definitions ( -DQWT_DLL )
# add_definitions ( -DQWT_MAKEDLL )

# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidQtAPI/PrecompiledHeader.h ALL_SRC )
add_library ( MantidQtAPI ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

set_target_properties ( MantidQtAPI PROPERTIES COMPILE_DEFINITIONS IN_MANTIDQT_API )

target_link_libraries ( MantidQtAPI ${CORE_MANTIDLIBS} ${QT_LIBRARIES} ${QWT_LIBRARIES} )

###########################################################################
# Windows: Generate a qt.conf file so that the dev build finds the qt
# plugins in third party. Done in two steps to get it into the build
# directory that is selected at build time.
# This file is NOT packaged. See Installers/WinInstaller for the package
# version.
###########################################################################
if ( WIN32 )
  set ( QT_CONF qt.conf )
  configure_file ( qt.conf.template ${CMAKE_CURRENT_BINARY_DIR}/${QT_CONF} @ONLY )
  add_custom_command ( TARGET MantidQtAPI POST_BUILD
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different
                               ${CMAKE_CURRENT_BINARY_DIR}/${QT_CONF} 
                               ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${QT_CONF} )
endif()

###########################################################################
# Testing
###########################################################################

add_subdirectory ( test )

###########################################################################
# Installation settings
###########################################################################

install ( TARGETS MantidQtAPI ${SYSTEM_PACKAGE_TARGET} DESTINATION ${LIB_DIR} )


set ( TEST_FILES
  ScriptRepositoryViewTest.h
)
