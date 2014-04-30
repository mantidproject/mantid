#ifndef MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECTTEST_H_
#define MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SpecularReflectionPositionCorrect.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include <cmath>
#include <boost/tuple/tuple.hpp>

using Mantid::Algorithms::SpecularReflectionPositionCorrect;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace
{
  typedef boost::tuple<double, double> VerticalHorizontalOffsetType;
}

class SpecularReflectionPositionCorrectTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpecularReflectionPositionCorrectTest *createSuite()
  {
    return new SpecularReflectionPositionCorrectTest();
  }
  static void destroySuite(SpecularReflectionPositionCorrectTest *suite)
  {
    delete suite;
  }

  SpecularReflectionPositionCorrectTest()
  {
    FrameworkManager::Instance();
  }

  void test_init()
  {
    SpecularReflectionPositionCorrect alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize())
    TS_ASSERT( alg.isInitialized())
  }

  void test_theta_is_mandatory()
  {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::Create1DWorkspaceConstant(1, 1, 1));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error&);
  }

  void test_theta_is_greater_than_zero_else_throws()
  {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::Create1DWorkspaceConstant(1, 1, 1));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS(alg.setProperty("ThetaIn", 0.0), std::invalid_argument&);
  }

  void test_theta_is_less_than_ninety_else_throws()
  {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::Create1DWorkspaceConstant(1, 1, 1));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    TS_ASSERT_THROWS(alg.setProperty("ThetaIn", 90.0), std::invalid_argument&);
  }

  void test_throws_if_SpectrumNumbersOfGroupedDetectors_less_than_zero()
  {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::Create1DWorkspaceConstant(1, 1, 1));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    alg.setProperty("ThetaIn", 10.0);
    std::vector<int> invalid(1, -1);
    TS_ASSERT_THROWS(alg.setProperty("SpectrumNumbersOfGroupedDetectors", invalid),
        std::invalid_argument&);
  }

  void test_throws_if_SpectrumNumbersOfGroupedDetectors_outside_range()
  {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace",
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 1, 1));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    alg.setProperty("ThetaIn", 10.0);
    std::vector<int> invalid(1, 1e7);  // Well outside range
    alg.setProperty("SpectrumNumbersOfGroupedDetectors", invalid);  // Well outside range
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument&);
  }

  void test_throws_if_DetectorComponentName_unknown()
  {
    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("InputWorkspace",
        WorkspaceCreationHelper::create2DWorkspaceWithRectangularInstrument(1, 1, 1));
    alg.setPropertyValue("OutputWorkspace", "test_out");
    alg.setProperty("ThetaIn", 10.0);
    std::vector<int> invalid(1, 1e7);
    alg.setProperty("DetectorComponentName", "junk_value");  // Well outside range
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument&);
  }

  VerticalHorizontalOffsetType determine_vertical_and_horizontal_offsets(MatrixWorkspace_sptr ws)
  {
    auto instrument = ws->getInstrument();
    const V3D pointDetector = instrument->getComponentByName("point-detector")->getPos();
    const V3D surfaceHolder = instrument->getComponentByName("some-surface-holder")->getPos();
    const auto referenceFrame = instrument->getReferenceFrame();
    const V3D sampleToDetector = pointDetector - surfaceHolder;

    const double sampleToDetectorVerticalOffset = sampleToDetector.scalar_prod(
        referenceFrame->vecPointingUp());
    const double sampleToDetectorHorizontalOffset = sampleToDetector.scalar_prod(
        referenceFrame->vecPointingAlongBeam());

    return VerticalHorizontalOffsetType(sampleToDetectorVerticalOffset, sampleToDetectorHorizontalOffset);
  }

  void test_correct_point_detector_to_current_position()
  {
    auto loadAlg = AlgorithmManager::Instance().create("Load");
    loadAlg->initialize();
    loadAlg->setChild(true);
    loadAlg->setProperty("Filename", "INTER00013460.nxs");
    loadAlg->setPropertyValue("OutputWorkspace", "demo");
    loadAlg->execute();
    Workspace_sptr temp = loadAlg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr toConvert = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);

    VerticalHorizontalOffsetType offsetTuple = determine_vertical_and_horizontal_offsets(toConvert); // Offsets before correction
    const double sampleToDetectorVerticalOffset = offsetTuple.get<0>();
    const double sampleToDetectorBeamOffset = offsetTuple.get<1>();

    // Based on the current positions, calculate the current incident theta.
    const double currentThetaInRad = std::atan(
        sampleToDetectorVerticalOffset / sampleToDetectorBeamOffset);
    const double currentThetaInDeg = currentThetaInRad * (180.0 / M_PI);

    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", toConvert);
    alg.setPropertyValue("OutputWorkspace", "test_out");
    alg.setProperty("ThetaIn", currentThetaInDeg);
    alg.execute();
    MatrixWorkspace_sptr corrected = alg.getProperty("OutputWorkspace");

    VerticalHorizontalOffsetType offsetTupleCorrected = determine_vertical_and_horizontal_offsets(
        corrected); // Positions after correction
    const double sampleToDetectorVerticalOffsetCorrected = offsetTupleCorrected.get<0>();
    const double sampleToDetectorBeamOffsetCorrected = offsetTupleCorrected.get<1>();

    // Positions should be identical to original. No correction
    TSM_ASSERT_DELTA("Vertical position should be unchanged", sampleToDetectorVerticalOffsetCorrected,
        sampleToDetectorVerticalOffset, 1e-6);
    TSM_ASSERT_DELTA("Beam position should be unchanged", sampleToDetectorBeamOffsetCorrected,
        sampleToDetectorBeamOffset, 1e-6);
  }

  void test_correct_point_detector_position()
  {
    auto loadAlg = AlgorithmManager::Instance().create("Load");
    loadAlg->initialize();
    loadAlg->setChild(true);
    loadAlg->setProperty("Filename", "INTER00013460.nxs");
    loadAlg->setPropertyValue("OutputWorkspace", "demo");
    loadAlg->execute();
    Workspace_sptr temp = loadAlg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr toConvert = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);

    const double thetaInDegrees = 10.0; //Desired theta in degrees.
    const double thetaInRad = thetaInDegrees * (M_PI/180);
    VerticalHorizontalOffsetType offsetTuple = determine_vertical_and_horizontal_offsets(toConvert); // Offsets before correction
    const double sampleToDetectorBeamOffsetExpected = offsetTuple.get<1>();
    const double sampleToDetectorVerticalOffsetExpected = std::tan(thetaInRad) * sampleToDetectorBeamOffsetExpected;

    SpecularReflectionPositionCorrect alg;
    alg.setRethrows(true);
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", toConvert);
    alg.setPropertyValue("OutputWorkspace", "test_out");
    alg.setProperty("ThetaIn", thetaInDegrees);
    alg.execute();
    MatrixWorkspace_sptr corrected = alg.getProperty("OutputWorkspace");

    VerticalHorizontalOffsetType offsetTupleCorrected = determine_vertical_and_horizontal_offsets(
        corrected); // Positions after correction
    const double sampleToDetectorVerticalOffsetCorrected = offsetTupleCorrected.get<0>();
    const double sampleToDetectorBeamOffsetCorrected = offsetTupleCorrected.get<1>();

    // Positions should be identical to original. No correction
    TSM_ASSERT_DELTA("Vertical position should be unchanged", sampleToDetectorVerticalOffsetCorrected,
        sampleToDetectorVerticalOffsetExpected, 1e-6);
    TSM_ASSERT_DELTA("Beam position should be unchanged", sampleToDetectorBeamOffsetCorrected,
        sampleToDetectorBeamOffsetExpected, 1e-6);
  }

};

#endif /* MANTID_ALGORITHMS_SPECULARREFLECTIONPOSITIONCORRECTTEST_H_ */
