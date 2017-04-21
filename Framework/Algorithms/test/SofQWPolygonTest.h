#ifndef MANTID_ALGORITHMS_SOFQWPOLYGONTEST_H_
#define MANTID_ALGORITHMS_SOFQWPOLYGONTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SofQWPolygon.h"
#include "MantidAPI/WorkspaceHistory.h"

#include "SofQWTest.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;

class SofQWPolygonTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    SofQWPolygon alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Aliased_To_SofQW2() {
    SofQWPolygon alg;
    TS_ASSERT_EQUALS("SofQW2", alg.alias())
  }

  void test_exec() {
    auto result = SofQWTest::runSQW<Mantid::Algorithms::SofQWPolygon>();

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
    TS_ASSERT_DELTA(result->y(0)[1160], 17.5583314826, delta);
    TS_ASSERT_DELTA(result->e(0)[1160], 0.197345265992, delta);

    TS_ASSERT_DELTA(result->y(1)[1145], 4.61301046588, delta);
    TS_ASSERT_DELTA(result->e(1)[1145], 0.0721823446635, delta);

    TS_ASSERT_DELTA(result->y(2)[1200], 1.33394133548, delta);
    TS_ASSERT_DELTA(result->e(2)[1200], 0.0419839252961, delta);

    TS_ASSERT_DELTA(result->y(3)[99], 0.0446085388561, delta);
    TS_ASSERT_DELTA(result->e(3)[99], 0.0185049423467, delta);

    TS_ASSERT_DELTA(result->y(4)[1654], 0.0171136490957, delta);
    TS_ASSERT_DELTA(result->e(4)[1654], 0.005007299861, delta);

    TS_ASSERT_DELTA(result->y(5)[1025], 0.0516113202152, delta);
    TS_ASSERT_DELTA(result->e(5)[1025], 0.0102893133461, delta);

    // Spectra-detector mapping
    const size_t nspectra(6);
    typedef std::set<int> IDSet;
    std::vector<IDSet> expectedIDs(nspectra);
    IDSet s1 = {3};
    expectedIDs[0] = s1;
    IDSet s2 = {13};
    expectedIDs[1] = s2;
    IDSet s3 = {23};
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

#endif /* MANTID_ALGORITHMS_SofQWPolygonTEST_H_ */
