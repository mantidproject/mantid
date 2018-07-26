/*
 * FindUBUsingIndexedPeakswithSatellitesTest.h
 *
 *  Created on: July, 2018
 *      Author: Vickie Lynch
 */

#ifndef FINDUBUSINGINDEXEDPEAKSWITHSATELLITESTEST_H_
#define FINDUBUSINGINDEXEDPEAKSWITHSATELLITESTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/PredictSatellitePeaks.h"
#include "MantidCrystal/FindUBUsingIndexedPeakswithSatellites.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidCrystal/FindUBUsingIndexedPeaks.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidCrystal/CalculateUMatrix.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class FindUBUsingIndexedPeakswithSatellitesTest : public CxxTest::TestSuite {

public:
  void test_Init() {
    FindUBUsingIndexedPeakswithSatellites alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_exec() {
    std::string WSName("peaks");
    LoadNexusProcessed loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "TOPAZ_3007.peaks.nxs");
    loader.setPropertyValue("OutputWorkspace", WSName);

    TS_ASSERT(loader.execute());
    TS_ASSERT(loader.isExecuted());

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
            AnalysisDataService::Instance().retrieve(WSName)));
    TS_ASSERT(ws);
    if (!ws)
      return;
    FindUBUsingIndexedPeaks alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize())
    TS_ASSERT(alg2.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg2.execute(););
    TS_ASSERT(alg2.isExecuted());

    TS_ASSERT(ws->mutableSample().hasOrientedLattice());

    OrientedLattice latt = ws->mutableSample().getOrientedLattice();

    double correct_UB[] = {-0.04542050, 0.04061990,  -0.0122354,
                           0.00140347,  -0.00318493, 0.116545,
                           0.05749760,  0.03223800,  0.02737380};

    std::vector<double> UB_calculated = latt.getUB().getVector();

    for (size_t i = 0; i < 9; i++) {
      TS_ASSERT_DELTA(correct_UB[i], UB_calculated[i], 5e-4);
    }

    PredictSatellitePeaks alg3;
    TS_ASSERT_THROWS_NOTHING(alg3.initialize());
    TS_ASSERT(alg3.isInitialized());

    alg3.setProperty("Peaks", WSName);

    alg3.setProperty("SatellitePeaks", std::string("SatellitePeaks"));
    alg3.setProperty("ModVector1", "0.5,0,.2");
    alg3.setProperty("MaxOrder", "1");
    TS_ASSERT(alg3.execute());
    TS_ASSERT(alg3.isExecuted());
    alg3.setPropertyValue("SatellitePeaks", "SatellitePeaks");
    PeaksWorkspace_sptr SatellitePeaks = alg3.getProperty("SatellitePeaks");

    FindUBUsingIndexedPeakswithSatellites alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setProperty("PeaksWorkspace", std::string("SatellitePeaks"));
    alg.setProperty("Tolerance", "0.25");
    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check that we set an oriented lattice
    TS_ASSERT(SatellitePeaks->mutableSample().hasOrientedLattice());

    OrientedLattice lattSat =
        SatellitePeaks->mutableSample().getOrientedLattice();

    std::vector<double> UB_calculatedSat = lattSat.getUB().getVector();

    for (size_t i = 0; i < 9; i++) {
      TS_ASSERT_DELTA(correct_UB[i], UB_calculatedSat[i], 5e-4);
    }

    AnalysisDataService::Instance().remove(WSName);
  }
};

#endif /* FINDUBUSINGINDEXEDPEAKSWITHSATELLITESTEST__H_ */
