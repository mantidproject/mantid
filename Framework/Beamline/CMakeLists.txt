set(SRC_FILES src/ComponentInfo.cpp src/DetectorInfo.cpp src/SpectrumInfo.cpp)

set(INC_FILES inc/MantidBeamline/ComponentInfo.h inc/MantidBeamline/ComponentType.h inc/MantidBeamline/DetectorInfo.h
              inc/MantidBeamline/SpectrumInfo.h
)

set(TEST_FILES ComponentInfoTest.h DetectorInfoTest.h SpectrumInfoTest.h)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add the target for this directory
add_library(Beamline ${SRC_FILES} ${INC_FILES})
add_library(Mantid::Beamline ALIAS Beamline)
# Set the name of the generated library
set_target_properties(Beamline PROPERTIES OUTPUT_NAME MantidBeamline COMPILE_DEFINITIONS IN_MANTID_BEAMLINE)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(Beamline PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Frameworks")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Beamline PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

target_include_directories(
  Beamline PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)
# Add to the 'Framework' group in VS
set_property(TARGET Beamline PROPERTY FOLDER "MantidFramework")

target_link_libraries(Beamline PUBLIC Eigen3::Eigen Mantid::Kernel)

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  Beamline PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/Beamline> $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(Beamline FALSE)

# Installation settings
set(TARGET_EXPORT_NAME "MantidBeamlineTargets")
mtd_install_framework_lib(TARGETS Beamline INSTALL_EXPORT_FILE EXPORT_NAME ${TARGET_EXPORT_NAME})
