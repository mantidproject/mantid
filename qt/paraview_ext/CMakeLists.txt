# Define the project name
project( Vates )

include( CommonVatesSetup )

# Select either building Paraview plugins
set( USE_PARAVIEW ON CACHE BOOL "Create paraview plugins. " )

# Paraview dependencies built if Paraview is found
if( ParaView_FOUND AND USE_PARAVIEW )
  message( STATUS "Using ParaView ${PARAVIEW_VERSION_FULL}" )
  include( ${PARAVIEW_USE_FILE} )

  function (set_pvplugin_properties target QT_VERSION vers)
    set_target_properties ( ${target} PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY ${PLUGINS_LIBRARY_OUTPUT_DIRECTORY}/${PVPLUGINS_SUBDIR}/qt${vers}
      RUNTIME_OUTPUT_DIRECTORY ${PLUGINS_LIBRARY_OUTPUT_DIRECTORY}/${PVPLUGINS_SUBDIR}/qt${vers}
      FOLDER "MantidVatesParaViewPlugins"
    )
  endfunction()

  function (install_pvplugin target QT_VERSION vers)
    install( TARGETS ${target} ${SYSTEM_PACKAGE_TARGET}
      DESTINATION ${PLUGINS_DIR}/${PVPLUGINS_SUBDIR}/qt${vers}
    )
  endfunction()

  add_subdirectory( VatesAPI )
  add_subdirectory( VatesAlgorithms )
  add_subdirectory( PVPlugins )
  add_subdirectory( VatesSimpleGui )
else( ParaView_FOUND AND USE_PARAVIEW )
  message( STATUS "Vates-Paraview plugins and widgets will not be built." )
endif( ParaView_FOUND AND USE_PARAVIEW )
