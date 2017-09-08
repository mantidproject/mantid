set ( SRC_FILES
    src/InstrumentHandler.cpp
    src/NexusGeometryParser.cpp
    src/ParsingErrors.cpp
        )

set ( INC_FILES
    inc/MantidNexusGeometry/InstrumentHandler.h
    inc/MantidNexusGeometry/NexusGeometryParser.h
    inc/MantidNexusGeometry/ParsingErrors.h
        )

set ( TEST_FILES
        )

# Add the target for this directory
add_library ( NexusGeometry ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties ( NexusGeometry PROPERTIES OUTPUT_NAME MantidNexusGeometry
        COMPILE_DEFINITIONS IN_MANTID_NEXUS_GEOMETRY )

if (OSX_VERSION VERSION_GREATER 10.8)
    set_target_properties ( NexusGeometry PROPERTIES INSTALL_RPATH "@loader_path/../MacOS" )
endif ()

#Include HDF5 libraries
include_directories (${HDF5_INCLUDE_DIRS})

# Add to the 'Framework' group in VS
set_property ( TARGET NexusGeometry PROPERTY FOLDER "MantidFramework" )

include_directories ( inc )
