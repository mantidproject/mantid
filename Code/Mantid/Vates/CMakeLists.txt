# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required ( VERSION 2.6 )
# Define the project name
project ( Vates )

include ( CommonVatesSetup )

#Select either building Paraview plugins or VisIT plugins.
set(USE_PARAVIEW ON CACHE BOOL "Create paraview plugins. ")

# Paraview dependencies built if Paraview is found
find_package( ParaView )
if (ParaView_FOUND AND USE_PARAVIEW)
  add_subdirectory ( VatesAPI )
  add_subdirectory(ParaviewPlugins)
  add_subdirectory( VatesSimpleGui )
else (ParaView_FOUND AND USE_PARAVIEW )
  message( STATUS "Vates-Paraview plugins and widgets will not be built.") 
endif( ParaView_FOUND AND USE_PARAVIEW )
