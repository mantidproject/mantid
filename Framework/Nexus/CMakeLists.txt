set(SRC_FILES
    src/H5Util.cpp
    src/NexusClasses.cpp
    src/napi.cpp
    src/napi5.cpp
    src/NeXusException.cpp
    src/NeXusFile.cpp
    src/nxstack.cpp
    src/stptok.cpp
)

set(INC_FILES
    inc/MantidNexus/H5Util.h
    inc/MantidNexus/NexusClasses.h
    inc/MantidNexus/NexusIOHelper.h
    inc/MantidNexus/NeXusFile_fwd.h
    inc/MantidNexus/napi.h
    inc/MantidNexus/napi5.h
    inc/MantidNexus/napiconfig.h
    inc/MantidNexus/napi_internal.h
    inc/MantidNexus/NeXusException.hpp
    inc/MantidNexus/NeXusFile.hpp
    inc/MantidNexus/nx_stptok.h
    inc/MantidNexus/nxstack.h
)

set(TEST_FILES H5UtilTest.h NexusIOHelperTest.h NeXusFileTest.h NeXusFileLeakTest.h NeXusFileNapiTest.h
               NeXusFileReadWriteTest.h
)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add the target for this directory
add_library(Nexus ${SRC_FILES} ${INC_FILES})
add_library(Mantid::Nexus ALIAS Nexus)

target_include_directories(
  Nexus PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

# Set the name of the generated library
set_target_properties(Nexus PROPERTIES OUTPUT_NAME MantidNexus COMPILE_DEFINITIONS IN_MANTID_NEXUS)

# H5_BUILT_AS_DYNAMIC_LIB required https://github.com/conda-forge/hdf5-feedstock/issues/58
target_compile_definitions(Nexus PUBLIC -DH5_BUILT_AS_DYNAMIC_LIB)
target_link_libraries(Nexus PRIVATE ${HDF5_LIBRARIES})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(Nexus PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Frameworks")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Nexus PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET Nexus PROPERTY FOLDER "MantidFramework")

target_link_libraries(Nexus PRIVATE Poco::Foundation)

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  Nexus PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/Nexus> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(Nexus FALSE)

# Installation settings
set(TARGET_EXPORT_NAME "MantidNexusTargets")
mtd_install_framework_lib(TARGETS Nexus INSTALL_EXPORT_FILE EXPORT_NAME ${TARGET_EXPORT_NAME})
