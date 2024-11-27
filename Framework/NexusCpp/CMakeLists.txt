include(GenerateExportHeader)

set(SRC_FILES
    src/napi.c
    src/napi4.c
    src/napi5.c
    src/NeXusException.cpp
    src/NeXusFile.cpp
    src/NeXusStream.cpp
    src/nxdataset.c # TODO REMOVE xml
    src/nxio.c # TODO REMOVE xml
    src/nxstack.c
    src/nxxml.c # TODO REMOVE xml
    src/stptok.c
)

set(INC_FILES
    inc/MantidNexusCpp/napi.h
    inc/MantidNexusCpp/napi4.h
    inc/MantidNexusCpp/napi5.h
    inc/MantidNexusCpp/napiconfig.h
    inc/MantidNexusCpp/napi_internal.h
    inc/MantidNexusCpp/NeXusException.hpp
    inc/MantidNexusCpp/NeXusFile.hpp
    inc/MantidNexusCpp/NeXusStream.hpp
    inc/MantidNexusCpp/nx_stptok.h
    inc/MantidNexusCpp/nxconfig.h
    inc/MantidNexusCpp/nxdataset.h # TODO REMOVE xml
    inc/MantidNexusCpp/nxio.h # TODO REMOVE xml
    inc/MantidNexusCpp/nxstack.h
    inc/MantidNexusCpp/nxxml.h # TODO REMOVE xml
)

# TODO should ahve tests set(TEST_FILES NeXusFileTest.h)

# Add the target for this directory
add_library(NexusCpp ${SRC_FILES} ${INC_FILES})
add_library(Mantid::NexusCpp ALIAS NexusCpp)

target_include_directories(
  NexusCpp PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

# Set the name of the generated library
set_target_properties(NexusCpp PROPERTIES OUTPUT_NAME MantidNexusCpp COMPILE_DEFINITIONS IN_MANTID_NEXUS)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(NexusCpp PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Frameworks")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(NexusCpp PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET NexusCpp PROPERTY FOLDER "MantidFramework")

# TODO remove this line? target_link_libraries(NexusCpp PUBLIC Mantid::API Mantid::DataObjects Mantid::Kernel
# Mantid::NexusCpp)

# TODO Add the unit tests directory add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  NexusCpp PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/NexusCpp> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(NexusCpp FALSE)

# Installation settings
set(TARGET_EXPORT_NAME "MantidNexusCppTargets")
mtd_install_framework_lib(TARGETS NexusCpp INSTALL_EXPORT_FILE EXPORT_NAME ${TARGET_EXPORT_NAME})
