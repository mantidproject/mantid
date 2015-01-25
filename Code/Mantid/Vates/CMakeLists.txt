# Define the project name
project( Vates )

include( CommonVatesSetup )

# Select either building Paraview plugins
set( USE_PARAVIEW ON CACHE BOOL "Create paraview plugins. " )

# Paraview dependencies built if Paraview is found
find_package( ParaView )
if( ParaView_FOUND AND USE_PARAVIEW )
  message( STATUS "Using ParaView ${PARAVIEW_VERSION_FULL}" )
  include( ${PARAVIEW_USE_FILE} )
  # @todo: We should see if we can avoid linking directly to MDEvents/DataObjects
  include_directories( ${CMAKE_CURRENT_LIST_DIR}/../Framework/DataObjects/inc ${CMAKE_CURRENT_LIST_DIR}/../Framework/MDEvents/inc )
  add_subdirectory( VatesAPI )
  
  include_directories ( VatesAPI/inc )
  add_subdirectory( ParaviewPlugins )
  include_directories ( ${CMAKE_CURRENT_LIST_DIR}/../MantidQt/API/inc 
                        ${CMAKE_BINARY_DIR}/Code/Mantid/MantidQt/API
                        ${CMAKE_CURRENT_LIST_DIR}/../MantidQt/MantidWidgets/inc
                        ${CMAKE_BINARY_DIR}/Code/Mantid/MantidQt/MantidWidgets
                        ${CMAKE_CURRENT_LIST_DIR}/../MantidQt/Factory/inc 
                        ${CMAKE_CURRENT_LIST_DIR}/../MantidQt/SliceViewer/inc )
  add_subdirectory( VatesSimpleGui )
else( ParaView_FOUND AND USE_PARAVIEW )
  message( STATUS "Vates-Paraview plugins and widgets will not be built." ) 
endif( ParaView_FOUND AND USE_PARAVIEW )

