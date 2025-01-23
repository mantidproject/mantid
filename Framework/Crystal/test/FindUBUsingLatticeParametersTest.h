// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/DeleteTableRows.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidCrystal/FindUBUsingLatticeParameters.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace Mantid::Crystal;
using Mantid::Geometry::OrientedLattice;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;

class FindUBUsingLatticeParametersTest : public CxxTest::TestSuite {
public:
  static FindUBUsingLatticeParametersTest *createSuite() { return new FindUBUsingLatticeParametersTest(); }
  static void destroySuite(FindUBUsingLatticeParametersTest *suite) { delete suite; }

  FindUBUsingLatticeParametersTest() { m_ws = loadPeaksWorkspace(); }

  ~FindUBUsingLatticeParametersTest() {
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(m_ws->getName());
  }

  void test_Init() {
    FindUBUsingLatticeParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {

    FindUBUsingLatticeParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", m_ws->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("a", "14.131"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("b", "19.247"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("c", "8.606"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("alpha", "90.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("beta", "105.071"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("gamma", "90.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NumInitial", "15"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Tolerance", "0.12"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Check that we set an oriented lattice
    TS_ASSERT(m_ws->mutableSample().hasOrientedLattice());
    // Check that the UB matrix is the same as in TOPAZ_3007.mat
    OrientedLattice latt = m_ws->mutableSample().getOrientedLattice();

    double correct_UB[] = {0.04542050, 0.040619900, 0.0122354,  -0.00140347, -0.00318493,
                           -0.1165450, -0.05749760, 0.03223800, -0.0273738};

    std::vector<double> UB_calculated = latt.getUB().getVector();

    for (size_t i = 0; i < 9; i++) {
      TS_ASSERT_DELTA(correct_UB[i], UB_calculated[i], 5e-4);
    }

    TS_ASSERT_DELTA(latt.a(), 14.131, 5e-4);
    TS_ASSERT_DELTA(latt.b(), 19.247, 5e-4);
    TS_ASSERT_DELTA(latt.c(), 8.606, 5e-4);

    TS_ASSERT_DELTA(latt.alpha(), 90.0, 5e-1);
    TS_ASSERT_DELTA(latt.beta(), 105.071, 5e-1);
    TS_ASSERT_DELTA(latt.gamma(), 90.0, 5e-1);

    // Check errors
    TS_ASSERT_DELTA(latt.errora(), 0.0134, 5e-4);
    TS_ASSERT_DELTA(latt.errorb(), 0.0243, 5e-4);
    TS_ASSERT_DELTA(latt.errorc(), 0.0101, 5e-4);

    TS_ASSERT_DELTA(latt.erroralpha(), 0.0994, 5e-4);
    TS_ASSERT_DELTA(latt.errorbeta(), 0.0773, 5e-4);
    TS_ASSERT_DELTA(latt.errorgamma(), 0.0906, 5e-4);
  }

  void test_exec_LeanElasticPeak() {

    // Convert PeaksWorkspace to LeanElasticPeaksWorkspace
    auto lpw = std::make_shared<LeanElasticPeaksWorkspace>();
    for (auto peak : m_ws->getPeaks())
      lpw->addPeak(peak);
    AnalysisDataService::Instance().addOrReplace("leanpeaksWS", lpw);

    FindUBUsingLatticeParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", lpw->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("a", "14.131"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("b", "19.247"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("c", "8.606"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("alpha", "90.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("beta", "105.071"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("gamma", "90.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NumInitial", "15"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Tolerance", "0.12"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Check that we set an oriented lattice
    TS_ASSERT(lpw->mutableSample().hasOrientedLattice());
    // Check that the UB matrix is the same as in TOPAZ_3007.mat
    OrientedLattice latt = lpw->mutableSample().getOrientedLattice();

    double correct_UB[] = {0.04542050, 0.040619900, 0.0122354,  -0.00140347, -0.00318493,
                           -0.1165450, -0.05749760, 0.03223800, -0.0273738};

    std::vector<double> UB_calculated = latt.getUB().getVector();

    for (size_t i = 0; i < 9; i++) {
      TS_ASSERT_DELTA(correct_UB[i], UB_calculated[i], 5e-4);
    }

    TS_ASSERT_DELTA(latt.a(), 14.131, 5e-4);
    TS_ASSERT_DELTA(latt.b(), 19.247, 5e-4);
    TS_ASSERT_DELTA(latt.c(), 8.606, 5e-4);

    TS_ASSERT_DELTA(latt.alpha(), 90.0, 5e-1);
    TS_ASSERT_DELTA(latt.beta(), 105.071, 5e-1);
    TS_ASSERT_DELTA(latt.gamma(), 90.0, 5e-1);

    // Check errors
    TS_ASSERT_DELTA(latt.errora(), 0.0134, 5e-4);
    TS_ASSERT_DELTA(latt.errorb(), 0.0243, 5e-4);
    TS_ASSERT_DELTA(latt.errorc(), 0.0101, 5e-4);

    TS_ASSERT_DELTA(latt.erroralpha(), 0.0994, 5e-4);
    TS_ASSERT_DELTA(latt.errorbeta(), 0.0773, 5e-4);
    TS_ASSERT_DELTA(latt.errorgamma(), 0.0906, 5e-4);
  }

  void test_fixAll() {
    FindUBUsingLatticeParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", m_ws->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("a", "14.131"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("b", "19.247"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("c", "8.606"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("alpha", "90.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("beta", "105.071"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("gamma", "90.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NumInitial", "15"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Tolerance", "0.12"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FixParameters", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check that we set an oriented lattice
    TS_ASSERT(m_ws->mutableSample().hasOrientedLattice());
    // Check that the UB matrix is the same as in TOPAZ_3007.mat
    OrientedLattice latt = m_ws->mutableSample().getOrientedLattice();

    double correct_UB[] = {0.04542050, 0.040619900, 0.0127661,  -0.00198382, -0.00264404,
                           -0.1165450, -0.05749760, 0.03223800, -0.0257623};

    std::vector<double> UB_calculated = latt.getUB().getVector();

    for (size_t i = 0; i < 9; i++) {
      TS_ASSERT_DELTA(correct_UB[i], UB_calculated[i], 5e-4);
    }

    TS_ASSERT_DELTA(latt.a(), 14.131, 5e-10);
    TS_ASSERT_DELTA(latt.b(), 19.247, 5e-10);
    TS_ASSERT_DELTA(latt.c(), 8.606, 5e-10);

    TS_ASSERT_DELTA(latt.alpha(), 90.0, 5e-10);
    TS_ASSERT_DELTA(latt.beta(), 105.071, 5e-10);
    TS_ASSERT_DELTA(latt.gamma(), 90.0, 5e-10);
  }

  void test_smallNumberOfPeaks() {
    // Use a tiny set of 3 peaks - the minimum required
    /// to successfully find a UB matrix this checks the case that we still
    /// get a UB (although perhaps not a very good one).
    std::vector<size_t> rows;
    for (size_t i = 3; i < m_ws->rowCount(); ++i) {
      rows.emplace_back(i);
    }

    Mantid::DataHandling::DeleteTableRows removeRowAlg;
    removeRowAlg.initialize();
    removeRowAlg.setPropertyValue("TableWorkspace", m_ws->getName());
    removeRowAlg.setProperty("Rows", rows);
    removeRowAlg.execute();

    FindUBUsingLatticeParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", m_ws->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("a", "14.131"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("b", "19.247"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("c", "8.606"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("alpha", "90.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("beta", "105.071"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("gamma", "90.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NumInitial", "15"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Tolerance", "0.12"));
    //    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FixAll", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check that we set an oriented lattice
    TS_ASSERT(m_ws->mutableSample().hasOrientedLattice());
    // Check that the UB matrix is the same as in TOPAZ_3007.mat
    OrientedLattice latt = m_ws->mutableSample().getOrientedLattice();

    double correct_UB[] = {0.0450, 0.0407, 0.0127, -0.0008, -0.0044, -0.1158, -0.0584, 0.0307, -0.0242};

    std::vector<double> UB_calculated = latt.getUB().getVector();

    for (size_t i = 0; i < 9; i++) {
      TS_ASSERT_DELTA(correct_UB[i], UB_calculated[i], 1e-2);
    }
    TS_ASSERT_DELTA(latt.a(), 13.9520, 1e-2);
    TS_ASSERT_DELTA(latt.b(), 19.5145, 1e-2);
    TS_ASSERT_DELTA(latt.c(), 8.6566, 1e-2);
    TS_ASSERT_DELTA(latt.alpha(), 92.6267, 1e-2);
    TS_ASSERT_DELTA(latt.beta(), 103.7440, 1e-2);
    TS_ASSERT_DELTA(latt.gamma(), 90.0272, 1e-2);
  }

private:
  /*
   * Load a peaks workspace to use as input data
   */
  PeaksWorkspace_sptr loadPeaksWorkspace() const {
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
    return ws;
  }

  /// Input peaks workspace
  PeaksWorkspace_sptr m_ws;
};
