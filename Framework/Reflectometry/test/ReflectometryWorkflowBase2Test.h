// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/ReflectometryHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidReflectometry/ReflectometryWorkflowBase2.h"

#include <cxxtest/TestSuite.h>
#include <string>

using namespace Mantid::Reflectometry;
using namespace Mantid::FrameworkTestHelpers;

class ReflectometryWorkflowBase2Stub : public ReflectometryWorkflowBase2 {
public:
  // Override pure virtual functions
  const std::string name() const override { return "ReflectometryWorkflowBase2Stub"; }
  int version() const override { return 2; }
  const std::string summary() const override { return "ReflectometryWorkflowBase2 stub for testing"; }
  void init() override {}
  void exec() override {}

  // Make the test a friend so we can test protected functions
  friend class ReflectometryWorkflowBase2Test;
};

class ReflectometryWorkflowBase2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryWorkflowBase2Test *createSuite() { return new ReflectometryWorkflowBase2Test(); }
  static void destroySuite(ReflectometryWorkflowBase2Test *suite) { delete suite; }

  void test_find_processing_instructions_from_IPF_for_point_detector_with_single_pixel() {
    auto workspace = createREFL_WS(5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, "PointDetector");
    ReflectometryWorkflowBase2Stub alg;
    alg.initAnalysisProperties();

    auto instructions = alg.findProcessingInstructions(workspace->getInstrument(), workspace);

    TS_ASSERT_EQUALS(instructions, "2");
  }

  void test_find_processing_instructions_from_IPF_for_point_detector_with_two_pixels() {
    auto workspace = createREFL_WS(5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, "PointDetector2Pixels");
    ReflectometryWorkflowBase2Stub alg;
    alg.initAnalysisProperties();
    alg.setProperty("AnalysisMode", "PointDetectorAnalysis");

    auto instructions = alg.findProcessingInstructions(workspace->getInstrument(), workspace);

    TS_ASSERT_EQUALS(instructions, "2-3");
  }

  void test_find_processing_instructions_from_IPF_for_point_detector_with_empty_start_throws() {
    // Due to a quirk of Mantid where it will load params from a previous parameters file for the same instrument,
    // and keep those params even if we re-load a different params file without them, we need to use the REFLEMPTY
    // instrument in order to test that the params are missing
    auto workspace = createREFL_WS(5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, "", "EMPTY");
    ReflectometryWorkflowBase2Stub alg;
    alg.initAnalysisProperties();
    alg.setProperty("AnalysisMode", "PointDetectorAnalysis");

    TS_ASSERT_THROWS(alg.findProcessingInstructions(workspace->getInstrument(), workspace), std::runtime_error const &);
  }

  void test_find_processing_instructions_from_IPF_for_point_detector_with_invalid_start_throws() {
    // Due to a quirk of Mantid where it will load params from a previous parameters file for the same instrument,
    // and keep those params even if we re-load a different params file without them, we need to use the REFLEMPTY
    // instrument in order to test that the params are missing
    auto workspace = createREFL_WS(5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, "PointDetector_InvalidStart");
    ReflectometryWorkflowBase2Stub alg;
    alg.initAnalysisProperties();
    alg.setProperty("AnalysisMode", "PointDetectorAnalysis");

    TS_ASSERT_THROWS(alg.findProcessingInstructions(workspace->getInstrument(), workspace), std::out_of_range const &);
  }

  void test_find_processing_instructions_from_IPF_for_point_detector_with_invalid_stop_throws() {
    // Due to a quirk of Mantid where it will load params from a previous parameters file for the same instrument,
    // and keep those params even if we re-load a different params file without them, we need to use the REFLEMPTY
    // instrument in order to test that the params are missing
    auto workspace = createREFL_WS(5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, "PointDetector_InvalidStop");
    ReflectometryWorkflowBase2Stub alg;
    alg.initAnalysisProperties();
    alg.setProperty("AnalysisMode", "PointDetectorAnalysis");

    TS_ASSERT_THROWS(alg.findProcessingInstructions(workspace->getInstrument(), workspace), std::out_of_range const &);
  }

  void test_find_processing_instructions_from_IPF_for_multi_detector() {
    auto workspace = createREFL_WS(5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, "MultiDetector");
    ReflectometryWorkflowBase2Stub alg;
    alg.initAnalysisProperties();
    alg.setProperty("AnalysisMode", "MultiDetectorAnalysis");

    auto instructions = alg.findProcessingInstructions(workspace->getInstrument(), workspace);

    TS_ASSERT_EQUALS(instructions, "2-3");
  }

  void test_find_processing_instructions_from_IPF_for_multi_detector_with_no_stop_uses_last_index() {
    // Due to a quirk of Mantid where it will load params from a previous parameters file for the same instrument,
    // and keep those params even if we re-load a different params file without them, we need to name the test
    // instrument unique here
    auto workspace = createREFL_WS(5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, "MultiDetector_NoStop", "MULTI");
    ReflectometryWorkflowBase2Stub alg;
    alg.initAnalysisProperties();
    alg.setProperty("AnalysisMode", "MultiDetectorAnalysis");

    auto instructions = alg.findProcessingInstructions(workspace->getInstrument(), workspace);

    auto const expected = std::string("2-") + std::to_string(workspace->getNumberHistograms() - 1);
    TS_ASSERT_EQUALS(instructions, expected);
  }

  void test_find_processing_instructions_from_IPF_for_multi_detector_with_empty_start_throws() {
    // Due to a quirk of Mantid where it will load params from a previous parameters file for the same instrument,
    // and keep those params even if we re-load a different params file without them, we need to to use the REFLEMPTY
    // instrument in order to test that the params are missing
    auto workspace = createREFL_WS(5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, "", "EMPTY");
    ReflectometryWorkflowBase2Stub alg;
    alg.initAnalysisProperties();
    alg.setProperty("AnalysisMode", "MultiDetectorAnalysis");

    TS_ASSERT_THROWS(alg.findProcessingInstructions(workspace->getInstrument(), workspace), std::runtime_error const &);
  }

  void test_find_processing_instructions_from_IPF_for_multi_detector_with_invalid_start_throws() {
    // Due to a quirk of Mantid where it will load params from a previous parameters file for the same instrument,
    // and keep those params even if we re-load a different params file without them, we need to to use the REFLEMPTY
    // instrument in order to test that the params are missing
    auto workspace = createREFL_WS(5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, "MultiDetector_InvalidStart");
    ReflectometryWorkflowBase2Stub alg;
    alg.initAnalysisProperties();
    alg.setProperty("AnalysisMode", "MultiDetectorAnalysis");

    TS_ASSERT_THROWS(alg.findProcessingInstructions(workspace->getInstrument(), workspace), std::out_of_range const &);
  }

  void test_find_processing_instructions_from_IPF_for_multi_detector_with_invalid_stop_throws() {
    // Due to a quirk of Mantid where it will load params from a previous parameters file for the same instrument,
    // and keep those params even if we re-load a different params file without them, we need to to use the REFLEMPTY
    // instrument in order to test that the params are missing
    auto workspace = createREFL_WS(5, 100.0, 500.0, {1.0, 2.0, 3.0, 4.0, 5.0}, "MultiDetector_InvalidStop");
    ReflectometryWorkflowBase2Stub alg;
    alg.initAnalysisProperties();
    alg.setProperty("AnalysisMode", "MultiDetectorAnalysis");

    TS_ASSERT_THROWS(alg.findProcessingInstructions(workspace->getInstrument(), workspace), std::out_of_range const &);
  }
};
