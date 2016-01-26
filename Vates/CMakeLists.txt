# Define the project name
project( Vates )

include( CommonVatesSetup )

# Select either building Paraview plugins
set( USE_PARAVIEW ON CACHE BOOL "Create paraview plugins. " )

# Paraview dependencies built if Paraview is found
if( ParaView_FOUND AND USE_PARAVIEW )
  message( STATUS "Using ParaView ${PARAVIEW_VERSION_FULL}" )
  include( ${PARAVIEW_USE_FILE} )
  add_subdirectory( VatesAPI )
  add_subdirectory( ParaviewPlugins )
  add_subdirectory( VatesSimpleGui )
else( ParaView_FOUND AND USE_PARAVIEW )
  message( STATUS "Vates-Paraview plugins and widgets will not be built." ) 
endif( ParaView_FOUND AND USE_PARAVIEW )

