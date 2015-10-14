#ifndef MANTID_ALGORITHMS_SPECULARREFLECTIONCORRECTTHETATEST_H_
#define MANTID_ALGORITHMS_SPECULARREFLECTIONCORRECTTHETATEST_H_

#include <cxxtest/TestSuite.h>

#include "SpecularReflectionAlgorithmTest.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/SpecularReflectionCalculateTheta.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;

// clang-format off
class SpecularReflectionCalculateThetaTest: public CxxTest::TestSuite,
    public SpecularReflectionAlgorithmTest
      // clang-format on
      {

private:
  Mantid::API::IAlgorithm_sptr makeAlgorithm() const {
    IAlgorithm_sptr alg =
        boost::make_shared<SpecularReflectionCalculateTheta>();
    alg->setRethrows(true);
    alg->setChild(true);
    alg->initialize();
    return alg;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpecularReflectionCalculateThetaTest *createSuite() {
    return new SpecularReflectionCalculateThetaTest();
  }
  static void destroySuite(SpecularReflectionCalculateThetaTest *suite) {
    delete suite;
  }

  void test_Init() {
    SpecularReflectionCalculateTheta alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  SpecularReflectionCalculateThetaTest() {}

  void test_throws_if_SpectrumNumbersOfDetectors_less_than_zero() {
    IAlgorithm_sptr alg = makeAlgorithm();
    alg->setProperty(
        "InputWorkspace",
        WorkspaceCreationHelper::Create1DWorkspaceConstant(1, 1, 1));

    SpecularReflectionAlgorithmTest::
        test_throws_if_SpectrumNumbersOfDetectors_less_than_zero(alg);
  }

  void test_throws_if_SpectrumNumbersOfDetectors_outside_range() {
    IAlgorithm_sptr alg = makeAlgorithm();
    alg->setProperty(
        "InputWorkspace",
        WorkspaceCreationHelper::Create1DWorkspaceConstant(1, 1, 1));

    SpecularReflectionAlgorithmTest::
        test_throws_if_SpectrumNumbersOfDetectors_outside_range(alg);
  }

  void test_throws_if_DetectorComponentName_unknown() {
    IAlgorithm_sptr alg = makeAlgorithm();
    alg->setProperty(
        "InputWorkspace",
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(
            1, 1, 1));

    SpecularReflectionAlgorithmTest::
        test_throws_if_DetectorComponentName_unknown(alg);
  }

  void test_correct_point_detector_to_current_position() {
    auto toConvert = pointDetectorWS;
    auto referenceFrame = toConvert->getInstrument()->getReferenceFrame();
    auto moveComponentAlg =
        AlgorithmManager::Instance().create("MoveInstrumentComponent");
    moveComponentAlg->initialize();
    moveComponentAlg->setProperty("Workspace", toConvert);
    const std::string componentName = "point-detector";
    moveComponentAlg->setProperty("ComponentName", componentName);
    moveComponentAlg->setProperty("RelativePosition", true);
    moveComponentAlg->setProperty(
        referenceFrame->pointingUpAxis(),
        0.5); // Give the point detector a starting vertical offset.
    // Execute the movement.
    moveComponentAlg->execute();

    VerticalHorizontalOffsetType offsetTuple =
        determine_vertical_and_horizontal_offsets(
            toConvert); // Offsets before correction
    const double sampleToDetectorVerticalOffset = offsetTuple.get<0>();
    const double sampleToDetectorBeamOffset = offsetTuple.get<1>();

    // Based on the current positions, calculate the current incident theta.
    const double currentTwoThetaInRad =
        std::atan(sampleToDetectorVerticalOffset / sampleToDetectorBeamOffset);
    const double currentTwoThetaInDeg = currentTwoThetaInRad * (180.0 / M_PI);

    IAlgorithm_sptr alg = this->makeAlgorithm();
    alg->setProperty("InputWorkspace", toConvert);
    alg->setProperty("DetectorComponentName", "point-detector");
    alg->setProperty("AnalysisMode", "PointDetectorAnalysis");
    alg->execute();
    const double twoThetaCalculated = alg->getProperty("TwoTheta");

    TSM_ASSERT_DELTA("Two theta value should be unchanged", twoThetaCalculated,
                     currentTwoThetaInDeg, 1e-6);
  }
};

#endif /* MANTID_ALGORITHMS_SPECULARREFLECTIONCORRECTTHETATEST_H_ */
