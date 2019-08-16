# testhelpers Python module

# Copying no longer happens but old builds have this
# Remove the directory if it exists
# TODO: Remove in 4.2 maintenance
if(EXISTS "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/testhelpers")
  file(REMOVE_RECURSE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/testhelpers")
endif()

# Create the targets for this directory
set(FRAMEWORK_DIR ../../..)
include_directories(${FRAMEWORK_DIR}/CurveFitting/inc
                    ${FRAMEWORK_DIR}/DataHandling/inc
                    ${FRAMEWORK_DIR}/DataObjects/inc
                    ${FRAMEWORK_DIR}/Nexus/inc
                    ${FRAMEWORK_DIR}/TestHelpers/inc)

# WorkspaceCreationHelper
set(SRC_FILES
    WorkspaceCreationHelper/WorkspaceCreationHelperModule.cpp
    ${FRAMEWORK_DIR}/TestHelpers/src/ComponentCreationHelper.cpp
    ${FRAMEWORK_DIR}/TestHelpers/src/MDEventsTestHelper.cpp
    ${FRAMEWORK_DIR}/TestHelpers/src/InstrumentCreationHelper.cpp
    ${FRAMEWORK_DIR}/TestHelpers/src/WorkspaceCreationHelper.cpp)
add_library(PythonWorkspaceCreationHelper
            ${SRC_FILES}
            ${INC_FILES})
set_python_properties(PythonWorkspaceCreationHelper _WorkspaceCreationHelper)

# Override folder
set_property(TARGET PythonWorkspaceCreationHelper
             PROPERTY FOLDER "UnitTests/Python")
target_link_libraries(PythonWorkspaceCreationHelper
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      PythonAPIModule
                      DataObjects
                      DataHandling
                      API
                      HistogramData
                      Indexing
                      Types
                      Kernel
                      Geometry
                      ${PYTHON_LIBRARIES}
                      ${Boost_LIBRARIES}
                      ${POCO_LIBRARIES})

# Overall testhelpers target
# .pth file for discovery (only needed for development)
set(_testhelpers_pth_src "${CMAKE_CURRENT_BINARY_DIR}/testhelpers.pth")
set(_testhelpers_pth_dest
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/testhelpers.pth")
file(WRITE ${_testhelpers_pth_src} "${CMAKE_CURRENT_LIST_DIR}/..\n")
add_custom_command(OUTPUT ${_testhelpers_pth_dest}
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           ${_testhelpers_pth_src} ${_testhelpers_pth_dest}
                   DEPENDS ${_testhelpers_pth_src}
                   COMMENT "Generating testhelpers .pth file")
add_custom_target(testhelpers DEPENDS PythonWorkspaceCreationHelper ${_testhelpers_pth_dest})
add_dependencies(PythonInterface ScriptsDotPth)

add_dependencies(FrameworkTests testhelpers)
set_property(TARGET testhelpers PROPERTY FOLDER "UnitTests/Python")
