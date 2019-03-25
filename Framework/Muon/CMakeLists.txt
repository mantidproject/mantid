set(SRC_FILES
    src/AlphaCalc.cpp
    src/ApplyDeadTimeCorr.cpp
    src/ApplyMuonDetectorGrouping.cpp
    src/ApplyMuonDetectorGroupPairing.cpp
    src/AsymmetryCalc.cpp
    src/CalculateMuonAsymmetry.cpp
    src/CalMuonDeadTime.cpp
    src/CalMuonDetectorPhases.cpp
    src/ConvertFitFunctionForMuonTFAsymmetry.cpp
    src/LoadAndApplyMuonDetectorGrouping.cpp
    src/EstimateMuonAsymmetryFromCounts.cpp
    src/MuonAlgorithmHelper.cpp
    src/MuonAsymmetryHelper.cpp
    src/MuonGroupDetectors.cpp
    src/MuonGroupingCounts.cpp
    src/MuonGroupingAsymmetry.cpp
    src/MuonPreProcess.cpp
    src/MuonPairingAsymmetry.cpp
    src/PhaseQuadMuon.cpp
    src/PlotAsymmetryByLogValue.cpp
    src/RemoveExpDecay.cpp
    src/RRFMuon.cpp)

set(INC_FILES
    inc/MantidMuon/AlphaCalc.h
    inc/MantidMuon/ApplyDeadTimeCorr.h
    inc/MantidMuon/ApplyMuonDetectorGrouping.h
    inc/MantidMuon/ApplyMuonDetectorGroupPairing.h
    inc/MantidMuon/AsymmetryCalc.h
    inc/MantidMuon/CalculateMuonAsymmetry.h
    inc/MantidMuon/CalMuonDeadTime.h
    inc/MantidMuon/CalMuonDetectorPhases.h
    inc/MantidMuon/ConvertFitFunctionForMuonTFAsymmetry.h
    inc/MantidMuon/LoadAndApplyMuonDetectorGrouping.h
    inc/MantidMuon/EstimateMuonAsymmetryFromCounts.h
    inc/MantidMuon/MuonAlgorithmHelper.h
    inc/MantidMuon/MuonAsymmetryHelper.h
    inc/MantidMuon/MuonGroupDetectors.h
    inc/MantidMuon/MuonGroupingCounts.h
    inc/MantidMuon/MuonPairingAsymmetry.h
    inc/MantidMuon/MuonGroupingAsymmetry.h
    inc/MantidMuon/MuonPreProcess.h
    inc/MantidMuon/PhaseQuadMuon.h
    inc/MantidMuon/PlotAsymmetryByLogValue.h
    inc/MantidMuon/RemoveExpDecay.h
    inc/MantidMuon/RRFMuon.h)

set(TEST_FILES
    AlphaCalcTest.h
    ApplyDeadTimeCorrTest.h
    ApplyMuonDetectorGroupingTest.h
    ApplyMuonDetectorGroupPairingTest.h
    AsymmetryCalcTest.h
    CalculateMuonAsymmetryTest.h
    CalMuonDeadTimeTest.h
    CalMuonDetectorPhasesTest.h
    ConvertFitFunctionForMuonTFAsymmetryTest.h
    LoadAndApplyMuonDetectorGroupingTest.h
    MuonAlgorithmHelperTest.h
    EstimateMuonAsymmetryFromCountsTest.h
    MuonGroupDetectorsTest.h
    MuonPairingAsymmetryTest.h
    MuonGroupingCountsTest.h
    MuonGroupingAsymmetryTest.h
    MuonPreProcessTest.h
    PhaseQuadMuonTest.h
    PlotAsymmetryByLogValueTest.h
    RemoveExpDecayTest.h
    RRFMuonTest.h)

if(COVERALLS)
  foreach(loop_var ${SRC_FILES} ${C_SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

# Add a precompiled header where they are supported enable_precompiled_headers (
# inc/MantidAlgorithms/PrecompiledHeader.h SRC_FILES ) Add the target for this
# directory
add_library(Muon ${SRC_FILES} ${C_SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties(Muon
                      PROPERTIES OUTPUT_NAME
                                 MantidMuon
                                 COMPILE_DEFINITIONS
                                 "IN_MANTID_MUON")

if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(Muon
                        PROPERTIES INSTALL_RPATH
                                   "@loader_path/../Contents/MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Muon PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET Muon PROPERTY FOLDER "MantidFramework")

include_directories(inc)

target_link_libraries(Muon
                      LINK_PRIVATE
                      ${TCMALLOC_LIBRARIES_LINKTIME}
                      ${MANTIDLIBS}
                      ${GSL_LIBRARIES})

# Add the unit tests directory
add_subdirectory(test)

#
# Installation settings
#

mtd_install_targets(TARGETS
                    Muon
                    INSTALL_DIRS
                    ${PLUGINS_DIR}
                    ${WORKBENCH_PLUGINS_DIR})
