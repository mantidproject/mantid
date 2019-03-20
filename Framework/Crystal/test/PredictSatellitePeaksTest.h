/*
 * PredictSatellitePeaksTest.h
 *
 *  Created on: July, 2018
 *      Author: Vickie Lynch
 */

#ifndef PREDICTSATELLITEPEAKSTEST_H_
#define PREDICTSATELLITEPEAKSTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Run.h"
#include "MantidCrystal/CalculateUMatrix.h"
#include "MantidCrystal/FindUBUsingIndexedPeaks.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidCrystal/PredictSatellitePeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class PredictSatellitePeaksTest : public CxxTest::TestSuite {

public:
  void test_Init() {
    PredictSatellitePeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_exec() {
    LoadIsawPeaks alg1;
    TS_ASSERT_THROWS_NOTHING(alg1.initialize())
    TS_ASSERT(alg1.isInitialized())
    alg1.setPropertyValue("Filename", "Modulated.peaks");
    alg1.setPropertyValue("OutputWorkspace", "Modulated");

    TS_ASSERT(alg1.execute());
    TS_ASSERT(alg1.isExecuted());

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
            AnalysisDataService::Instance().retrieve("Modulated")));
    TS_ASSERT(ws);
    if (!ws)
      return;

    FindUBUsingIndexedPeaks alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize())
    TS_ASSERT(alg2.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg2.setPropertyValue("ToleranceForSatellite", "0.05"));
    TS_ASSERT_THROWS_NOTHING(
        alg2.setPropertyValue("PeaksWorkspace", "Modulated"));
    TS_ASSERT_THROWS_NOTHING(alg2.execute(););
    TS_ASSERT(alg2.isExecuted());

    PredictSatellitePeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setProperty("Peaks", "Modulated");
    alg.setProperty("SatellitePeaks", std::string("SatellitePeaks"));
    alg.setProperty("MaxOrder", "1");
    alg.setProperty("GetModVectorsFromUB", true);
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());
    alg.setPropertyValue("SatellitePeaks", "SatellitePeaks");
    PeaksWorkspace_sptr SatellitePeaks = alg.getProperty("SatellitePeaks");
    TS_ASSERT(SatellitePeaks);

    TS_ASSERT_EQUALS(SatellitePeaks->getNumberPeaks(), 40);

    auto &peak0 = SatellitePeaks->getPeak(4);
    TS_ASSERT_DELTA(peak0.getH(), 1.49, .01);
    TS_ASSERT_DELTA(peak0.getK(), -0.56, .01);
    TS_ASSERT_DELTA(peak0.getL(), 1.61, .01);

    auto &peak3 = SatellitePeaks->getPeak(6);
    TS_ASSERT_DELTA(peak3.getH(), 1.51, .01);
    TS_ASSERT_DELTA(peak3.getK(), -0.44, .01);
    TS_ASSERT_DELTA(peak3.getL(), 1.39, .01);

    PredictSatellitePeaks alg4;
    TS_ASSERT_THROWS_NOTHING(alg4.initialize());
    TS_ASSERT(alg4.isInitialized());
    alg4.setProperty("Peaks", "Modulated");
    alg4.setProperty("SatellitePeaks", std::string("SatellitePeaks"));
    alg4.setProperty("MaxOrder", "1");
    alg4.setProperty("IncludeAllPeaksInRange", true);
    alg4.setProperty("GetModVectorsFromUB", true);
    alg4.setProperty("MinDSpacing", "0.5");
    alg4.setProperty("MaxDSpacing", "3");
    alg4.setProperty("WavelengthMin", "1");
    alg4.setProperty("WavelengthMax", "2");
    TS_ASSERT(alg4.execute())
    TS_ASSERT(alg4.isExecuted());
    alg4.setPropertyValue("SatellitePeaks", "SatellitePeaks");
    PeaksWorkspace_sptr SatellitePeaks2 = alg4.getProperty("SatellitePeaks");
    TS_ASSERT(SatellitePeaks2);

    TS_ASSERT_EQUALS(SatellitePeaks2->getNumberPeaks(), 939);

    AnalysisDataService::Instance().remove("Modulated");
  }
};

#endif /* PREDICTSATELLITEPEAKSTEST__H_ */
