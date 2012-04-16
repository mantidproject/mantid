# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required( VERSION 2.6 )
# Define the project name
project( Vates )



###########################################################################
# Add OpenMP flags here . TODO: Is this the right spot?
###########################################################################
if ( OPENMP_FOUND )
  #message (Hey OPENMP was found for VATES.)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}" )
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}" )
endif ()


include( CommonVatesSetup )

# Select either building Paraview plugins
set( USE_PARAVIEW ON CACHE BOOL "Create paraview plugins. " )

# Paraview dependencies built if Paraview is found
find_package( ParaView )
if( ParaView_FOUND AND USE_PARAVIEW )

  if( ENABLE_CPACK )
    message( STATUS "In Vates CPack" )
    set( CPACK_PACKAGE_NAME "mantid_vates${CPACK_PACKAGE_SUFFIX}" )
    include( CPackCommon )
    set( CPACK_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/CPackVatesConfig.cmake" )
	
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

    if( WIN32 )
      # Windows needs to have this for shared libraries install targets
      set( TARGET_TYPE RUNTIME)
      # This is a bug in NSIS itself, so it's not CPack's fault
      set( CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.png" )
      set( CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.ico" )
      set( CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/Images\\\\MantidPlot_Icon_32offset.ico" )

      #On install commands.
	  #1) Check that MantidPath exists.
	  #2) Append MantidPath/../plugins directory onto path, as it contains libs MDAlgorithms, etc.
	  #3) Append the installdir/bin directory onto path as it contains libs MantidVatesAPI etc (required for advanced Paraview plugin loading).
	  #4) Point to the location of the plubin libs. Paraview will then autoload these.
	  set (CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
	    ClearErrors 
	    ReadEnvStr $R0 'MANTIDPATH'
	    IfErrors 0 +2
	    messagebox mb_ok 'Mantid may not be installed. Please check that you have installed Mantid properly.'
		
		Push \\\"PATH\\\"
        Push \\\"A\\\"
        Push \\\"HKCU\\\"
        Push \\\"%MANTIDPATH%\\\\..\\\\plugins\\\"
        Call EnvVarUpdate
        Pop  \\\$0
		
		Push \\\"PATH\\\"
        Push \\\"A\\\"
        Push \\\"HKCU\\\"
        Push \\\"$INSTDIR\\\\plugins\\\"
        Call EnvVarUpdate
        Pop  \\\$0
		
		Push \\\"PATH\\\"
        Push \\\"A\\\"
        Push \\\"HKCU\\\"
        Push \\\"$INSTDIR\\\\bin\\\"
        Call EnvVarUpdate
        Pop  \\\$0
		
		Push \\\"PV_PLUGIN_PATH\\\"
        Push \\\"A\\\"
        Push \\\"HKCU\\\"
        Push \\\"$INSTDIR\\\\${PVPLUGINS_DIR}\\\"
        Call EnvVarUpdate
        Pop  \\\$0
		")
	
	  # On unistall reverse stages listed above.
	  set (CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Push \\\"PATH\\\"
        Push \\\"R\\\"
        Push \\\"HKCU\\\"
        Push \\\"%MANTIDPATH%\\\\..\\\\plugins\\\"
        Call un.EnvVarUpdate
        Pop  \\\$0
		
		Push \\\"PATH\\\"
        Push \\\"R\\\"
        Push \\\"HKCU\\\"
        Push \\\"$INSTDIR\\\\plugins\\\"
        Call un.EnvVarUpdate
        Pop  \\\$0
		
		Push \\\"PATH\\\"
        Push \\\"R\\\"
        Push \\\"HKCU\\\"
        Push \\\"$INSTDIR\\\\bin\\\"
        Call un.EnvVarUpdate
        Pop  \\\$0
		
		Push \\\"PV_PLUGIN_PATH\\\"
        Push \\\"R\\\"
        Push \\\"HKCU\\\"
        Push \\\"$INSTDIR\\\\${PVPLUGINS_DIR}\\\"
        Call un.EnvVarUpdate
        Pop  \\\$0
		")
		
      set( CPACK_GENERATOR "NSIS" )
      message( STATUS "Windows-NSIS installer available for Mantid-Vates")
    endif()

    # Pickup all Vates projects
    set( CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR}/Vates" "Vates" "ALL" "/" )
    set( CPACK_PACKAGE_EXECUTABLES "VatesSimpleGui" "VatesSimpleGui" )

    # Set dependency on Mantid. Need to add ParaView
    set( CPACK_RPM_PACKAGE_REQUIRES "mantid${CPACK_PACKAGE_SUFFIX} = ${CPACK_PACKAGE_VERSION}" )
    set( CPACK_DEBIAN_PACKAGE_DEPENDS "mantid${CPACK_PACKAGE_SUFFIX} (=${CPACK_PACKAGE_VERSION})" )

    # Create target for Vates package creation
    add_custom_target( package-vates COMMAND ${CMAKE_CPACK_COMMAND} --config ${CPACK_OUTPUT_CONFIG_FILE} )

    # run cpack configuration
    include( CPack )
  endif()
  
  include( ${PARAVIEW_USE_FILE} )
  add_subdirectory( VatesAPI )
  add_subdirectory( ParaviewPlugins )
  add_subdirectory( VatesSimpleGui )
else( ParaView_FOUND AND USE_PARAVIEW )
  message( STATUS "Vates-Paraview plugins and widgets will not be built." ) 
endif( ParaView_FOUND AND USE_PARAVIEW )

