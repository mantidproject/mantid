/*
 * IndexPeaksWithSatellitesTest.h
 *
 *  Created on: July, 2018
 *      Author: Vickie Lynch
 */

#ifndef INDEXPEAKSWITHSATELLITESTEST_H_
#define INDEXPEAKSWITHSATELLITESTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidCrystal/FindUBUsingIndexedPeaks.h"
#include "MantidCrystal/IndexPeaksWithSatellites.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class IndexPeaksWithSatellitesTest : public CxxTest::TestSuite {

public:
  void test_Init() {
    IndexPeaksWithSatellites alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_exec() {
    LoadIsawPeaks alg1;
    TS_ASSERT_THROWS_NOTHING(alg1.initialize())
    TS_ASSERT(alg1.isInitialized())
    alg1.setPropertyValue("Filename", "Modulated.peaks");
    alg1.setPropertyValue("OutputWorkspace", "peaks");

    TS_ASSERT(alg1.execute());
    TS_ASSERT(alg1.isExecuted());

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
            AnalysisDataService::Instance().retrieve("peaks")));
    TS_ASSERT(ws);
    if (!ws)
      return;

    FindUBUsingIndexedPeaks alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize())
    TS_ASSERT(alg2.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg2.setPropertyValue("ToleranceForSatellite", "0.05"));
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("PeaksWorkspace", "peaks"));
    TS_ASSERT_THROWS_NOTHING(alg2.execute(););
    TS_ASSERT(alg2.isExecuted());

    IndexPeaksWithSatellites alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("PeaksWorkspace", "peaks");
    alg.setProperty("ModVector1", "-0.493670,-0.440224,0.388226");
    alg.setPropertyValue("Tolerance", "0.1");
    alg.setPropertyValue("ToleranceForSatellite", "0.02");
    alg.setProperty("MaxOrder", "2");
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    PeaksWorkspace_sptr Modulated;
    TS_ASSERT_THROWS_NOTHING(
        Modulated = boost::dynamic_pointer_cast<PeaksWorkspace>(
            AnalysisDataService::Instance().retrieve("peaks")));

    TS_ASSERT(Modulated);
    TS_ASSERT_EQUALS(Modulated->getNumberPeaks(), 18);

    auto &peak0 = Modulated->getPeak(0);
    TS_ASSERT_DELTA(peak0.getH(), 1.0, .01);
    TS_ASSERT_DELTA(peak0.getK(), -1.0, .01);
    TS_ASSERT_DELTA(peak0.getL(), 2.0, .01);

    auto &peak3 = Modulated->getPeak(3);
    TS_ASSERT_DELTA(peak3.getH(), 1.49, .01);
    TS_ASSERT_DELTA(peak3.getK(), -0.56, .01);
    TS_ASSERT_DELTA(peak3.getL(), 1.61, .01);

    AnalysisDataService::Instance().remove("peaks");
  }
};

#endif /* INDEXPEAKSWITHSATELLITESTEST__H_ */
