# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required ( VERSION 2.6 )

# Define the project name
project ( Vates )

# Check for VisIt
find_package ( VisIt )

if ( VISIT_FOUND )
  include_directories ( VisitPresenters/inc )
  add_subdirectory ( VisitPresenters )
  add_subdirectory ( VisitPlugins )
  add_subdirectory ( VisitDataBases )
else ( VISIT_FOUND )
  message ( STATUS "Vates-VisIt plugins and widgets will not be built." )
endif ( VISIT_FOUND )
