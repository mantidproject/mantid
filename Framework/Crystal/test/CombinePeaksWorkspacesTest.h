// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidCrystal/CombinePeaksWorkspaces.h"
#include "MantidCrystal/PredictFractionalPeaks.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Crystal::CombinePeaksWorkspaces;

class CombinePeaksWorkspacesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CombinePeaksWorkspacesTest *createSuite() { return new CombinePeaksWorkspacesTest(); }
  static void destroySuite(CombinePeaksWorkspacesTest *suite) { delete suite; }

  void test_init() {
    CombinePeaksWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_invalid_input() {
    CombinePeaksWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    // Tolerance has to be positive. Even if CombineMatchingPeaks is false!
    TS_ASSERT_THROWS(alg.setProperty("Tolerance", -1.0), const std::invalid_argument &)
  }

  void test_keep_all_peaks() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    PeaksWorkspace_sptr lhsWS = WorkspaceCreationHelper::createPeaksWorkspace(2);
    PeaksWorkspace_sptr rhsWS = WorkspaceCreationHelper::createPeaksWorkspace(3);

    // Name of the output workspace.
    std::string outWSName("CombinePeaksWorkspacesTest_OutputWS");

    CombinePeaksWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LHSWorkspace", lhsWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RHSWorkspace", rhsWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName))
    TS_ASSERT(alg.execute())

    // Retrieve the workspace from data service.
    IPeaksWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 5)
    TS_ASSERT_EQUALS(ws->getPeak(0).getQLabFrame(), ws->getPeak(2).getQLabFrame())
    TS_ASSERT_EQUALS(ws->getPeak(1).getQLabFrame(), ws->getPeak(3).getQLabFrame())
    TS_ASSERT_DELTA(ws->getPeak(4).getWavelength(), 2.5, 0.001)
    TS_ASSERT_EQUALS(ws->getInstrument()->baseInstrument(), lhsWS->getInstrument()->baseInstrument())

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_match_peaks_identical_workspaces() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    PeaksWorkspace_sptr inWS = WorkspaceCreationHelper::createPeaksWorkspace(2);

    // Name of the output workspace.
    std::string outWSName("CombinePeaksWorkspacesTest_OutputWS");

    CombinePeaksWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LHSWorkspace", inWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RHSWorkspace", inWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CombineMatchingPeaks", true))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName))
    TS_ASSERT(alg.execute())

    // Retrieve the workspace from data service.
    IPeaksWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 2)
    TS_ASSERT_EQUALS(ws->getPeak(0).getWavelength(), inWS->getPeak(0).getWavelength())
    TS_ASSERT_EQUALS(ws->getPeak(1).getWavelength(), inWS->getPeak(1).getWavelength())
    TS_ASSERT_EQUALS(ws->getInstrument()->baseInstrument(), inWS->getInstrument()->baseInstrument())

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_match_peaks_within_tolerance() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    PeaksWorkspace_sptr lhsWS = WorkspaceCreationHelper::createPeaksWorkspace(4);
    PeaksWorkspace_sptr rhsWS = WorkspaceCreationHelper::createPeaksWorkspace(4);

    // Slightly adjust the peaks in one of the workspaces
    auto &rhsPeaks = rhsWS->getPeaks();
    auto &lhsPeaks = lhsWS->getPeaks();

    // Need to change a couple of detector IDs so that I can get peaks with
    // larger |Q_z| than |Q_x|
    lhsPeaks[2].setDetectorID(50);
    lhsPeaks[3].setDetectorID(51);
    rhsPeaks[2].setDetectorID(50);
    rhsPeaks[3].setDetectorID(51);

    // And need to shift some peaks in one workspace to test the delta checking
    // This one will fails to match in x & z
    rhsPeaks[0].setWavelength(rhsPeaks[0].getWavelength() * 1.01);
    // This one matches in z but not in x
    rhsPeaks[1].setWavelength(rhsPeaks[1].getWavelength() * 1.02);
    // This one matches in x but not z
    rhsPeaks[2].setWavelength(rhsPeaks[2].getWavelength() * 1.0335);
    // This one will be matched (to lhsPeaks[0]) and will not appear in the
    // output
    rhsPeaks[3].setWavelength(rhsPeaks[3].getWavelength() * 1.04);

    // Name of the output workspace.
    std::string outWSName("CombinePeaksWorkspacesTest_OutputWS");

    CombinePeaksWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LHSWorkspace", lhsWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RHSWorkspace", rhsWS))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CombineMatchingPeaks", true))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Tolerance", 0.08145))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName))
    TS_ASSERT(alg.execute())

    // Retrieve the workspace from data service.
    IPeaksWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 7)
    TS_ASSERT_EQUALS(ws->getPeak(0).getQLabFrame(), lhsWS->getPeak(0).getQLabFrame())
    TS_ASSERT_EQUALS(ws->getPeak(1).getQLabFrame(), lhsWS->getPeak(1).getQLabFrame())
    TS_ASSERT_EQUALS(ws->getPeak(2).getQLabFrame(), lhsWS->getPeak(2).getQLabFrame())
    TS_ASSERT_EQUALS(ws->getPeak(3).getQLabFrame(), lhsWS->getPeak(3).getQLabFrame())
    TS_ASSERT_EQUALS(ws->getPeak(4).getQLabFrame(), rhsWS->getPeak(0).getQLabFrame())
    TS_ASSERT_EQUALS(ws->getPeak(5).getQLabFrame(), rhsWS->getPeak(1).getQLabFrame())
    TS_ASSERT_EQUALS(ws->getPeak(6).getQLabFrame(), rhsWS->getPeak(2).getQLabFrame())
    TS_ASSERT_EQUALS(ws->getInstrument()->baseInstrument(), lhsWS->getInstrument()->baseInstrument())

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_modulation_vectors_are_combined() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::Crystal;

    PeaksWorkspace_sptr peaksWs = WorkspaceCreationHelper::createPeaksWorkspace(3, true);

    Mantid::Crystal::PredictFractionalPeaks predictAlg;
    predictAlg.initialize();
    predictAlg.setProperty("Peaks", peaksWs);
    predictAlg.setProperty("ModVector1", "0.5, 0, 0.5");
    predictAlg.setProperty("FracPeaks", "frac_vec1");
    predictAlg.setProperty("MaxOrder", 1);
    predictAlg.execute();

    predictAlg.initialize();
    predictAlg.setProperty("Peaks", peaksWs);
    predictAlg.setProperty("ModVector1", "-0.5, -0.5, -0.5");
    predictAlg.setProperty("FracPeaks", "frac_vec2");
    predictAlg.setProperty("MaxOrder", 1);
    predictAlg.execute();

    CombinePeaksWorkspaces alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LHSWorkspace", "frac_vec1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RHSWorkspace", "frac_vec2"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "frac_vec_1and2"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    IPeaksWorkspace_const_sptr outWs;
    TS_ASSERT_THROWS_NOTHING(outWs = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>("frac_vec_1and2"));

    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(0)[0], 0.5);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(0)[1], 0);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(0)[2], 0.5);

    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(1)[0], -0.5);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(1)[1], -0.5);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(1)[2], -0.5);

    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(2)[0], 0);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(2)[1], 0);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(2)[2], 0);
  }

  void test_lhs_modulation_vectors_are_used_when_too_many() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::Crystal;

    PeaksWorkspace_sptr peaksWs = WorkspaceCreationHelper::createPeaksWorkspace(3, true);

    Mantid::Crystal::PredictFractionalPeaks predictAlg;
    predictAlg.initialize();
    predictAlg.setProperty("Peaks", peaksWs);
    predictAlg.setProperty("ModVector1", "0.5, 0, 0.5");
    predictAlg.setProperty("ModVector2", "0.5, 0, 0.5");
    predictAlg.setProperty("ModVector3", "0.5, 0, 0.5");
    predictAlg.setProperty("FracPeaks", "frac_vec1");
    predictAlg.setProperty("MaxOrder", 1);
    predictAlg.execute();

    predictAlg.initialize();
    predictAlg.setProperty("Peaks", peaksWs);
    predictAlg.setProperty("ModVector1", "-0.5, -0.5, -0.5");
    predictAlg.setProperty("FracPeaks", "frac_vec2");
    predictAlg.setProperty("MaxOrder", 1);
    predictAlg.execute();

    CombinePeaksWorkspaces alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LHSWorkspace", "frac_vec1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RHSWorkspace", "frac_vec2"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "frac_vec_1and2"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    IPeaksWorkspace_const_sptr outWs;
    TS_ASSERT_THROWS_NOTHING(outWs = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>("frac_vec_1and2"));

    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(0)[0], 0.5);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(0)[1], 0);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(0)[2], 0.5);

    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(1)[0], 0.5);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(1)[1], 0);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(1)[2], 0.5);

    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(2)[0], 0.5);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(2)[1], 0);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(2)[2], 0.5);
  }

  void test_duplicate_workspaces_are_not_combined() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::Crystal;

    PeaksWorkspace_sptr peaksWs = WorkspaceCreationHelper::createPeaksWorkspace(3, true);

    Mantid::Crystal::PredictFractionalPeaks predictAlg;
    predictAlg.initialize();
    predictAlg.setProperty("Peaks", peaksWs);
    predictAlg.setProperty("ModVector1", "0.5, 0, 0.5");
    predictAlg.setProperty("ModVector2", "0.5, 0, 0.5");
    predictAlg.setProperty("FracPeaks", "frac_vec1");
    predictAlg.setProperty("MaxOrder", 1);
    predictAlg.execute();

    predictAlg.initialize();
    predictAlg.setProperty("Peaks", peaksWs);
    predictAlg.setProperty("ModVector1", "0.5, 0, 0.5");
    predictAlg.setProperty("FracPeaks", "frac_vec2");
    predictAlg.setProperty("MaxOrder", 1);
    predictAlg.execute();

    CombinePeaksWorkspaces alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LHSWorkspace", "frac_vec1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RHSWorkspace", "frac_vec2"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "frac_vec_1and2"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    IPeaksWorkspace_const_sptr outWs;
    TS_ASSERT_THROWS_NOTHING(outWs = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>("frac_vec_1and2"));

    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(0)[0], 0.5);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(0)[1], 0);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(0)[2], 0.5);

    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(1)[0], 0.5);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(1)[1], 0);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(1)[2], 0.5);

    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(2)[0], 0);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(2)[1], 0);
    TS_ASSERT_EQUALS(outWs->sample().getOrientedLattice().getModVec(2)[2], 0);
  }

  void test_LeanElasticPeak() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    PeaksWorkspace_sptr ws1 = WorkspaceCreationHelper::createPeaksWorkspace(3);

    auto ws2 = std::make_shared<LeanElasticPeaksWorkspace>();
    ws2->addPeak(LeanElasticPeak(Mantid::Kernel::V3D(1, 0, 0), 1.));
    ws2->addPeak(LeanElasticPeak(Mantid::Kernel::V3D(0, 4, 0), 1.));

    auto ws3 = std::make_shared<LeanElasticPeaksWorkspace>();
    ws3->addPeak(LeanElasticPeak(Mantid::Kernel::V3D(2, 0, 0), 1.));
    ws3->addPeak(LeanElasticPeak(Mantid::Kernel::V3D(0, 4, 0), 1.));

    // Name of the output workspace.
    std::string outWSName("CombinePeaksWorkspacesTest_OutputWS");

    // LeanElasticPeak + LeanElasticPeak - no combine
    CombinePeaksWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LHSWorkspace", ws2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RHSWorkspace", ws3))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName))
    TS_ASSERT(alg.execute())

    // Retrieve the workspace from data service.
    LeanElasticPeaksWorkspace_const_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<LeanElasticPeaksWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 4)
    TS_ASSERT_EQUALS(ws->getPeak(1).getQSampleFrame(), ws->getPeak(3).getQSampleFrame())

    // LeanElasticPeak + LeanElasticPeak - combine
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LHSWorkspace", ws2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RHSWorkspace", ws3))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CombineMatchingPeaks", true))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Tolerance", 0.00001))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName))
    TS_ASSERT(alg.execute())

    // Retrieve the workspace from data service.
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<LeanElasticPeaksWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 3)

    // LeanElasticPeak + Peak
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LHSWorkspace", ws2))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RHSWorkspace", ws1))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName))
    TS_ASSERT(alg.execute())

    // Retrieve the workspace from data service.
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<LeanElasticPeaksWorkspace>(outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 5)

    // Peak + LeanElasticPeak - SHOULD FAIL TO EXECUTE
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LHSWorkspace", ws1))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RHSWorkspace", ws2))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outWSName))
    TS_ASSERT(!alg.execute())

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
};
