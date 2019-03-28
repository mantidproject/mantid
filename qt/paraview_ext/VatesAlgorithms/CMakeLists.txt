# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required(VERSION 3.5)

project(VatesAlgorithms)

set(SRC_FILES
    src/LoadVTK.cpp
    src/SaveMDWorkspaceToVTK.cpp
    src/SaveMDWorkspaceToVTKImpl.cpp)

set(INC_FILES
    inc/MantidVatesAlgorithms/LoadVTK.h
    inc/MantidVatesAlgorithms/SaveMDWorkspaceToVTK.h
    inc/MantidVatesAlgorithms/SaveMDWorkspaceToVTKImpl.h)

set(TEST_FILES
    test/LoadVTKTest.h
    test/SaveMDWorkspaceToVTKTest.h
    test/SaveMDWorkspaceToVTKImplTest.h)

include_directories(inc ../VatesAPI/inc)
include_directories(SYSTEM ${PARAVIEW_INCLUDE_DIRS})

# For Windows:
add_definitions(-DIN_MANTID_VATESAPI)
# Add the target for this directory
add_library(VatesAlgorithms ${SRC_FILES} ${INC_FILES})
# target properties
set_target_properties(VatesAlgorithms
                      PROPERTIES OUTPUT_NAME
                                 MantidVatesAlgorithms
                                 FOLDER
                                 "MantidVates")

target_link_libraries(VatesAlgorithms
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${MANTID_SUBPROJECT_LIBS}
                      VatesAPI
                      vtkCommonCore
                      vtkCommonDataModel
                      ${vtkjsoncpp_LIBRARIES}
                      vtkIOLegacy
                      vtkIOXML
                      vtkInteractionStyle
                      vtkRenderingFreeType
                      vtkRenderingOpenGL2
                      ${POCO_LIBRARIES})

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(
    VatesAlgorithms
    PROPERTIES INSTALL_RPATH
               "@loader_path/../Contents/MacOS;@loader_path/../Libraries")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(
    VatesAlgorithms
    PROPERTIES
      INSTALL_RPATH
      "\$ORIGIN/../${LIB_DIR};\$ORIGIN/../${LIB_DIR}/paraview-${ParaView_VERSION_MAJOR}.${ParaView_VERSION_MINOR}"
    )
endif()

# Create test file projects
include_directories(SYSTEM
                    ${CXXTEST_INCLUDE_DIR}
                    ${GMOCK_INCLUDE_DIR}
                    ${GTEST_INCLUDE_DIR})

include_directories(inc
                    ../../../Framework/TestHelpers/inc
                    ../../../Framework/DataHandling/inc
                    ../../../Framework/DataObjects/inc
                    ../../../Framework/MDAlgorithms/inc)
set(TESTHELPER_SRCS
    ../../../Framework/TestHelpers/src/ComponentCreationHelper.cpp
    ../../../Framework/TestHelpers/src/InstrumentCreationHelper.cpp
    ../../../Framework/TestHelpers/src/WorkspaceCreationHelper.cpp
    ../../../Framework/TestHelpers/src/MDEventsTestHelper.cpp
    ../../../Framework/TestHelpers/src/StartFrameworkManager.cpp)
cxxtest_add_test(VatesAlgorithmsTest ${TEST_FILES})
target_link_libraries(VatesAlgorithmsTest
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${MANTID_SUBPROJECT_LIBS}
                      Indexing
                      VatesAPI
                      VatesAlgorithms
                      ${POCO_LIBRARIES}
                      ${GMOCK_LIBRARIES}
                      ${GTEST_LIBRARIES})
add_dependencies(AllTests VatesAlgorithmsTest)
# Add to the 'UnitTests' group in VS
set_property(TARGET VatesAlgorithmsTest PROPERTY FOLDER "UnitTests")

install(TARGETS VatesAlgorithms ${SYSTEM_PACKAGE_TARGET}
        DESTINATION ${PLUGINS_DIR})
