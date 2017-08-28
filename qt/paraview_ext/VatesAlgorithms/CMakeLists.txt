#This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required( VERSION 3.5 )

project( VatesAlgorithms )

set( SRC_FILES
src/LoadVTK.cpp
src/SaveMDWorkspaceToVTK.cpp
src/SaveMDWorkspaceToVTKImpl.cpp
)

set( INC_FILES
inc/MantidVatesAlgorithms/LoadVTK.h
inc/MantidVatesAlgorithms/SaveMDWorkspaceToVTK.h
inc/MantidVatesAlgorithms/SaveMDWorkspaceToVTKImpl.h
)

set( TEST_FILES
test/LoadVTKTest.h
test/SaveMDWorkspaceToVTKTest.h
test/SaveMDWorkspaceToVTKImplTest.h
)

include_directories( inc ../VatesAPI/inc )
include_directories ( SYSTEM ${QWT_INCLUDE_DIR} ${PARAVIEW_INCLUDE_DIRS})

set( SRC_UNITY_IGNORE_FILES )
if( UNITY_BUILD )
  include( UnityBuild )
  enable_unity_build( VatesAlgorithms SRC_FILES SRC_UNITY_IGNORE_FILES 10 )
endif( UNITY_BUILD )

# For Windows:
add_definitions( -DIN_MANTID_VATESAPI )
# Add the target for this directory
add_library( VatesAlgorithms ${SRC_FILES} ${INC_FILES} )
# Set the name of the generated library
set_target_properties( VatesAlgorithms PROPERTIES OUTPUT_NAME MantidVatesAlgorithms )
# Add to the 'Framework' group in VS
set_property( TARGET VatesAlgorithms PROPERTY FOLDER "MantidVates" )

target_link_libraries( VatesAlgorithms LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
${MANTID_SUBPROJECT_LIBS}
VatesAPI
vtkCommonCore
vtkCommonDataModel
vtkIOLegacy
vtkFiltersExtraction
vtkFiltersSources
${vtkjsoncpp_LIBRARIES}
vtkPVVTKExtensionsDefault
${QWT_LIBRARIES}
Qt4::QtCore
${POCO_LIBRARIES}
${Boost_LIBRARIES}
${NEXUS_LIBRARIES}
)

if ( MSVC )
  # To simplify developer builds from the IDE we copy the ParaView DLLs to the
  # appropriate buildbinary folder. This avoids have to set the PATH to point to
  # a different ParaView build directory for each configuration
  set ( _src_dir ${ParaView_DIR}/bin/${CMAKE_CFG_INTDIR} )
  set ( _dest_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} )
  add_custom_command (
    TARGET VatesAlgorithms POST_BUILD
    COMMAND xcopy ARGS \"${_src_dir}/*.dll\" \"${_dest_dir}\" /D /Y /F
    COMMENT "Updating ParaView DLLs in bin/${CMAKE_CFG_INTDIR}"
  )
  unset ( _dest_dir )
  unset ( _src_dir )
endif()


if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( VatesAlgorithms PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Libraries")
endif ()

# Create test file projects
include_directories ( SYSTEM ${CXXTEST_INCLUDE_DIR} ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} )

include_directories( inc ../../../Framework/TestHelpers/inc ../../../Framework/DataHandling/inc ../../../Framework/DataObjects/inc ../../../Framework/MDAlgorithms/inc)
set ( TESTHELPER_SRCS ../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
                      ../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
                      ../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
                      ../../../Framework/TestHelpers/src/MDEventsTestHelper.cpp
                      ../../../Framework/TestHelpers/src/StartFrameworkManager.cpp )
cxxtest_add_test( VatesAlgorithmsTest ${TEST_FILES} )
target_link_libraries( VatesAlgorithmsTest LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
  ${MANTID_SUBPROJECT_LIBS}
  VatesAlgorithms
  DataHandling
  Kernel
  DataObjects
  vtkCommonCore
  vtkCommonDataModel
  vtkIOLegacy
  vtkFiltersExtraction
  vtkFiltersSources
  vtkPVVTKExtensionsDefault
  ${vtkjsoncpp_LIBRARIES}
  ${POCO_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GMOCK_LIBRARIES}
  ${GTEST_LIBRARIES}
  ${QWT_LIBRARIES}
  Qt4::QtCore
  )
add_dependencies( AllTests VatesAlgorithmsTest )
# Add to the 'UnitTests' group in VS
set_property ( TARGET VatesAlgorithmsTest PROPERTY FOLDER "UnitTests" )

install( TARGETS VatesAlgorithms ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PLUGINS_DIR} )
