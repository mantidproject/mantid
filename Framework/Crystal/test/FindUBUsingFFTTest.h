// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Sample.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidCrystal/FindUBUsingFFT.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace Mantid::Crystal;
using Mantid::Geometry::OrientedLattice;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::DataHandling::LoadNexusProcessed;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class FindUBUsingFFTTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    FindUBUsingFFT alg;
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
        ws = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(WSName)));
    TS_ASSERT(ws);
    if (!ws)
      return;
    FindUBUsingFFT alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MinD", "8.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaxD", "13.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Tolerance", "0.15"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Check that we set an oriented lattice
    TS_ASSERT(ws->mutableSample().hasOrientedLattice());
    // Check that the UB matrix is the same as in TOPAZ_3007.mat
    OrientedLattice latt = ws->mutableSample().getOrientedLattice();

    double correct_UB[] = {0.0122354,  0.00480056, 0.0860404,   -0.1165450, 0.00178145,
                           -0.0045884, -0.0273738, -0.08973560, -0.0252595};

    std::vector<double> UB_calculated = latt.getUB().getVector();

    for (size_t i = 0; i < 9; i++) {
      TS_ASSERT_DELTA(correct_UB[i], UB_calculated[i], 5e-4);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(WSName);
  }

  void test_exec_LeanElasticPeak() {
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
        ws = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(WSName)));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Convert PeaksWorkspace to LeanElasticPeaksWorkspace
    auto lpw = std::make_shared<LeanElasticPeaksWorkspace>();
    for (auto peak : ws->getPeaks())
      lpw->addPeak(peak);
    AnalysisDataService::Instance().addOrReplace(WSName, lpw);

    FindUBUsingFFT alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MinD", "8.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaxD", "13.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Tolerance", "0.15"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Check that we set an oriented lattice
    TS_ASSERT(lpw->mutableSample().hasOrientedLattice());
    // Check that the UB matrix is the same as in TOPAZ_3007.mat
    OrientedLattice latt = lpw->mutableSample().getOrientedLattice();

    double correct_UB[] = {0.0122354,  0.00480056, 0.0860404,   -0.1165450, 0.00178145,
                           -0.0045884, -0.0273738, -0.08973560, -0.0252595};

    std::vector<double> UB_calculated = latt.getUB().getVector();

    for (size_t i = 0; i < 9; i++) {
      TS_ASSERT_DELTA(correct_UB[i], UB_calculated[i], 5e-4);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(WSName);
  }
};
