set(SRC_FILES src/MuonNexusReader.cpp src/NexusClasses.cpp src/NexusFileIO.cpp)

set(INC_FILES
    inc/MantidNexus/MuonNexusReader.h
    inc/MantidNexus/NexusClasses.h
    inc/MantidNexus/NexusFileIO.h
    inc/MantidNexus/NexusIOHelper.h)

set(TEST_FILES NexusIOHelperTest.h)

if(COVERALLS)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

add_definitions(-DIN_NEXUS_CPP_LIBRARY)

# Add the target for this directory
add_library(Nexus ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties(Nexus
                      PROPERTIES OUTPUT_NAME
                                 MantidNexus
                                 COMPILE_DEFINITIONS
                                 IN_MANTID_NEXUS)

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(Nexus PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Nexus PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET Nexus PROPERTY FOLDER "MantidFramework")

include_directories(inc)

target_link_libraries(Nexus
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${MANTIDLIBS}
                      ${NEXUS_C_LIBRARIES}
                      ${NEXUS_LIBRARIES})

# Add the unit tests directory
add_subdirectory(test)

#
# Installation settings
#

mtd_install_targets(TARGETS Nexus INSTALL_DIRS ${LIB_DIR} ${WORKBENCH_LIB_DIR})
