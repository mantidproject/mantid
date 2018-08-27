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
    checkSetTwoTheta(inputWS, angle);
  }

  void test_SetTwoThetaWhenSampleNotInOrigin() {
    using namespace WorkspaceCreationHelper;
    constexpr double angle{1.23};
    auto inputWS = create2DWorkspaceWithReflectometryInstrument();
    checkSetTwoTheta(inputWS, angle);
  }

  void test_SetNegativeTwoTheta() {
    using namespace WorkspaceCreationHelper;
    constexpr double angle{-1.23};
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
    checkSetTwoTheta(inputWS, angle);
  }

  void test_correctionWithLinePosition() {
    constexpr double angle{12.3};
    constexpr double pixelSize{0.03};
    auto inputWS = multiDetectorWorkspace(pixelSize);
    constexpr int lineIndex{2};
    Algorithms::ReflectometryCorrectDetectorAngle alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("LinePosition", static_cast<double>(lineIndex)))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TwoTheta", angle))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DetectorComponent", "detector-panel"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PixelSize", pixelSize))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT(onlyInstrumentsDiffer(inputWS, outputWS));
    auto const &spectrumInfo = outputWS->spectrumInfo();
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(lineIndex), angle * deg2rad,
                    1e-10)
    auto const &inSpectrumInfo = inputWS->spectrumInfo();
    for (size_t i = 0; i < spectrumInfo.size(); ++i) {
      TS_ASSERT_EQUALS(spectrumInfo.l2(i), inSpectrumInfo.l2(i))
    }
  }

  void test_directBeamCalibration() {
    constexpr double pixelSize{0.03};
    auto inputWS = multiDetectorWorkspace(pixelSize);
    auto directWS = multiDetectorWorkspace(pixelSize);
    constexpr int directLinePosition{0};
    Algorithms::ReflectometryCorrectDetectorAngle alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DetectorComponent", "detector-panel"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PixelSize", pixelSize))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DirectBeamWorkspace", directWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "DirectLinePosition", static_cast<double>(directLinePosition)))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT(onlyInstrumentsDiffer(inputWS, outputWS));
    auto const directBeamAngleOffset =
        directWS->spectrumInfo().signedTwoTheta(directLinePosition);
    auto const &spectrumInfo = outputWS->spectrumInfo();
    constexpr size_t centrePixel{1};
    TS_ASSERT_EQUALS(spectrumInfo.signedTwoTheta(centrePixel),
                     -directBeamAngleOffset)
    auto const &inSpectrumInfo = inputWS->spectrumInfo();
    for (size_t i = 0; i < spectrumInfo.size(); ++i) {
      TS_ASSERT_DELTA(spectrumInfo.l2(i), inSpectrumInfo.l2(i), 1e-10)
    }
  }

private:
  void checkSetTwoTheta(API::MatrixWorkspace_sptr &inputWS,
                        double const twoTheta) {
    // A random pixel size, not really needed.
    Algorithms::ReflectometryCorrectDetectorAngle alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TwoTheta", twoTheta))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DetectorComponent", "point-detector"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    API::MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_ASSERT(onlyInstrumentsDiffer(inputWS, outputWS));
    auto const &spectrumInfo = outputWS->spectrumInfo();
    TS_ASSERT_DELTA(spectrumInfo.signedTwoTheta(0), twoTheta * deg2rad, 1e-10)
  }

  static API::MatrixWorkspace_sptr
  extractDetectors(API::MatrixWorkspace_sptr &ws) {
    auto alg =
        API::AlgorithmManager::Instance().createUnmanaged("ExtractSpectra");
    alg->initialize();
    alg->setChild(true);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("OutputWorkspace", "_unused_for_child");
    alg->setProperty("StartWorkspaceIndex", 1);
    alg->execute();
    API::MatrixWorkspace_sptr detectorWS = alg->getProperty("OutputWorkspace");
    return detectorWS;
  }

  static API::MatrixWorkspace_sptr
  multiDetectorWorkspace(double const pixelSize) {
    using namespace WorkspaceCreationHelper;
    constexpr double startX{0.};
    Kernel::V3D const slit1Pos(-2, 0, 0);
    Kernel::V3D const slit2Pos(-1, 0, 0);
    double const slitOpening{0.001};
    Kernel::V3D const sourcePos(-15, 0, 0);
    Kernel::V3D const monitorPos(-3, 0, 0);
    Kernel::V3D const samplePos(0, 0, 0);
    Kernel::V3D const detectorPos(1.42, 0, 0);
    auto ws = create2DWorkspaceWithReflectometryInstrumentMultiDetector(
        startX, pixelSize, slit1Pos, slit2Pos, slitOpening, slitOpening,
        sourcePos, monitorPos, samplePos, detectorPos);
    ws = extractDetectors(ws);
    return ws;
  }

  static bool onlyInstrumentsDiffer(API::MatrixWorkspace_sptr &ws1,
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
