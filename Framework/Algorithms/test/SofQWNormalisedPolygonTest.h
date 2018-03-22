#ifndef MANTID_ALGORITHMS_SOFQWNORMALISEDPOLYGONTEST_H_
#define MANTID_ALGORITHMS_SOFQWNORMALISEDPOLYGONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SofQWNormalisedPolygon.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidKernel/Unit.h"

#include "SofQWTest.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class SofQWNormalisedPolygonTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SofQWNormalisedPolygon alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Aliased_To_SofQW3() {
    SofQWNormalisedPolygon alg;
    TS_ASSERT_EQUALS("SofQW3", alg.alias())
  }

  void test_exec() {
    auto result =
        SofQWTest::runSQW<Mantid::Algorithms::SofQWNormalisedPolygon>();

    TS_ASSERT_EQUALS(result->getAxis(0)->length(), 1904);
    TS_ASSERT_EQUALS(result->getAxis(0)->unit()->unitID(), "DeltaE");
    TS_ASSERT_DELTA((*(result->getAxis(0)))(0), -0.5590, 0.0001);
    TS_ASSERT_DELTA((*(result->getAxis(0)))(999), -0.0971, 0.0001);
    TS_ASSERT_DELTA((*(result->getAxis(0)))(1900), 0.5728, 0.0001);

    TS_ASSERT_EQUALS(result->getAxis(1)->length(), 7);
    TS_ASSERT_EQUALS(result->getAxis(1)->unit()->unitID(), "MomentumTransfer");
    TS_ASSERT_EQUALS((*(result->getAxis(1)))(0), 0.5);
    TS_ASSERT_EQUALS((*(result->getAxis(1)))(3), 1.25);
    TS_ASSERT_EQUALS((*(result->getAxis(1)))(6), 2.0);

    const double delta(1e-08);
    TS_ASSERT_DELTA(result->y(0)[1160], 22.8567683273, delta);
    TS_ASSERT_DELTA(result->e(0)[1160], 0.2568965638, delta);

    TS_ASSERT_DELTA(result->y(1)[1145], 7.5942160104, delta);
    TS_ASSERT_DELTA(result->e(1)[1145], 0.2413193155, delta);

    TS_ASSERT_DELTA(result->y(2)[1200], 2.0249626546, delta);
    TS_ASSERT_DELTA(result->e(2)[1200], 0.0925193880, delta);

    TS_ASSERT_DELTA(result->y(3)[99], 0.0419939169, delta);
    TS_ASSERT_DELTA(result->e(3)[99], 0.0226037551, delta);

    TS_ASSERT_DELTA(result->y(4)[1654], 0.0167189448, delta);
    TS_ASSERT_DELTA(result->e(4)[1654], 0.0056801131, delta);

    TS_ASSERT_DELTA(result->y(5)[1025], 0.0808168496, delta);
    TS_ASSERT_DELTA(result->e(5)[1025], 0.0161117732, delta);

    // Spectra-detector mapping
    const size_t nspectra(6);
    using IDSet = std::set<int>;
    std::vector<IDSet> expectedIDs(nspectra);
    IDSet s1 = {3};
    expectedIDs[0] = s1;
    IDSet s2 = {13};
    expectedIDs[1] = s2;
    IDSet s3 = {13, 23};
    expectedIDs[2] = s3;
    IDSet s4 = {23, 33};
    expectedIDs[3] = s4;
    IDSet s5 = {33, 43};
    expectedIDs[4] = s5;
    IDSet s6 = {43};
    expectedIDs[5] = s6;

    for (size_t i = 0; i < nspectra; ++i) {
      const auto &spectrum = result->getSpectrum(i);
      Mantid::specnum_t specNoActual = spectrum.getSpectrumNo();
      Mantid::specnum_t specNoExpected = static_cast<Mantid::specnum_t>(i + 1);
      TS_ASSERT_EQUALS(specNoExpected, specNoActual);
      TS_ASSERT_EQUALS(expectedIDs[i], spectrum.getDetectorIDs());
    }
  }
};

class SofQWNormalisedPolygonTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SofQWNormalisedPolygonTestPerformance *createSuite() {
    return new SofQWNormalisedPolygonTestPerformance();
  }
  static void destroySuite(SofQWNormalisedPolygonTestPerformance *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  void testExec() {
    auto result =
        SofQWTest::runSQW<Mantid::Algorithms::SofQWNormalisedPolygon>();
  }
};

#endif /* MANTID_ALGORITHMS_SOFQW2TEST_H_ */
