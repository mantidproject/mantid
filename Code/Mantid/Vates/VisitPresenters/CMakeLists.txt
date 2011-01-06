project ( VisitPresenters )

set ( SRC_FILES 
src/MultiDimensionalDbPresenter.cpp
src/RebinningCutterPresenter.cpp
src/RebinningCutterXMLDefinitions.cpp
src/RebinningXMLGenerator.cpp
)

set ( INC_FILES 
inc/MantidVisitPresenters/MultiDimensionalDbPresenter.h
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

INCLUDE(GetMantidSubProject)
get_mantid_subproject(Framework/API)
get_mantid_subproject(Framework/Geometry)
get_mantid_subproject(Framework/Kernel)
get_mantid_subproject(Framework/MDAlgorithms)
get_mantid_subproject(Framework/MDDataObjects)

include_directories (
${MANTID_API_INCLUDE_DIR}
${MANTID_GEOMETRY_INCLUDE_DIR}
${MANTID_KERNEL_INCLUDE_DIR}
${MANTID_MDALGORITHMS_INCLUDE_DIR}
# TODO: MDDataObjects headers need to conform to Mantid naming convention
# So we can't use get_mantid_subproject stuff
${CMAKE_CURRENT_SOURCE_DIR}/../../Framework/MDDataObjects/inc 
)
include_directories (${VISIT_INCLUDE_DIR}/vtk/include/vtk-5.0)
include_directories (inc)

message(STATUS "P: ${MANTIDLIBS}")

# Handle Mantid subproject libraries
set ( MANTID_SUBPROJECT_LIBS 
Kernel
Geometry
API
MDDataObjects
MDAlgorithms
#${MANTID_KERNEL_LIBRARY} 
#${MANTID_GEOMETRY_LIBRARY} 
#${MANTID_API_LIBRARY} 
#${MANTID_MDALGORITHMS_LIBRARY} 
#${MANTID_MDDATAOBJECTS_LIBRARY}
)

target_link_libraries (VisitPresenters ${MANTID_SUBPROJECT_LIBS} ${VISIT_ROOT}/lib/libvtkCommon${CMAKE_SHARED_LIBRARY_SUFFIX} ${VISIT_ROOT}/lib/libvtkFiltering${CMAKE_SHARED_LIBRARY_SUFFIX}) #TODO: Not the correct way to access shared libraries
# Create test file projects
if ( CXXTEST_FOUND )
  cxxtest_add_test ( VisitPresentersTest ${TEST_FILES} )
  target_link_libraries( VisitPresentersTest VisitPresenters )
  add_dependencies ( FrameworkTests VisitPresentersTest )
endif ()
