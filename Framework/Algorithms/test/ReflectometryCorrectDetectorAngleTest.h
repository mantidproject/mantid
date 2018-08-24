#ifndef MANTID_ALGORITHMS_REFLECTOMETRYCORRECTDETECTORANGLETEST_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYCORRECTDETECTORANGLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ReflectometryCorrectDetectorAngle.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using Mantid::Geometry::deg2rad;

class ReflectometryCorrectDetectorAngleTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryCorrectDetectorAngleTest *createSuite() {
    return new ReflectometryCorrectDetectorAngleTest();
  }
  static void destroySuite(ReflectometryCorrectDetectorAngleTest *suite) {
    delete suite;
  }

  void test_Init() {
    Algorithms::ReflectometryCorrectDetectorAngle alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_SetTwoTheta() {
    using namespace WorkspaceCreationHelper;
    constexpr double pixelSize{0.003};
    constexpr double angle{1.23};
    constexpr double startX{0.};
    Kernel::V3D const slit1Pos(-2, 0, 0);
    Kernel::V3D const slit2Pos(-1, 0, 0);
    double const slitOpening{0.001};
    Kernel::V3D const sourcePos(-15, 0, 0);
    Kernel::V3D const monitorPos(-3, 0, 0);
    Kernel::V3D const samplePos(0, 0, 0);
    Kernel::V3D const detectorPos(5.42, 0, 0);
    auto inputWS = create2DWorkspaceWithReflectometryInstrument(
        startX, slit1Pos, slit2Pos, slitOpening, slitOpening, sourcePos,
        monitorPos, samplePos, detectorPos);
    Algorithms::ReflectometryCorrectDetectorAngle alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TwoTheta", angle))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DetectorComponent", "point-detector"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PixelSize", pixelSize))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT(onlyInstrumentsDiffer(inputWS, outputWS));
    auto const &spectrumInfo = outputWS->spectrumInfo();
    TS_ASSERT(!spectrumInfo.isMonitor(0));
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(0), angle * deg2rad, 1e-10)
  }

  void test_SetTwoThetaWhenSampleNotInOrigin() {
    using namespace WorkspaceCreationHelper;
    constexpr double pixelSize{0.003};
    constexpr double angle{1.23};
    auto inputWS = create2DWorkspaceWithReflectometryInstrument();
    Algorithms::ReflectometryCorrectDetectorAngle alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TwoTheta", angle))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DetectorComponent", "point-detector"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PixelSize", pixelSize))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT(onlyInstrumentsDiffer(inputWS, outputWS));
    auto const &spectrumInfo = outputWS->spectrumInfo();
    TS_ASSERT(!spectrumInfo.isMonitor(0));
    TS_ASSERT_DELTA(spectrumInfo.twoTheta(0), angle * deg2rad, 1e-10)
  }

private:
  bool onlyInstrumentsDiffer(API::MatrixWorkspace_sptr &ws1,
                             API::MatrixWorkspace_sptr &ws2) {
    auto alg =
        API::AlgorithmManager::Instance().createUnmanaged("CompareWorkspaces");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("Workspace1", ws1);
    alg->setProperty("Workspace2", ws2);
    alg->setProperty("CheckType", true);
    alg->setProperty("CheckAxes", true);
    alg->setProperty("CheckSpectraMap", true);
    alg->setProperty("CheckInstrument", false);
    alg->setProperty("CheckMasking", true);
    alg->setProperty("CheckSample", true);
    alg->execute();
    bool const result = alg->getProperty("Result");
    return result;
  }
};

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYCORRECTDETECTORANGLETEST_H_ */
