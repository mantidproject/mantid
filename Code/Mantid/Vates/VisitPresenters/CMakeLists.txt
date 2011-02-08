# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required ( VERSION 2.6 )
include(${VISIT_INCLUDE_DIR}/PluginVsInstall.cmake)
include(${VISIT_INCLUDE_DIR}/VisItLibraryDependencies.cmake)

project ( VisitPresenters )

set ( SRC_FILES 
src/Clipper.cpp
src/MultiDimensionalDbPresenter.cpp
src/RebinningCutterPresenter.cpp
src/RebinningCutterXMLDefinitions.cpp
src/RebinningXMLGenerator.cpp
src/vtkDataSetFactory.cpp
src/vtkClipperDataSetFactory.cpp
src/vtkProxyFactory.cpp
src/vtkRectilinearGridFactory.cpp
src/vtkStructuredGridFactory.cpp
src/vtkThresholdingUnstructuredGridFactory.cpp
)

set ( INC_FILES 
inc/MantidVisitPresenters/Clipper.h
inc/MantidVisitPresenters/MultiDimensionalDbPresenter.h
inc/MantidVisitPresenters/RebinningCutterPresenter.h
inc/MantidVisitPresenters/RebinningCutterXMLDefinitions.h 
inc/MantidVisitPresenters/RebinningXMLGenerator.h
inc/MantidVisitPresenters/vtkDataSetFactory.h
inc/MantidVisitPresenters/vtkClipperDataSetFactory.h
inc/MantidVisitPresenters/vtkProxyFactory.h
inc/MantidVisitPresenters/vtkRectilinearGridFactory.h
inc/MantidVisitPresenters/vtkStructuredGridFactory.h
inc/MantidVisitPresenters/vtkThresholdingUnstructuredGridFactory.h
)

set (TEST_FILES
test/vtkClipperDataSetFactoryTest.h
test/vtkProxyFactoryTest.h
test/vtkRectilinearGridFactoryTest.h
test/vtkStructuredGridFactoryTest.h
test/vtkThresholdingUnstructuredGridFactoryTest.h
test/MultiDimensionalDbPresenterTest.h
test/RebinningCutterTest.h
test/RebinningXMLGeneratorTest.h
)

include_directories (${VTK_INCLUDE_DIRS})
include_directories (inc)

link_directories ( ${VTK_LIBRARY_DIRS} )

# For Windows:
add_definitions ( -DIN_MANTID_VISITPRESENTERS )
# Add the target for this directory
add_library ( VisitPresenters ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( VisitPresenters PROPERTIES OUTPUT_NAME MantidVisitPresenters )
# Add to the 'Framework' group in VS
set_property ( TARGET VisitPresenters PROPERTY FOLDER "Framework" )

target_link_libraries (VisitPresenters 
${MANTID_SUBPROJECT_LIBS} 
vtkCommon 
vtkFiltering
)

# Create test file projects
if ( CXXTEST_FOUND AND GMOCK_FOUND AND GTEST_FOUND )
  cxxtest_add_test ( VisitPresentersTest ${TEST_FILES} )
  target_link_libraries( VisitPresentersTest VisitPresenters ${GMOCK_LIBRARIES} )
  add_dependencies ( FrameworkTests VisitPresentersTest )
endif ()
