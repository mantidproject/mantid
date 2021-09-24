set(SRC_FILES src/MuonNexusReader.cpp src/NexusClasses.cpp src/NexusFileIO.cpp)

set(INC_FILES
    inc/MantidNexus/MuonNexusReader.h
    inc/MantidNexus/NexusClasses.h
    inc/MantidNexus/NexusFileIO.h
    inc/MantidNexus/NexusIOHelper.h)

set(TEST_FILES NexusIOHelperTest.h)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

add_definitions(-DIN_NEXUS_CPP_LIBRARY)

# Add the target for this directory
add_library(Nexus ${SRC_FILES} ${INC_FILES})
add_library(Mantid::Nexus ALIAS Nexus)

target_include_directories(Nexus PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
		$<INSTALL_INTERFACE:include/Mantid>
)
# Set the name of the generated library
set_target_properties(Nexus
                      PROPERTIES OUTPUT_NAME
                                 MantidNexus
                                 COMPILE_DEFINITIONS
                                 IN_MANTID_NEXUS)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(Nexus PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Frameworks")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Nexus PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET Nexus PROPERTY FOLDER "MantidFramework")

target_link_libraries(Nexus PUBLIC Mantid::API Mantid::DataObjects Mantid::Kernel Nexus::nexus Nexus::nexuscpp)

# Add the unit tests directory
add_subdirectory(test)

# Installation settings
if (CONDA_BUILD)
set(TARGET_EXPORT_NAME "MantidNexusTargets")
mtd_install_framework_lib(TARGETS Nexus EXPORT_NAME ${TARGET_EXPORT_NAME})
else()
mtd_install_targets(TARGETS Nexus INSTALL_DIRS ${LIB_DIR} ${WORKBENCH_LIB_DIR})
endif()
