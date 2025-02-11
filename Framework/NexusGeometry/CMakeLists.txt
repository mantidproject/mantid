set(SRC_FILES
    src/Hdf5Version.cpp
    src/InstrumentBuilder.cpp
    src/JSONGeometryParser.cpp
    src/JSONInstrumentBuilder.cpp
    src/NexusGeometryParser.cpp
    src/NexusGeometrySave.cpp
    src/NexusGeometryUtilities.cpp
    src/NexusShapeFactory.cpp
    src/TubeBuilder.cpp
    src/TubeHelpers.cpp
)

set(INC_FILES
    inc/MantidNexusGeometry/AbstractLogger.h
    inc/MantidNexusGeometry/Hdf5Version.h
    inc/MantidNexusGeometry/InstrumentBuilder.h
    inc/MantidNexusGeometry/JSONGeometryParser.h
    inc/MantidNexusGeometry/JSONInstrumentBuilder.h
    inc/MantidNexusGeometry/NexusGeometryDefinitions.h
    inc/MantidNexusGeometry/NexusGeometryParser.h
    inc/MantidNexusGeometry/NexusGeometrySave.h
    inc/MantidNexusGeometry/NexusGeometryUtilities.h
    inc/MantidNexusGeometry/NexusShapeFactory.h
    inc/MantidNexusGeometry/TubeBuilder.h
    inc/MantidNexusGeometry/TubeHelpers.h
)

set(TEST_FILES
    InstrumentBuilderTest.h
    JSONInstrumentBuilderTest.h
    NexusGeometryParserTest.h
    NexusGeometrySaveTest.h
    JSONGeometryParserTest.h
    NexusShapeFactoryTest.h
    TubeBuilderTest.h
    TubeHelpersTest.h
)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add the target for this directory
add_library(NexusGeometry ${SRC_FILES} ${INC_FILES})
add_library(Mantid::NexusGeometry ALIAS NexusGeometry)

target_include_directories(
  NexusGeometry PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)
# Set the name of the generated library
set_target_properties(
  NexusGeometry PROPERTIES OUTPUT_NAME MantidNexusGeometry COMPILE_DEFINITIONS IN_MANTID_NEXUS_GEOMETRY
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(NexusGeometry PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Frameworks")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET NexusGeometry PROPERTY FOLDER "MantidFramework")

target_link_libraries(
  NexusGeometry
  PUBLIC Mantid::Geometry Mantid::API
  PRIVATE Mantid::Kernel Mantid::Json Mantid::Indexing
)

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  NexusGeometry PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/NexusGeometry>
                       $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(NexusGeometry TRUE)

# Installation settings
set(TARGET_EXPORT_NAME "MantidNexusGeometryTargets")
mtd_install_framework_lib(TARGETS NexusGeometry INSTALL_EXPORT_FILE EXPORT_NAME ${TARGET_EXPORT_NAME})
