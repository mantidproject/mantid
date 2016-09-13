#ifndef MANTID_ALGORITHMS_CALC_COUNTRATE_TEST_H_
#define MANTID_ALGORITHMS_CALC_COUNTRATE_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CalcCountRate.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using Mantid::DataObjects::Workspace2D_sptr;
using namespace Mantid::API;
using namespace Mantid::Algorithms;

class CalcCountRateTester : public CalcCountRate {
public:
  void setSearchRanges(DataObjects::EventWorkspace_sptr &InputWorkspace) {
    CalcCountRate::setWSDataRanges(InputWorkspace);
  }
  std::tuple<double, double, bool> getXRanges() const {
    return std::tuple<double, double, bool>(m_XRangeMin, m_XRangeMax,
                                            m_rangeExplicit);
  }
};

class CalcCountRateTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalcCountRateTest *createSuite() { return new CalcCountRateTest(); }
  static void destroySuite(CalcCountRateTest *suite) { delete suite; }

  void test_Init() {
    CalcCountRate alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }
  void test_ranges() {

    DataObjects::EventWorkspace_sptr sws =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 10,
                                                                        false);

    CalcCountRateTester alg;
    alg.initialize();
    alg.setProperty("Workspace", sws);

    alg.setSearchRanges(sws);

    auto ranget = alg.getXRanges();
    TS_ASSERT_DELTA(std::get<0>(ranget), 0., 1.e-8);
    TS_ASSERT_DELTA(std::get<1>(ranget), 100., 1.e-8);
    TS_ASSERT(!std::get<2>(ranget));
    //--------------------------------------------------------------------
    alg.setProperty("Workspace", sws);
    alg.setProperty("XMax", 20.);
    alg.setProperty("RangeUnits", "dSpacing");

    alg.setSearchRanges(sws);

    ranget = alg.getXRanges();
    TS_ASSERT_DELTA(std::get<0>(ranget), 0., 1.e-8);
    TS_ASSERT_DELTA(std::get<1>(ranget), 20., 1.e-8);
    TS_ASSERT(std::get<2>(ranget));

    //--------------------------------------------------------------------
    sws = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(
        2, 10, false);

    alg.setProperty("XMin", 1.);
    alg.setProperty("XMax", 30.);
    alg.setProperty("RangeUnits", "Energy");

    alg.setSearchRanges(sws);

    ranget = alg.getXRanges();
    TS_ASSERT_DELTA(std::get<0>(ranget), 1., 1.e-8);
    TS_ASSERT_DELTA(std::get<1>(ranget), 30., 1.e-8);
    TS_ASSERT(std::get<2>(ranget));

    double XRangeMin, XRangeMax;
    sws->getEventXMinMax(XRangeMin,XRangeMax);

    TS_ASSERT(std::get<0>(ranget) < XRangeMin);
    TS_ASSERT(std::get<1>(ranget) > XRangeMax);


  }
};

#endif /* MANTID_ALGORITHMS_CALC_COUNTRATE_TEST_H_ */
