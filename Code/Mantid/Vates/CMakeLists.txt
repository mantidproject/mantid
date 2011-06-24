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
  
  if( ENABLE_CPACK )
    message( STATUS "In Vates CPack" )
    set( CPACK_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/CPackVatesConfig.cmake" )
    set( CPACK_PACKAGE_NAME Mantid_Vates )
    include( CPackCommon )
    if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
      include( CPackLinuxSetup )
    endif()

    set( CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/Images/MantidPlot_Icon_32offset.png" )
    set( CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/Installers/WinInstaller/License.txt )
	
    if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
      set( TARGET_TYPE LIBRARY )
      message( STATUS "VATES CPACK_PACKAGE_FILE_NAME = ${CPACK_PACKAGE_FILE_NAME}" )
      message( STATUS "Linux installer available for Mantid-Vates")
    endif()

    if( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )
      # Windows needs to have this for shared libraries install targets
      set( TARGET_TYPE RUNTIME )
      # This is a bug in NSIS itself, so it's not CPack's fault
      set( CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.png" )
      set( CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.ico" )
      set( CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.ico" )

      # Check that Mantid exists. Warn if not installing over Mantid.
      set( CPACK_NSIS_EXTRA_INSTALL_COMMANDS "ClearErrors
           ReadEnvStr $R0 'MANTIDPATH'
           IfErrors 0 +2
             messagebox mb_ok 'Mantid does not appear to be installed.'
           ;Var /GLOBAL InstallBinPath
           ;StrCpy $InstallBinPath '$INSTDIR$\\bin'
           ;!if $InstallBinPath != $R0
             ;messagebox mb_ok 'You are installing VATES to $InstallBinPath. This is a different location from Mantid $R0. This is not recommended!'
        ;!endif
            ")
      set( CPACK_GENERATOR "NSIS" )
      message( STATUS "Windows-NSIS installer available for Mantid-Vates")
    endif()

    # Pickup all Vates projects
    set( CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR}/Vates" "Vates" "ALL" "/" )
    set( CPACK_PACKAGE_EXECUTABLES "VatesSimpleGui" "VatesSimpleGui" )

    # Set dependency on Mantid. Need to add ParaView
    set( CPACK_RPM_PACKAGE_REQUIRES "mantid" )
    set( CPACK_DEBIAN_PACKAGE_DEPENDS "mantid" )

    # run cpack configuration
    include( CPack )
  endif()
  
else( ParaView_FOUND AND USE_PARAVIEW )
  message( STATUS "Vates-Paraview plugins and widgets will not be built." ) 
endif( ParaView_FOUND AND USE_PARAVIEW )
