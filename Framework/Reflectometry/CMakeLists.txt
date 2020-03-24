set(SRC_FILES
    src/ApplyFloodWorkspace.cpp
    src/CalculateSlits.cpp
    src/ConvertToReflectometryQ.cpp
    src/CreateFloodWorkspace.cpp
    src/CreatePolarizationEfficiencies.cpp
    src/CreatePolarizationEfficienciesBase.cpp
    src/CreateTransmissionWorkspace.cpp
    src/CreateTransmissionWorkspace2.cpp
    src/CreateTransmissionWorkspaceAuto.cpp
    src/CreateTransmissionWorkspaceAuto2.cpp
    src/FindReflectometryLines2.cpp
    src/JoinISISPolarizationEfficiencies.cpp
    src/LoadILLPolarizationFactors.cpp
    src/LoadILLReflectometry.cpp
    src/LoadISISPolarizationEfficiencies.cpp
    src/NRCalculateSlitResolution.cpp
    src/PolarizationCorrectionFredrikze.cpp
    src/PolarizationCorrectionWildes.cpp
    src/PolarizationEfficiencyCor.cpp
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
    src/ReflectometryWorkflowBase.cpp
    src/ReflectometryWorkflowBase2.cpp
    src/RefRoi.cpp
    src/SaveANSTOAscii.cpp
    src/SaveILLCosmosAscii.cpp
    src/SaveReflCustomAscii.cpp
    src/SaveReflThreeColumnAscii.cpp
    src/SaveReflectometryAscii.cpp
    src/SpecularReflectionAlgorithm.cpp
    src/SpecularReflectionCalculateTheta.cpp
    src/SpecularReflectionCalculateTheta2.cpp
    src/SpecularReflectionPositionCorrect.cpp
    src/SpecularReflectionPositionCorrect2.cpp)

set(INC_FILES
    inc/MantidReflectometry/ApplyFloodWorkspace.h
    inc/MantidReflectometry/CalculateSlits.h
    inc/MantidReflectometry/ConvertToReflectometryQ.h
    inc/MantidReflectometry/CreateFloodWorkspace.h
    inc/MantidReflectometry/CreatePolarizationEfficiencies.h
    inc/MantidReflectometry/CreatePolarizationEfficienciesBase.h
    inc/MantidReflectometry/CreateTransmissionWorkspace.h
    inc/MantidReflectometry/CreateTransmissionWorkspace2.h
    inc/MantidReflectometry/CreateTransmissionWorkspaceAuto.h
    inc/MantidReflectometry/CreateTransmissionWorkspaceAuto2.h
    inc/MantidReflectometry/FindReflectometryLines2.h
    inc/MantidReflectometry/JoinISISPolarizationEfficiencies.h
    inc/MantidReflectometry/LoadILLPolarizationFactors.h
    inc/MantidReflectometry/LoadILLReflectometry.h
    inc/MantidReflectometry/LoadISISPolarizationEfficiencies.h
    inc/MantidReflectometry/NRCalculateSlitResolution.h
    inc/MantidReflectometry/PolarizationCorrectionFredrikze.h
    inc/MantidReflectometry/PolarizationCorrectionWildes.h
    inc/MantidReflectometry/PolarizationEfficiencyCor.h
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
    inc/MantidReflectometry/ReflectometryWorkflowBase.h
    inc/MantidReflectometry/ReflectometryWorkflowBase2.h
    inc/MantidReflectometry/RefRoi.h
    inc/MantidReflectometry/SaveANSTOAscii.h
    inc/MantidReflectometry/SaveILLCosmosAscii.h
    inc/MantidReflectometry/SaveReflCustomAscii.h
    inc/MantidReflectometry/SaveReflThreeColumnAscii.h
    inc/MantidReflectometry/SaveReflectometryAscii.h
    inc/MantidReflectometry/SpecularReflectionAlgorithm.h
    inc/MantidReflectometry/SpecularReflectionCalculateTheta.h
    inc/MantidReflectometry/SpecularReflectionCalculateTheta2.h
    inc/MantidReflectometry/SpecularReflectionPositionCorrect.h
    inc/MantidReflectometry/SpecularReflectionPositionCorrect2.h)

set(TEST_FILES
    ApplyFloodWorkspaceTest.h
    CalculateSlitsTest.h
    ConvertToReflectometryQTest.h
    CreatePolarizationEfficienciesTest.h
    CreateTransmissionWorkspace2Test.h
    CreateTransmissionWorkspaceAuto2Test.h
    CreateTransmissionWorkspaceAutoTest.h
    CreateTransmissionWorkspaceTest.h
    FindReflectometryLines2Test.h
    JoinISISPolarizationEfficienciesTest.h
    LoadILLPolarizationFactorsTest.h
    LoadILLReflectometryTest.h
    LoadISISPolarizationEfficienciesTest.h
    NRCalculateSlitResolutionTest.h
    PolarizationCorrectionFredrikzeTest.h
    PolarizationCorrectionWildesTest.h
    PolarizationEfficiencyCorTest.h
    ReflectometryBeamStatisticsTest.h
    ReflectometryMomentumTransferTest.h
    ReflectometryReductionOne2Test.h
    ReflectometryReductionOneAuto2Test.h
    ReflectometryReductionOneAuto3Test.h
    ReflectometryTransformKiKfTest.h
    ReflectometryTransformPTest.h
    ReflectometryTransformQxQzTest.h
    ReflectometrySumInQTest.h
    SaveANSTOAsciiTest.h
    SaveILLCosmosAsciiTest.h
    SaveReflCustomAsciiTest.h
    SaveReflThreeColumnAsciiTest.h
    SaveReflectometryAsciiTest.h
    SpecularReflectionCalculateTheta2Test.h
    SpecularReflectionCalculateThetaTest.h
    SpecularReflectionPositionCorrect2Test.h
    SpecularReflectionPositionCorrectTest.h)

set(TEST_PY_FILES
    ReflectometryBackgroundSubtractionTest.py
    ReflectometryReductionOne2Test.py)

if(COVERALLS)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add a precompiled header where they are supported 
enable_precompiled_headers (inc/MantidReflectometry/PrecompiledHeader.h SRC_FILES )
# Add the target for this directory
add_library(Reflectometry ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties(Reflectometry
                      PROPERTIES OUTPUT_NAME
                                 MantidReflectometry
                                 COMPILE_DEFINITIONS
                                 "IN_MANTID_REFLECTOMETRY")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(Reflectometry
                        PROPERTIES INSTALL_RPATH
                                   "@loader_path/../Contents/MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Reflectometry PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET Reflectometry PROPERTY FOLDER "MantidFramework")

# Auto-generate exports header
generate_mantid_export_header(Reflectometry FALSE)

target_include_directories(Reflectometry PUBLIC
  inc ${CMAKE_BINARY_DIR}/Framework/Reflectometry ../Nexus/inc)

target_link_libraries(Reflectometry
                      PUBLIC
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${MANTIDLIBS}
                      Nexus
                      ${NEXUS_LIBRARIES}
                      ${GSL_LIBRARIES})

# Add the unit tests directory
add_subdirectory(test)

# Installation settings
mtd_install_targets(TARGETS
                    Reflectometry
                    INSTALL_DIRS
                    ${PLUGINS_DIR}
                    ${WORKBENCH_PLUGINS_DIR})
