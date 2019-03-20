// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_FIND_UB_USING_INDEXED_PEAKS_TEST_H_
#define MANTID_CRYSTAL_FIND_UB_USING_INDEXED_PEAKS_TEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Sample.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidCrystal/FindUBUsingIndexedPeaks.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace Mantid::Crystal;
using Mantid::Geometry::OrientedLattice;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;

class FindUBUsingIndexedPeaksTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    FindUBUsingIndexedPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Name of the output workspace.
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
    FindUBUsingIndexedPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Check that we set an oriented lattice
    TS_ASSERT(ws->mutableSample().hasOrientedLattice());
    // Check that the UB matrix is the same as in TOPAZ_3007.mat
    OrientedLattice latt = ws->mutableSample().getOrientedLattice();

    double correct_UB[] = {-0.04542050, 0.04061990,  -0.0122354,
                           0.00140347,  -0.00318493, 0.116545,
                           0.05749760,  0.03223800,  0.02737380};

    std::vector<double> UB_calculated = latt.getUB().getVector();

    for (size_t i = 0; i < 9; i++) {
      TS_ASSERT_DELTA(correct_UB[i], UB_calculated[i], 5e-4);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(WSName);
  }
  void test_mod() {
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

    // Add vector so cross terms will be true for test
    for (auto &peak : ws->getPeaks()) {
      const V3D mnp = peak.getIntMNP();
      V3D mnpNew = mnp;
      if (std::abs(mnp[0]) == 1)
        mnpNew[1] = 1;
      peak.setIntMNP(mnpNew);
    }
    FindUBUsingIndexedPeaks alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize())
    TS_ASSERT(alg2.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg2.setPropertyValue("ToleranceForSatellite", "0.05"));
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("PeaksWorkspace", "peaks"));
    TS_ASSERT_THROWS_NOTHING(alg2.execute(););
    TS_ASSERT(alg2.isExecuted());

    PeaksWorkspace_sptr Modulated;
    TS_ASSERT_THROWS_NOTHING(
        Modulated = boost::dynamic_pointer_cast<PeaksWorkspace>(
            AnalysisDataService::Instance().retrieve("peaks")));
    // Check that we set an oriented lattice
    TS_ASSERT(ws->mutableSample().hasOrientedLattice());
    // Check that the UB matrix is the same as in TOPAZ_3007.mat
    OrientedLattice latt = ws->mutableSample().getOrientedLattice();

    V3D correct_err1 = V3D(0.003723, 0.002231, 0.002820);
    V3D correct_err2 = V3D(0.000796, 0.002043, 0.002671);

    V3D err_calculated1 = latt.getVecErr(0);
    V3D err_calculated2 = latt.getVecErr(1);

    for (size_t i = 0; i < 3; i++) {
      TS_ASSERT_DELTA(correct_err1[i], err_calculated1[i], 5e-4);
      TS_ASSERT_DELTA(correct_err2[i], err_calculated2[i], 5e-4);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove("peaks");
  }
};

#endif /* MANTID_CRYSTAL_FIND_UB_USING_INDEXED_PEAKS_TEST_H_ */
