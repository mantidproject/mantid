
set ( SRC_FILES 
src/MultiDimensionalDbPresenter.cpp
src/RebinningCutterPresenter.cpp
src/RebinningCutterXMLDefinitions.cpp
src/RebinningXMLGenerator.cpp
)

set ( INC_FILES inc/MantidVisitPresenters/MultiDimensionalDbPresenter.h
inc/MantidVisitPresenters/RebinningCutterPresenter.h
inc/MantidVisitPresenters/RebinningCutterXMLDefinitions.h 
inc/MantidVisitPresenters/RebinningXMLGenerator.h
)

set (TEST_FILES
test/MultiDimensionalDbPresenterTest.h
test/RebinningCutterTest.h
test/RebinningXMLGeneratorTest.h
)

# For Windows:
add_definitions ( -DIN_MANTID_VISITPRESENTERS )
# Add the target for this directory
add_library ( VisitPresenters ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( VisitPresenters PROPERTIES OUTPUT_NAME MantidVisitPresenters )
# Add to the 'Framework' group in VS
set_property ( TARGET VisitPresenters PROPERTY FOLDER "Framework" )

include_directories (${CMAKE_CURRENT_SOURCE_DIR}../../Framework/Kernel/inc)        #TODO use get_mantid_subproject
include_directories (${CMAKE_CURRENT_SOURCE_DIR}../../Framework/API/inc)           #TODO use get_mantid_subproject
include_directories (${CMAKE_CURRENT_SOURCE_DIR}../../Framework/Geometry/inc)      #TODO use get_mantid_subproject
include_directories (${CMAKE_CURRENT_SOURCE_DIR}../../Framework/MDDataObjects/inc) #TODO use get_mantid_subproject
include_directories (${CMAKE_CURRENT_SOURCE_DIR}../../Framework/MDAlgorithms/inc)  #TODO use get_mantid_subproject
include_directories (${VISIT_INCLUDE_DIR}/vtk/include/vtk-5.0)  #TODO use get_mantid_subproject
include_directories (inc)
target_link_libraries (VisitPresenters ${MANTIDLIBS} ${VISIT_ROOT}/lib/libvtkCommon${CMAKE_SHARED_LIBRARY_SUFFIX} ${VISIT_ROOT}/lib/libvtkFiltering${CMAKE_SHARED_LIBRARY_SUFFIX}) #TODO: Not the correct way to access shared libraries
# Create test file projects
if ( CXXTEST_FOUND )
  cxxtest_add_test ( VisitPresentersTest ${TEST_FILES} )
  target_link_libraries( VisitPresentersTest VisitPresenters )
  add_dependencies ( FrameworkTests VisitPresentersTest )
endif ()
