# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required( VERSION 2.6 )
# Define the project name
project( Vates )

include( CommonVatesSetup )

# Select either building Paraview plugins
set( USE_PARAVIEW ON CACHE BOOL "Create paraview plugins. " )

# Paraview dependencies built if Paraview is found
find_package( ParaView )
if( ParaView_FOUND AND USE_PARAVIEW )
  include( ${PARAVIEW_USE_FILE} )
  add_subdirectory( VatesAPI )
  add_subdirectory( ParaviewPlugins )
  add_subdirectory( VatesSimpleGui )
  
  if ( ENABLE_CPACK )
    if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" ) #Only handling windows. TODO. Verify cross-platform
      set ( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Visualisation and Analysis Toolkit ExtensionS to Mantid" )
      set ( CPACK_PACKAGE_VENDOR "ISIS Rutherford Appleton Laboratory and NScD Oak Ridge National Laboratory" )
      set ( CPACK_PACKAGE_URL http://www.mantidproject.org/ )
      set ( CPACK_PACKAGE_CONTACT mantid-help@mantidproject.org )
      set ( CPACK_PACKAGE_NAME Mantid_Vates)
      set ( CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/Installers/WinInstaller/License.txt)
      set ( CPACK_PACKAGE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${MtdVersion_WC_LAST_CHANGED_REV} )
      set ( CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR} )
      set ( CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR} )
      set ( CPACK_PACKAGE_VERSION_PATCH ${MtdVersion_WC_LAST_CHANGED_REV} )
	
	  #TODO. There is a bug in cpack with the following line. Remove or workaround.
      set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.png" )
	  set(CPACK_NSIS_MUI_ICON	"${CMAKE_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.ico" )
	  set(CPACK_NSIS_MUI_UNIICON	"${CMAKE_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.ico" )
	
	  #TODO: the following project selections can be avoided by ensuring that each subdirectory of VATES has a project.
	  set ( CPACK_INSTALL_CMAKE_PROJECTS "Vates/ParaviewPlugins/ParaViewReaders/EventNexusReader;MantidParaViewEventNexusReaderSMPlugin;ALL;/")
      set ( CPACK_INSTALL_CMAKE_PROJECTS "Vates/ParaviewPlugins/ParaViewReaders/SQWReader;MantidParaViewSQWReaderSMPlugin;ALL;/")
      set ( CPACK_INSTALL_CMAKE_PROJECTS "Vates/ParaviewPlugins/ParaViewFilters/RebinningCutterOperator;MantidParaViewRebinningCutterSMPlugin;ALL;/")
      set ( CPACK_INSTALL_CMAKE_PROJECTS "Vates/ParaviewPlugins/ParaViewWidgets/EventNexusReaderObjectPanel;MantidParaViewEventNexusReaderObjectPanel;ALL;/")
      set ( CPACK_INSTALL_CMAKE_PROJECTS "Vates/ParaviewPlugins/ParaViewWidgets/RebinningCutterObjectPanel;MantidParaViewRebinningCutterObjectPanel;ALL;/")
      set ( CPACK_INSTALL_CMAKE_PROJECTS "Vates/ParaviewPlugins/ParaViewWidgets/QtWidgets;MantidParaViewQtWidgets;ALL/")
      set ( CPACK_INSTALL_CMAKE_PROJECTS "Vates/ParaviewPlugins/ParaViewWidgets/RebinningCutterToolBarExtension;MantidParaViewRebinningCutterToolBarActions;ALL/")
      set ( CPACK_INSTALL_CMAKE_PROJECTS "Vates;VatesAPI;ALL;/")
      set ( CPACK_INSTALL_CMAKE_PROJECTS "Vates;VatesSimpleGui;ALL;/")
      set (CPACK_PACKAGE_EXECUTABLES "VatesSimpleGui;VatesSimpleGui")
	
	  #Check that Mantid exists. Warn if not installing over Mantid.
	  set (CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ClearErrors 
	  ReadEnvStr $R0 'MANTIDPATH'
	  IfErrors 0 +2
	    messagebox mb_ok 'Mantid does not appear to be installed.'
	  ;Var /GLOBAL InstallBinPath 
	  ;StrCpy $InstallBinPath '$INSTDIR$\\bin'
	  ;!if $InstallBinPath != $R0
	    ;messagebox mb_ok 'You are installing VATES to $InstallBinPath. This is a different location from Mantid $R0. This is not recommended!'
      ;!endif
	  ")
	
	  #set (CPACK_NSIS_EXTRA_INSTALL_COMMANDS "${EnvVarUpdate} $0 \\\"PATH\\\" \\\"A\\\" \\\"HKLM\\\" \\\"x\\\"")
	
	  set (CPACK_GENERATOR "NSIS")
      message ( STATUS "Package targets Mantid-Vates Windows NSIS Installer available." )
    endif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

   # run cpack configuration
  include ( CPack )
endif ()
  
  
else( ParaView_FOUND AND USE_PARAVIEW )
  message( STATUS "Vates-Paraview plugins and widgets will not be built." ) 
endif( ParaView_FOUND AND USE_PARAVIEW )
