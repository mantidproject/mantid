set(SRC_FILES
    src/napi.cpp
    src/napi4.cpp
    src/napi5.cpp
    src/NexusClasses.cpp
    src/NeXusException.cpp
    src/NeXusFile.cpp
    src/NeXusFileInfo.cpp
    src/stptok.cpp
)

set(INC_FILES
    inc/MantidLegacyNexus/NeXusFile_fwd.h
    inc/MantidLegacyNexus/napi.h
    inc/MantidLegacyNexus/napi4.h
    inc/MantidLegacyNexus/napi5.h
    inc/MantidLegacyNexus/napiconfig.h
    inc/MantidLegacyNexus/napi_internal.h
    inc/MantidLegacyNexus/NexusClasses.h
    inc/MantidLegacyNexus/NeXusException.hpp
    inc/MantidLegacyNexus/NeXusFile.hpp
    inc/MantidLegacyNexus/nx_stptok.h
    inc/MantidLegacyNexus/NeXusFileInfo.h
)

set(TEST_FILES NeXusFileTest.h NeXusFileLeakTest.h NeXusFileNapiTest.h NeXusFileReadWriteTest.h)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add the target for this directory
add_library(LegacyNexus ${SRC_FILES} ${INC_FILES})
add_library(Mantid::LegacyNexus ALIAS LegacyNexus)

target_include_directories(
  LegacyNexus PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

# Set the name of the generated library
set_target_properties(LegacyNexus PROPERTIES OUTPUT_NAME MantidLegacyNexus COMPILE_DEFINITIONS IN_MANTID_NEXUSCPP)
# H5_BUILT_AS_DYNAMIC_LIB required https://github.com/conda-forge/hdf5-feedstock/issues/58
target_compile_definitions(LegacyNexus PUBLIC -DH5_BUILT_AS_DYNAMIC_LIB)
target_link_libraries(LegacyNexus PUBLIC ${HDF4_DF_LIBRARY} ${HDF4_MFHDF_LIBRARY} ${HDF5_LIBRARIES})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(LegacyNexus PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Frameworks")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(LegacyNexus PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET LegacyNexus PROPERTY FOLDER "MantidFramework")

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  LegacyNexus PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/LegacyNexus> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(LegacyNexus FALSE)

# Installation settings
set(TARGET_EXPORT_NAME "MantidLegacyNexusTargets")
mtd_install_framework_lib(TARGETS LegacyNexus INSTALL_EXPORT_FILE EXPORT_NAME ${TARGET_EXPORT_NAME})
