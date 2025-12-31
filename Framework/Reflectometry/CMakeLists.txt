set(SRC_FILES
    src/CalculateSlits.cpp
    src/ConvertToReflectometryQ.cpp
    src/ConvertToReflectometryQ2.cpp
    src/CreateTransmissionWorkspace2.cpp
    src/CreateTransmissionWorkspaceAuto2.cpp
    src/FindReflectometryLines2.cpp
    src/NRCalculateSlitResolution.cpp
    src/ReflectometryBackgroundSubtraction.cpp
    src/ReflectometryBeamStatistics.cpp
    src/ReflectometryMomentumTransfer.cpp
    src/ReflectometryReductionOne2.cpp
    src/ReflectometryReductionOneAuto2.cpp
    src/ReflectometryReductionOneAuto3.cpp
    src/ReflectometryTransformKiKf.cpp
    src/ReflectometryTransformP.cpp
    src/ReflectometryTransformQxQz.cpp
    src/ReflectometrySumInQ.cpp
    src/ReflectometryWorkflowBase2.cpp
    src/RefRoi.cpp
    src/SpecularReflectionAlgorithm.cpp
    src/SpecularReflectionCalculateTheta.cpp
    src/SpecularReflectionCalculateTheta2.cpp
    src/SpecularReflectionPositionCorrect.cpp
    src/SpecularReflectionPositionCorrect2.cpp
)

set(INC_FILES
    inc/MantidReflectometry/CalculateSlits.h
    inc/MantidReflectometry/ConvertToReflectometryQ.h
    inc/MantidReflectometry/ConvertToReflectometryQ2.h
    inc/MantidReflectometry/CreateTransmissionWorkspace2.h
    inc/MantidReflectometry/CreateTransmissionWorkspaceAuto2.h
    inc/MantidReflectometry/FindReflectometryLines2.h
    inc/MantidReflectometry/NRCalculateSlitResolution.h
    inc/MantidReflectometry/ReflectometryBackgroundSubtraction.h
    inc/MantidReflectometry/ReflectometryBeamStatistics.h
    inc/MantidReflectometry/ReflectometryMomentumTransfer.h
    inc/MantidReflectometry/ReflectometryReductionOne2.h
    inc/MantidReflectometry/ReflectometryReductionOneAuto2.h
    inc/MantidReflectometry/ReflectometryReductionOneAuto3.h
    inc/MantidReflectometry/ReflectometryTransformKiKf.h
    inc/MantidReflectometry/ReflectometryTransformP.h
    inc/MantidReflectometry/ReflectometryTransformQxQz.h
    inc/MantidReflectometry/ReflectometrySumInQ.h
    inc/MantidReflectometry/ReflectometryWorkflowBase2.h
    inc/MantidReflectometry/RefRoi.h
    inc/MantidReflectometry/SpecularReflectionAlgorithm.h
    inc/MantidReflectometry/SpecularReflectionCalculateTheta.h
    inc/MantidReflectometry/SpecularReflectionCalculateTheta2.h
    inc/MantidReflectometry/SpecularReflectionPositionCorrect.h
    inc/MantidReflectometry/SpecularReflectionPositionCorrect2.h
)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND PROPERTY COVERAGE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add the target for this directory
add_library(Reflectometry ${SRC_FILES} ${INC_FILES})
add_library(Mantid::Reflectometry ALIAS Reflectometry)

# Add a precompiled header
target_precompile_headers(
  Reflectometry
  PRIVATE "MantidAPI/Algorithm.h" "MantidKernel/Exception.h" "MantidReflectometry/DllConfig.h"
)

target_include_directories(
  Reflectometry PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

# Set the name of the generated library
set_target_properties(
  Reflectometry PROPERTIES OUTPUT_NAME MantidReflectometry COMPILE_DEFINITIONS "IN_MANTID_REFLECTOMETRY"
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(Reflectometry PROPERTIES INSTALL_RPATH "@loader_path/../Contents/MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Reflectometry PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET Reflectometry PROPERTY FOLDER "MantidFramework")

target_link_libraries(
  Reflectometry
  PUBLIC Mantid::Kernel Mantid::API Mantid::Geometry Mantid::DataObjects
  PRIVATE Mantid::Indexing Mantid::HistogramData
)

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(
  Reflectometry PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/Framework/Reflectometry>
                       $<INSTALL_INTERFACE:include/Mantid>
)
generate_mantid_export_header(Reflectometry FALSE)

# Installation settings
mtd_install_framework_lib(TARGETS Reflectometry PLUGIN_LIB)
