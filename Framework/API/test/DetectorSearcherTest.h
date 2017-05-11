#ifndef MANTID_API_DETECTORSEARCHERTEST_H_
#define MANTID_API_DETECTORSEARCHERTEST_H_

#include "MantidAPI/DetectorSearcher.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidKernel/V3D.h"

#include <cmath>
#include <cxxtest/TestSuite.h>

using Mantid::Kernel::V3D;
using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;

class DetectorSearcherTest : public CxxTest::TestSuite {
public:
  void test_init() {
    auto inst1 = ComponentCreationHelper::createTestInstrumentCylindrical(
        3, V3D(0, 0, -1), V3D(0, 0, 0), 1.6, 1.0);
    auto inst2 =
        ComponentCreationHelper::createTestInstrumentRectangular2(1, 100);

    ExperimentInfo expInfo1;
    expInfo1.setInstrument(inst1);
    ExperimentInfo expInfo2;
    expInfo2.setInstrument(inst2);

    TS_ASSERT_THROWS_NOTHING(
        DetectorSearcher searcher(inst1, expInfo1.detectorInfo()))
    TS_ASSERT_THROWS_NOTHING(
        DetectorSearcher searcher(inst2, expInfo2.detectorInfo()))
  }

  void test_search_cylindrical() {
    auto inst = ComponentCreationHelper::createTestInstrumentCylindrical(
        3, V3D(0, 0, -1), V3D(0, 0, 0), 1.6, 1.0);

    ExperimentInfo expInfo;
    expInfo.setInstrument(inst);

    DetectorSearcher searcher(inst, expInfo.detectorInfo());
    const auto checkResult = [&searcher](const V3D &q, size_t index) {
      const auto result = searcher.findDetectorIndex(q);
      TS_ASSERT(std::get<0>(result))
      TS_ASSERT_EQUALS(std::get<1>(result), index)
    };

    checkResult(V3D(0.913156, 0.285361, 0.291059), 0);
    checkResult(V3D(-6.09343e-17, 0.995133, 0.0985376), 1);
    checkResult(V3D(-0.913156, 0.285361, 0.291059), 2);
    checkResult(V3D(0.959758, -1.17536e-16, 0.280828), 3);

    checkResult(V3D(-0.959758, -0, 0.280828), 5);
    checkResult(V3D(0.913156, -0.285361, 0.291059), 6);
    checkResult(V3D(-6.09343e-17, -0.995133, 0.0985376), 7);
    checkResult(V3D(-0.913156, -0.285361, 0.291059), 8);
    checkResult(V3D(0.942022, 0.294382, 0.161038), 9);
    checkResult(V3D(-6.11563e-17, 0.998759, 0.0498137), 10);
    checkResult(V3D(-0.942022, 0.294382, 0.161038), 11);
    checkResult(V3D(0.988034, -1.20999e-16, 0.154233), 12);

    checkResult(V3D(-0.988034, -0, 0.154233), 14);
    checkResult(V3D(0.942022, -0.294382, 0.161038), 15);
    checkResult(V3D(-6.11563e-17, -0.998759, 0.0498137), 16);
    checkResult(V3D(-0.942022, -0.294382, 0.161038), 17);
    checkResult(V3D(0.948717, 0.296474, 0.109725), 18);
    checkResult(V3D(-6.11984e-17, 0.999446, 0.0332779), 19);
    checkResult(V3D(-0.948717, 0.296474, 0.109725), 20);
    checkResult(V3D(0.994483, -1.21789e-16, 0.104898), 21);

    checkResult(V3D(-0.994483, -0, 0.104898), 23);
    checkResult(V3D(0.948717, -0.296474, 0.109725), 24);
    checkResult(V3D(-6.11984e-17, -0.999446, 0.0332779), 25);
    checkResult(V3D(-0.948717, -0.296474, 0.109725), 26);
  }

  void test_invalid_rectangular() {
    auto inst =
        ComponentCreationHelper::createTestInstrumentRectangular2(1, 100);

    ExperimentInfo expInfo;
    expInfo.setInstrument(inst);
    const auto &info = expInfo.detectorInfo();

    DetectorSearcher searcher(inst, info);
    const auto resultNull = searcher.findDetectorIndex(V3D(0, 0, 0));
    TS_ASSERT(!std::get<0>(resultNull))

    const auto resultNaN = searcher.findDetectorIndex(V3D(NAN, NAN, NAN));
    TS_ASSERT(!std::get<0>(resultNaN))
  }

  void test_invalid_cylindrical() {
    auto inst = ComponentCreationHelper::createTestInstrumentCylindrical(
        3, V3D(0, 0, -1), V3D(0, 0, 0), 1.6, 1.0);
    ExperimentInfo expInfo;
    expInfo.setInstrument(inst);
    const auto &info = expInfo.detectorInfo();

    DetectorSearcher searcher(inst, info);
    const auto resultNull = searcher.findDetectorIndex(V3D(0, 0, 0));
    TS_ASSERT(!std::get<0>(resultNull))

    const auto resultNaN = searcher.findDetectorIndex(V3D(NAN, NAN, NAN));
    TS_ASSERT(!std::get<0>(resultNaN))
  }

  void test_search_rectangular() {
    auto inst =
        ComponentCreationHelper::createTestInstrumentRectangular2(1, 100);
    ExperimentInfo expInfo;
    expInfo.setInstrument(inst);
    const auto &info = expInfo.detectorInfo();

    DetectorSearcher searcher(inst, info);
    const auto checkResult = [&searcher](V3D q, size_t index) {
      const auto result = searcher.findDetectorIndex(q);
      TS_ASSERT(std::get<0>(result))
      TS_ASSERT_EQUALS(std::get<1>(result), index)
    };

    for (size_t pointNo = 0; pointNo < info.size(); ++pointNo) {
      const auto &det = info.detector(pointNo);
      const auto q = convertDetectorPositionToQ(det);
      checkResult(q, pointNo);
    }
  }

  V3D convertDetectorPositionToQ(const IDetector &det) {
    const auto tt1 = det.getTwoTheta(V3D(0, 0, 0), V3D(0, 0, 1)); // two theta
    const auto ph1 = det.getPhi();                                // phi
    auto E1 =
        V3D(-std::sin(tt1) * std::cos(ph1), -std::sin(tt1) * std::sin(ph1),
            1. - std::cos(tt1));  // end of trajectory
    return E1 * (1. / E1.norm()); // normalize
  }
};

class DetectorSearcherTestPerformance : public CxxTest::TestSuite {
public:
  void test_rectangular() {
    auto inst =
        ComponentCreationHelper::createTestInstrumentRectangular2(1, 100);
    ExperimentInfo expInfo;
    expInfo.setInstrument(inst);
    const auto &info = expInfo.detectorInfo();

    DetectorSearcher searcher(inst, info);

    std::vector<double> xDirections(100);
    std::vector<double> yDirections(100);
    std::vector<double> zDirections(50);

    // create x values of the range -1 to 1
    int index = 0;
    double startValue = -1;
    std::generate(
        xDirections.begin(), xDirections.end(),
        [&index, &startValue]() { return startValue + index++ * 0.1; });

    // create z values of the range 0.1 to 1
    // ignore negative z values as these are not physical!
    index = 0;
    startValue = 0.1;
    std::generate(
        zDirections.begin(), zDirections.end(),
        [&index, &startValue]() { return startValue + index++ * 0.1; });

    yDirections = xDirections;

    size_t hitCount = 0;
    for (auto &x : xDirections) {
      for (auto &y : yDirections) {
        for (auto &z : zDirections) {
          const auto result = searcher.findDetectorIndex(V3D(x, y, z));
          if (std::get<0>(result))
            ++hitCount;
        }
      }
    }

    TS_ASSERT_EQUALS(hitCount, 246)
  }

  void test_cylindrical() {
    auto inst = ComponentCreationHelper::createTestInstrumentCylindrical(
        3, V3D(0, 0, -1), V3D(0, 0, 0), 1.6, 1.0);

    ExperimentInfo expInfo;
    expInfo.setInstrument(inst);
    const auto &info = expInfo.detectorInfo();

    DetectorSearcher searcher(inst, info);

    std::vector<double> xDirections(50);
    std::vector<double> yDirections(50);
    std::vector<double> zDirections(50);

    // create x values of the range -1 to 1
    int index = 0;
    double startValue = -1;
    std::generate(
        xDirections.begin(), xDirections.end(),
        [&index, &startValue]() { return startValue + index++ * 0.1; });

    // create z values of the range 0.1 to 1
    // ignore negative z values as these are not physical!
    index = 0;
    startValue = 0.1;
    std::generate(
        zDirections.begin(), zDirections.end(),
        [&index, &startValue]() { return startValue + index++ * 0.1; });

    yDirections = xDirections;

    size_t hitCount = 0;
    for (auto &x : xDirections) {
      for (auto &y : yDirections) {
        for (auto &z : zDirections) {
          const auto result = searcher.findDetectorIndex(V3D(x, y, z));
          if (std::get<0>(result))
            ++hitCount;
        }
      }
    }

    TS_ASSERT_EQUALS(hitCount, 16235)
  }
};

#endif
