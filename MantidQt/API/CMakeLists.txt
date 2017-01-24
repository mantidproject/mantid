 set ( SRC_FILES
	src/AlgorithmDialog.cpp
	src/AlgorithmInputHistory.cpp
	src/AlgorithmPropertiesWidget.cpp
	src/AlgorithmRunner.cpp
	src/BatchAlgorithmRunner.cpp
	src/BoolPropertyWidget.cpp
	src/FileDialogHandler.cpp
	src/FilePropertyWidget.cpp
	src/GenericDialog.cpp
	src/HelpWindow.cpp
	src/FlowLayout.cpp
	src/InterfaceFactory.cpp
	src/InterfaceManager.cpp
	src/ListPropertyWidget.cpp
	src/ManageUserDirectories.cpp
	src/MantidColorMap.cpp
	src/MantidDesktopServices.cpp
	src/MantidDialog.cpp
	src/MantidHelpInterface.cpp
	src/MantidQwtIMDWorkspaceData.cpp
	src/MantidQwtWorkspaceData.cpp
	src/MantidWidget.cpp
	src/MdConstants.cpp
	src/MdPlottingCmapsProvider.cpp
	src/MdSettings.cpp
	src/Message.cpp
	src/NonOrthogonal.cpp
	src/MWRunFiles.cpp
	src/OptionsPropertyWidget.cpp
	src/pixmaps.cpp
	src/PlotAxis.cpp
	src/PowerScaleEngine.cpp
	src/PropertyWidget.cpp
	src/PropertyWidgetFactory.cpp
	src/PythonRunner.cpp
	src/PythonThreading.cpp
	src/QScienceSpinBox.cpp
	src/QtSignalChannel.cpp
	src/QwtRasterDataMD.cpp
	src/QwtRasterDataMDNonOrthogonal.cpp
	src/QwtWorkspaceBinData.cpp
	src/QwtWorkspaceSpectrumData.cpp
	src/RepoModel.cpp
	src/ScaleEngine.cpp
	src/ScriptRepositoryView.cpp
	src/SelectionNotificationService.cpp
	src/SignalBlocker.cpp
	src/SignalRange.cpp
	src/SyncedCheckboxes.cpp
	src/TextPropertyWidget.cpp
	src/TSVSerialiser.cpp
	src/UserSubWindow.cpp
	src/VatesViewerInterface.cpp
	src/WidgetScrollbarDecorator.cpp
  src/WindowIcons.cpp
	src/WorkspaceObserver.cpp
	src/WorkspaceIcons.cpp
)

set ( MOC_FILES
  inc/MantidQtAPI/AlgorithmDialog.h
  inc/MantidQtAPI/AlgorithmRunner.h
  inc/MantidQtAPI/AlgorithmPropertiesWidget.h
  inc/MantidQtAPI/BatchAlgorithmRunner.h
  inc/MantidQtAPI/BoolPropertyWidget.h
  inc/MantidQtAPI/FilePropertyWidget.h
  inc/MantidQtAPI/GenericDialog.h
  inc/MantidQtAPI/ListPropertyWidget.h
  inc/MantidQtAPI/ManageUserDirectories.h
  inc/MantidQtAPI/MantidDialog.h
  inc/MantidQtAPI/MantidHelpInterface.h
  inc/MantidQtAPI/MantidWidget.h
  inc/MantidQtAPI/MWRunFiles.h
  inc/MantidQtAPI/OptionsPropertyWidget.h
  inc/MantidQtAPI/PropertyWidget.h
  inc/MantidQtAPI/PythonRunner.h
  inc/MantidQtAPI/QScienceSpinBox.h
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
	inc/MantidQtAPI/FlowLayout.h
	inc/MantidQtAPI/GraphOptions.h
	inc/MantidQtAPI/DistributionOptions.h
	inc/MantidQtAPI/HelpWindow.h
	inc/MantidQtAPI/InterfaceFactory.h
	inc/MantidQtAPI/InterfaceManager.h
	inc/MantidQtAPI/IProjectSerialisable.h
	inc/MantidQtAPI/MantidColorMap.h
	inc/MantidQtAPI/MantidDesktopServices.h
	inc/MantidQtAPI/MantidQwtIMDWorkspaceData.h
	inc/MantidQtAPI/MantidQwtWorkspaceData.h
	inc/MantidQtAPI/MantidAlgorithmMetatype.h
	inc/MantidQtAPI/MdConstants.h
	inc/MantidQtAPI/MdPlottingCmapsProvider.h
	inc/MantidQtAPI/MdSettings.h
	inc/MantidQtAPI/NonOrthogonal.h
	inc/MantidQtAPI/pixmaps.h
	inc/MantidQtAPI/PlotAxis.h
	inc/MantidQtAPI/PowerScaleEngine.h
	inc/MantidQtAPI/PropertyWidgetFactory.h
	inc/MantidQtAPI/PythonSystemHeader.h
	inc/MantidQtAPI/PythonThreading.h
	inc/MantidQtAPI/QScienceSpinBox.h
	inc/MantidQtAPI/QwtRasterDataMD.h
	inc/MantidQtAPI/QwtRasterDataMDNonOrthogonal.h
	inc/MantidQtAPI/QwtWorkspaceBinData.h
	inc/MantidQtAPI/QwtWorkspaceSpectrumData.h
	inc/MantidQtAPI/ScaleEngine.h
	inc/MantidQtAPI/ScriptRepositoryView.h
	inc/MantidQtAPI/SelectionNotificationService.h
	inc/MantidQtAPI/SignalBlocker.h
	inc/MantidQtAPI/SignalRange.h
	inc/MantidQtAPI/TSVSerialiser.h
	inc/MantidQtAPI/qwt_compat.h
	inc/MantidQtAPI/WidgetScrollbarDecorator.h
	inc/MantidQtAPI/WindowIcons.h
	inc/MantidQtAPI/WorkspaceIcons.h
)

set ( UI_FILES
  inc/MantidQtAPI/ManageUserDirectories.ui
  inc/MantidQtAPI/MWRunFiles.ui
  inc/MantidQtAPI/ScriptRepositoryView.ui
)

# Python unit tests
set ( TEST_PY_FILES
  test/MWRunFilesTest.py
)

set( TEST_FILES
   BatchAlgorithmRunnerTest.h
   FileDialogHandlerTest.h
   InterfaceManagerTest.h
   MantidColorMapTest.h
   NonOrthogonalTest.h
   PlotAxisTest.h
   QwtWorkspaceBinDataTest.h
   QwtWorkspaceSpectrumDataTest.h
   SelectionNotificationServiceTest.h
   SignalRangeTest.h
   SignalBlockerTest.h
)

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )
set ( ALL_SRC ${SRC_FILES} ${MOCCED_FILES} )
qt4_wrap_ui ( UI_HDRS ${UI_FILES} )

if(MAKE_VATES AND ParaView_FOUND)
  include( ${PARAVIEW_USE_FILE} )
endif()

# Python includes
include_directories ( ${PYTHON_INCLUDE_PATH})
if(MAKE_VATES AND ParaView_FOUND)
  include_directories (SYSTEM ${PARAVIEW_INCLUDE_DIRS})
endif()
# Use a precompiled header where they are supported
enable_precompiled_headers( inc/MantidQtAPI/PrecompiledHeader.h ALL_SRC )
add_library ( MantidQtAPI ${ALL_SRC} ${INC_FILES} ${UI_HDRS} )

set_target_properties ( MantidQtAPI PROPERTIES COMPILE_DEFINITIONS IN_MANTIDQT_API )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( MantidQtAPI PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Libraries")
endif ()

target_link_libraries ( MantidQtAPI LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
  ${CORE_MANTIDLIBS} ${QT_LIBRARIES} ${QWT_LIBRARIES}
  ${POCO_LIBRARIES} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES}
)

if(MAKE_VATES AND ParaView_FOUND)
  target_link_libraries (MantidQtAPI LINK_PRIVATE
  vtkPVServerManagerRendering
  ${vtkjsoncpp_LIBRARIES}
  )
endif()

###########################################################################
# Windows: Generate a qt.conf file so that the dev build finds the qt
# plugins in third party. Done in two steps to get it into the build
# directory that is selected at build time.
# This file is NOT packaged. See installers/WinInstaller for the package
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
