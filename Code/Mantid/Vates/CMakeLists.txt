# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required ( VERSION 2.6 )
# Define the project name
project ( Vates )

# Check for VisIt
find_package ( VisIt )

include ( CommonVatesSetup )
add_subdirectory ( VatesAPI )

#Select either building Paraview plugins or VisIT plugins.
set(USE_PARAVIEW ON CACHE BOOL "Create paraview plugins. If OFF, creates VisIT plugins.")

if ( VISIT_FOUND AND (NOT USE_PARAVIEW))

  # Perform common setup
  add_subdirectory ( VisitPlugins )
  add_subdirectory ( VisitDataBases )

else ( VISIT_FOUND AND (NOT USE_PARAVIEW) )
  message ( STATUS "Vates-VisIt plugins and widgets will not be built." )
endif ( VISIT_FOUND AND (NOT USE_PARAVIEW) )

# Paraview dependencies built if Paraview is found
find_package( ParaView QUIET ) #TODO switch from QUIET to REQUIRED

if (ParaView_FOUND AND USE_PARAVIEW)
  add_subdirectory(ParaviewPlugins)
else (ParaView_FOUND AND USE_PARAVIEW )
  message( STATUS "Vates-Paraview plugins and widgets will not be built.") #TODO Nice message should be escalated
endif( ParaView_FOUND AND USE_PARAVIEW )



