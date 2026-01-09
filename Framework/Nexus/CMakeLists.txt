set(SRC_FILES
    src/H5Util.cpp
    src/NexusClasses.cpp
    src/inverted_napi.cpp
    src/hdf5_type_helper.cpp
    src/NexusDescriptor.cpp
    src/NexusException.cpp
    src/NexusFile.cpp
    src/NexusAddress.cpp
    src/UniqueID.cpp
)

set(INC_FILES
    inc/MantidNexus/H5Util.h
    inc/MantidNexus/NexusClasses.h
    inc/MantidNexus/NexusIOHelper.h
    inc/MantidNexus/NexusFile_fwd.h
    inc/MantidNexus/inverted_napi.h
    inc/MantidNexus/hdf5_type_helper.h
    inc/MantidNexus/NexusDescriptor.h
    inc/MantidNexus/NexusException.h
    inc/MantidNexus/NexusFile.h
    inc/MantidNexus/NexusAddress.h
    inc/MantidNexus/UniqueID.h
)

set(TEST_FILES
    H5UtilTest.h
    NexusIOHelperTest.h
    NexusClassesTest.h
    NexusDescriptorTest.h
    NexusFileTest.h
    NexusFileLeakTest.h
    NexusFileNapiTest.h
    NexusFileReadWriteTest.h
    NexusAddressTest.h
    NapiUnitTest.h
    UniqueIDTest.h
)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add the target for this directory
add_library(Nexus ${SRC_FILES} ${INC_FILES})
add_library(Mantid::Nexus ALIAS Nexus)

# Use a precompiled header
target_precompile_headers(
  Nexus
  PRIVATE
  "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/inc/MantidNexus/NexusFile_fwd.h>"
  <climits>
  <map>
  <string>
  <vector>
)

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

target_link_libraries(
  Nexus
  PUBLIC Mantid::Types
  PRIVATE Poco::Foundation
)

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
