set(SRC_FILES
    src/Hdf5Version.cpp
    src/InstrumentBuilder.cpp
    src/NexusGeometryParser.cpp
    src/NexusGeometrySave.cpp
    src/NexusShapeFactory.cpp
    src/TubeBuilder.cpp
    src/TubeHelpers.cpp)

set(INC_FILES
    inc/MantidNexusGeometry/DllConfig.h
    inc/MantidNexusGeometry/Hdf5Version.h
    inc/MantidNexusGeometry/InstrumentBuilder.h
    inc/MantidNexusGeometry/NexusGeometryParser.h
    inc/MantidNexusGeometry/NexusGeometrySave.h
    inc/MantidNexusGeometry/NexusShapeFactory.h
    inc/MantidNexusGeometry/TubeBuilder.h
    inc/MantidNexusGeometry/TubeHelpers.h)

set(TEST_FILES
    InstrumentBuilderTest.h
    NexusGeometryParserTest.h
    NexusGeometrySaveTest.h
    NexusShapeFactoryTest.h
    TubeBuilderTest.h
    TubeHelpersTest.h)

if(COVERALLS)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add the target for this directory
add_library(NexusGeometry ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties(NexusGeometry
                      PROPERTIES OUTPUT_NAME
                                 MantidNexusGeometry
                                 COMPILE_DEFINITIONS
                                 IN_MANTID_NEXUS_GEOMETRY)

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(NexusGeometry
                        PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif()

# Include HDF5 libraries
target_include_directories(NexusGeometry SYSTEM PRIVATE ${HDF5_INCLUDE_DIRS})
# Add to the 'Framework' group in VS
set_property(TARGET NexusGeometry PROPERTY FOLDER "MantidFramework")

target_link_libraries(NexusGeometry
                      LINK_PRIVATE
                      ${MANTIDLIBS}
                      ${HDF5_LIBRARIES}
                      ${HDF5_HL_LIBRARIES}
                      ${HDF5_CXX_LIBRARIES})

# Add the unit tests directory
add_subdirectory(test)

mtd_install_targets(TARGETS
                    NexusGeometry
                    INSTALL_DIRS
                    ${LIB_DIR}
                    ${WORKBENCH_LIB_DIR})
