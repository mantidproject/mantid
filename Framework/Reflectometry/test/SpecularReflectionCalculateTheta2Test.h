// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "SpecularReflectionAlgorithmTest.h"

#include "MantidReflectometry/SpecularReflectionCalculateTheta2.h"

using namespace Mantid::Reflectometry;
using namespace Mantid::API;

class SpecularReflectionCalculateTheta2Test : public CxxTest::TestSuite, public SpecularReflectionAlgorithmTest {

private:
  Mantid::API::IAlgorithm_sptr makeAlgorithm() const {
    auto alg = std::make_shared<SpecularReflectionCalculateTheta2>();
    alg->setRethrows(true);
    alg->setChild(true);
    alg->initialize();
    return alg;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpecularReflectionCalculateTheta2Test *createSuite() { return new SpecularReflectionCalculateTheta2Test(); }
  static void destroySuite(SpecularReflectionCalculateTheta2Test *suite) { delete suite; }

  void test_Init() {
    SpecularReflectionCalculateTheta2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  SpecularReflectionCalculateTheta2Test() {}

  void test_throws_if_SpectrumNumbersOfDetectors_less_than_zero() {
    auto alg = makeAlgorithm();
    alg->setProperty("InputWorkspace", WorkspaceCreationHelper::create1DWorkspaceConstant(1, 1, 1, true));

    SpecularReflectionAlgorithmTest::test_throws_if_SpectrumNumbersOfDetectors_less_than_zero(alg);
  }

  void test_throws_if_SpectrumNumbersOfDetectors_outside_range() {
    auto alg = makeAlgorithm();
    alg->setProperty("InputWorkspace", WorkspaceCreationHelper::create1DWorkspaceConstant(1, 1, 1, true));

    SpecularReflectionAlgorithmTest::test_throws_if_SpectrumNumbersOfDetectors_outside_range(alg);
  }

  void test_throws_if_DetectorComponentName_unknown() {
    auto alg = makeAlgorithm();
    alg->setProperty("InputWorkspace", WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 1, 1));

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
    const double currentTwoThetaInRad = std::atan(sampleToDetectorVerticalOffset / sampleToDetectorBeamOffset);
    const double currentTwoThetaInDeg = currentTwoThetaInRad * (180.0 / M_PI);

    auto alg = this->makeAlgorithm();
    alg->setProperty("InputWorkspace", toConvert);
    alg->setProperty("DetectorComponentName", "point-detector");
    alg->setProperty("AnalysisMode", "PointDetectorAnalysis");
    alg->execute();
    const double twoThetaCalculated = alg->getProperty("TwoTheta");

    TSM_ASSERT_DELTA("Two theta value should be unchanged", twoThetaCalculated, currentTwoThetaInDeg, 1e-6);
  }
};
