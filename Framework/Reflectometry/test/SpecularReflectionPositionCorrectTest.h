// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "SpecularReflectionAlgorithmTest.h"
#include <cxxtest/TestSuite.h>

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/V3D.h"
#include "MantidReflectometry/SpecularReflectionPositionCorrect.h"
#include <cmath>
#include <utility>

using Mantid::Reflectometry::SpecularReflectionPositionCorrect;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class SpecularReflectionPositionCorrectTest : public CxxTest::TestSuite, public SpecularReflectionAlgorithmTest {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpecularReflectionPositionCorrectTest *createSuite() { return new SpecularReflectionPositionCorrectTest(); }
  static void destroySuite(SpecularReflectionPositionCorrectTest *suite) { delete suite; }

  SpecularReflectionPositionCorrectTest() {}

  void test_init() {
    SpecularReflectionPositionCorrect alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_theta_is_mandatory() {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::create1DWorkspaceConstant(1, 1, 1, true));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error &);
  }

  void test_theta_is_greater_than_zero_else_throws() {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::create1DWorkspaceConstant(1, 1, 1, true));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS(alg.setProperty("TwoThetaIn", 0.0), std::invalid_argument &);
  }

  void test_theta_is_less_than_ninety_else_throws() {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::create1DWorkspaceConstant(1, 1, 1, true));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS(alg.setProperty("TwoThetaIn", 90.0), std::invalid_argument &);
  }

  void test_throws_if_SpectrumNumbersOfDetectors_less_than_zero() {
    IAlgorithm_sptr alg = std::make_shared<SpecularReflectionPositionCorrect>();
    alg->setRethrows(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", WorkspaceCreationHelper::create1DWorkspaceConstant(1, 1, 1, true));
    alg->setPropertyValue("OutputWorkspace", "test_out");
    alg->setProperty("TwoThetaIn", 10.0);

    SpecularReflectionAlgorithmTest::test_throws_if_SpectrumNumbersOfDetectors_less_than_zero(alg);
  }

  void test_throws_if_SpectrumNumbersOfDetectors_outside_range() {
    IAlgorithm_sptr alg = std::make_shared<SpecularReflectionPositionCorrect>();
    alg->setRethrows(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 1, 1));
    alg->setPropertyValue("OutputWorkspace", "test_out");
    alg->setProperty("TwoThetaIn", 10.0);

    SpecularReflectionAlgorithmTest::test_throws_if_SpectrumNumbersOfDetectors_outside_range(alg);
  }

  void test_throws_if_DetectorComponentName_unknown() {
    IAlgorithm_sptr alg = std::make_shared<SpecularReflectionPositionCorrect>();
    alg->setRethrows(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 1, 1));
    alg->setPropertyValue("OutputWorkspace", "test_out");
    alg->setProperty("TwoThetaIn", 10.0);

    SpecularReflectionAlgorithmTest::test_throws_if_DetectorComponentName_unknown(alg);
  }

  void test_correct_point_detector_to_current_position() {
    auto toConvert = pointDetectorWS;
    auto referenceFrame = toConvert->getInstrument()->getReferenceFrame();
    auto moveComponentAlg = AlgorithmManager::Instance().create("MoveInstrumentComponent");
    moveComponentAlg->initialize();
    moveComponentAlg->setProperty("Workspace", toConvert);
    const std::string componentName = "point-detector";
    moveComponentAlg->setProperty("ComponentName", componentName);
    moveComponentAlg->setProperty("RelativePosition", true);
    moveComponentAlg->setProperty(referenceFrame->pointingUpAxis(),
                                  0.5); // Give the point detector a starting vertical offset.
    // Execute the movement.
    moveComponentAlg->execute();

    VerticalHorizontalOffsetType offsetTuple =
        determine_vertical_and_horizontal_offsets(toConvert); // Offsets before correction
    const double sampleToDetectorVerticalOffset = offsetTuple.get<0>();
    const double sampleToDetectorBeamOffset = offsetTuple.get<1>();

    // Based on the current positions, calculate the current incident theta.
    const double currentThetaInRad = std::atan(sampleToDetectorVerticalOffset / sampleToDetectorBeamOffset);
    const double currentThetaInDeg = currentThetaInRad * (180.0 / M_PI);

    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", toConvert);
    alg.setPropertyValue("OutputWorkspace", "test_out");
    alg.setProperty("TwoThetaIn", 2 * currentThetaInDeg);
    alg.execute();
    MatrixWorkspace_sptr corrected = alg.getProperty("OutputWorkspace");

    VerticalHorizontalOffsetType offsetTupleCorrected =
        determine_vertical_and_horizontal_offsets(corrected); // Positions after correction
    const double sampleToDetectorVerticalOffsetCorrected = offsetTupleCorrected.get<0>();
    const double sampleToDetectorBeamOffsetCorrected = offsetTupleCorrected.get<1>();

    // Positions should be identical to original. No correction
    TSM_ASSERT_DELTA("Vertical position should be unchanged", sampleToDetectorVerticalOffsetCorrected,
                     sampleToDetectorVerticalOffset, 1e-6);
    TSM_ASSERT_DELTA("Beam position should be unchanged", sampleToDetectorBeamOffsetCorrected,
                     sampleToDetectorBeamOffset, 1e-6);
  }

  void do_test_correct_point_detector_position(const std::string &detectorFindProperty = "",
                                               const std::string &stringValue = "") {
    MatrixWorkspace_sptr toConvert = pointDetectorWS;

    const double thetaInDegrees = 10.0; // Desired theta in degrees.
    const double thetaInRad = thetaInDegrees * (M_PI / 180);
    VerticalHorizontalOffsetType offsetTuple =
        determine_vertical_and_horizontal_offsets(toConvert); // Offsets before correction
    const double sampleToDetectorBeamOffsetExpected = offsetTuple.get<1>();
    const double sampleToDetectorVerticalOffsetExpected = std::tan(thetaInRad) * sampleToDetectorBeamOffsetExpected;

    SpecularReflectionPositionCorrect alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", toConvert);
    alg.setPropertyValue("OutputWorkspace", "test_out");
    if (!detectorFindProperty.empty())
      alg.setProperty(detectorFindProperty, stringValue);
    alg.setProperty("TwoThetaIn", 2 * thetaInDegrees);
    alg.execute();
    MatrixWorkspace_sptr corrected = alg.getProperty("OutputWorkspace");

    VerticalHorizontalOffsetType offsetTupleCorrected =
        determine_vertical_and_horizontal_offsets(corrected); // Positions after correction
    const double sampleToDetectorVerticalOffsetCorrected = offsetTupleCorrected.get<0>();
    const double sampleToDetectorBeamOffsetCorrected = offsetTupleCorrected.get<1>();

    // Positions should be identical to original. No correction
    TSM_ASSERT_DELTA("Vertical position should be unchanged", sampleToDetectorVerticalOffsetCorrected,
                     sampleToDetectorVerticalOffsetExpected, 1e-6);
    TSM_ASSERT_DELTA("Beam position should be unchanged", sampleToDetectorBeamOffsetCorrected,
                     sampleToDetectorBeamOffsetExpected, 1e-6);
  }

  void test_correct_point_detector_position_using_defaults_for_specifying_detector() {
    do_test_correct_point_detector_position();
  }

  void test_correct_point_detector_position_using_name_for_specifying_detector() {
    do_test_correct_point_detector_position("DetectorComponentName", "point-detector");
  }

  void test_correct_point_detector_position_using_spectrum_number_for_specifying_detector() {
    do_test_correct_point_detector_position("SpectrumNumbersOfDetectors", "4");
  }

  double do_test_correct_line_detector_position(const std::vector<int> &specNumbers, double thetaInDegrees,
                                                const std::string &detectorName = "lineardetector",
                                                bool strictSpectrumCheck = true) {
    auto toConvert = this->linearDetectorWS;

    SpecularReflectionPositionCorrect alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", toConvert);
    alg.setPropertyValue("OutputWorkspace", "test_out");
    alg.setProperty("SpectrumNumbersOfDetectors", specNumbers);
    alg.setProperty("StrictSpectrumChecking", strictSpectrumCheck);
    alg.setProperty("TwoThetaIn", thetaInDegrees);
    alg.execute();
    MatrixWorkspace_sptr corrected = alg.getProperty("OutputWorkspace");

    VerticalHorizontalOffsetType offsetTupleCorrected =
        determine_vertical_and_horizontal_offsets(corrected, std::move(detectorName)); // Positions after correction
    const double sampleToDetectorVerticalOffsetCorrected = offsetTupleCorrected.get<0>();

    return sampleToDetectorVerticalOffsetCorrected;
  }

  void test_correct_line_detector_position_many_spec_numbers_equal_averaging() {
    std::vector<int> specNumbers;
    specNumbers.emplace_back(74);
    double offset1 = do_test_correct_line_detector_position(specNumbers, 1, "lineardetector");

    specNumbers.emplace_back(73); // Add spectra below
    specNumbers.emplace_back(75); // Add spectra above
    double offset2 = do_test_correct_line_detector_position(specNumbers, 1, "lineardetector");

    TSM_ASSERT_DELTA("If grouping has worked correctly the group average "
                     "position should be the same as for spectra 74",
                     offset1, offset2, 1e-9);
  }

  void test_correct_line_detector_position_average_offset_by_one_pixel() {
    std::vector<int> specNumbers;
    specNumbers.emplace_back(100);
    double offset1 =
        do_test_correct_line_detector_position(specNumbers, 0.1, "lineardetector"); // Average spectrum number at 100

    specNumbers.emplace_back(101);
    specNumbers.emplace_back(102);
    double offset2 = do_test_correct_line_detector_position(specNumbers, 0.1,
                                                            "lineardetector"); // Average spectrum number now at 101.

    double const width = 1.2e-3; // Pixel height

    TS_ASSERT_DELTA(offset1, offset2 + width, 1e-9);
  }

  void test_correct_line_detector_position_average_offset_by_many_pixels() {
    std::vector<int> specNumbers;
    specNumbers.emplace_back(100);
    double offset1 =
        do_test_correct_line_detector_position(specNumbers, 0.1, "lineardetector"); // Average spectrum number at 100

    specNumbers.emplace_back(104);
    const bool strictSpectrumCheck = false;
    double offset2 = do_test_correct_line_detector_position(specNumbers, 0.1, "lineardetector",
                                                            strictSpectrumCheck); // Average spectrum number at 102

    double const width = 1.2e-3; // Pixel height

    TS_ASSERT_DELTA(offset1, offset2 + (2 * width), 1e-9);
  }

  void test_correct_line_detector_position_throws_with_non_sequential_spec_numbers() {
    std::vector<int> specNumbers;
    specNumbers.emplace_back(1);
    specNumbers.emplace_back(3); // Missing 2 in sequence.
    TS_ASSERT_THROWS(do_test_correct_line_detector_position(specNumbers, 0.1, "lineardetector"),
                     std::invalid_argument &);
  }
};
