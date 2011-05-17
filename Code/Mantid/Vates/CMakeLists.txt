# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required ( VERSION 2.6 )
# Define the project name
project ( Vates )

# Check for VisIt
find_package ( VisIt )

include ( CommonVatesSetup )

#Select either building Paraview plugins or VisIT plugins.
set(USE_PARAVIEW ON CACHE BOOL "Create paraview plugins. ")

#Select either building Paraview plugins or VisIT plugins.
set(USE_VISIT OFF CACHE BOOL "Create visit plugins.")

if ( VISIT_FOUND AND USE_VISIT)

  add_subdirectory ( VatesAPI )
  
  # Perform common setup
  add_subdirectory ( VisitPlugins )
  add_subdirectory ( VisitDataBases )

else ( VISIT_FOUND AND USE_VISIT )
  message ( STATUS "Vates-VisIt plugins and widgets will not be built." )
endif ( VISIT_FOUND AND USE_VISIT )

# Paraview dependencies built if Paraview is found
find_package( ParaView QUIET )

if (ParaView_FOUND AND USE_PARAVIEW)

  add_subdirectory ( VatesAPI )
  add_subdirectory(ParaviewPlugins)
else (ParaView_FOUND AND USE_PARAVIEW )
  message( STATUS "Vates-Paraview plugins and widgets will not be built.") 
endif( ParaView_FOUND AND USE_PARAVIEW )



