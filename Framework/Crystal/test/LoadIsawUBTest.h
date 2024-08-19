// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class LoadIsawUBTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    LoadIsawUB alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Fake output WS
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace("LoadIsawUBTest_ws", ws);

    LoadIsawUB alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "TOPAZ_3007.mat"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "LoadIsawUBTest_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT(ws);
    if (!ws)
      return;

    // Check the results
    OrientedLattice latt;
    TS_ASSERT_THROWS_NOTHING(latt = ws->mutableSample().getOrientedLattice());
    TS_ASSERT_DELTA(latt.a(), 14.1526, 1e-4);
    TS_ASSERT_DELTA(latt.b(), 19.2903, 1e-4);
    TS_ASSERT_DELTA(latt.c(), 8.5813, 1e-4);
    TS_ASSERT_DELTA(latt.alpha(), 90.0000, 1e-4);
    TS_ASSERT_DELTA(latt.beta(), 105.0738, 1e-4);
    TS_ASSERT_DELTA(latt.gamma(), 90.0000, 1e-4);

    Matrix<double> UB = latt.getUB();
    TS_ASSERT_EQUALS(UB.numRows(), 3);
    TS_ASSERT_EQUALS(UB.numCols(), 3);
    TS_ASSERT_DELTA(UB[0][0], -0.0453,
                    1e-4); // (Values were taken from the result, for consistency)
    TS_ASSERT_DELTA(UB[1][0], 0.0013, 1e-4);
    TS_ASSERT_DELTA(UB[2][2], 0.0273, 1e-4);

    AnalysisDataService::Instance().remove("LoadIsawUBTest_ws");
  }

  /**
# Raw data
LoadEventNexus(Filename="/home/8oz/data/TOPAZ_3007_event.nxs",OutputWorkspace="TOPAZ_3007",FilterByTime_Stop="1500",SingleBankPixelsOnly="0",CompressTolerance="0.050000000000000003")

# The found peaks
LoadIsawPeaks(Filename="/home/8oz/Code/Mantid/Test/AutoTestData/TOPAZ_3007.peaks",OutputWorkspace="TOPAZ_3007_peaks")

SortEvents(InputWorkspace="TOPAZ_3007")
LoadIsawUB(InputWorkspace="TOPAZ_3007",Filename="/home/8oz/Code/Mantid/Test/AutoTestData/TOPAZ_3007.mat")
PredictPeaks(InputWorkspace="TOPAZ_3007",HKLPeaksWorkspace="TOPAZ_3007_peaks",OutputWorkspace="peaks")
MaskPeaksWorkspace("TOPAZ_3007", "peaks")
   *
   */
  void test_integration() {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspaceBinned(10, 20);
    PeaksWorkspace_sptr pw;
    AnalysisDataService::Instance().addOrReplace("TOPAZ_3007", ws);

    FrameworkManager::Instance().exec("LoadInstrument", 6, "Workspace", "TOPAZ_3007", "Filename",
                                      "unit_testing/MINITOPAZ_Definition.xml", "RewriteSpectraMap", "True");

    // Match the goniometer angles
    WorkspaceCreationHelper::setGoniometer(ws, 86.92, 135.00, -105.66);
    // WorkspaceCreationHelper::SetGoniometer(ws, 0, 0, 0);

    // Load the .mat file into it
    FrameworkManager::Instance().exec("LoadIsawUB", 4, "Filename", "TOPAZ_3007.mat", "InputWorkspace", "TOPAZ_3007");

    // Load the .mat file into it
    FrameworkManager::Instance().exec("PredictPeaks", 4, "InputWorkspace", "TOPAZ_3007", "OutputWorkspace",
                                      "peaks_predicted");

    pw = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve("peaks_predicted"));

    TS_ASSERT(pw);
    if (!pw)
      return;

    TS_ASSERT_EQUALS(pw->getNumberPeaks(), 220);
  }

  void test_md() {
    // Create an MDHisto workspace with several ExperimentInfo
    MDHistoWorkspace_sptr histo = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 5, 10.0, 1.0);
    for (int i = 0; i < 4; i++) {
      ExperimentInfo_sptr ei = std::make_shared<ExperimentInfo>();
      histo->addExperimentInfo(ei);
    }
    TS_ASSERT(histo);
    TS_ASSERT_EQUALS(histo->getNumExperimentInfo(), 5);
    AnalysisDataService::Instance().addOrReplace("LoadIsawUBTest_MD", histo);

    // Run the algorithm
    LoadIsawUB alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "TOPAZ_3007.mat"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "LoadIsawUBTest_MD"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Check the results
    OrientedLattice latt;
    for (uint16_t i = 0; i < 5; i++) {
      TS_ASSERT_THROWS_NOTHING(latt = histo->getExperimentInfo(i)->sample().getOrientedLattice());
      TS_ASSERT_DELTA(latt.a(), 14.1526, 1e-4);
      TS_ASSERT_DELTA(latt.b(), 19.2903, 1e-4);
      TS_ASSERT_DELTA(latt.c(), 8.5813, 1e-4);
      TS_ASSERT_DELTA(latt.alpha(), 90.0000, 1e-4);
      TS_ASSERT_DELTA(latt.beta(), 105.0738, 1e-4);
      TS_ASSERT_DELTA(latt.gamma(), 90.0000, 1e-4);
    }
    AnalysisDataService::Instance().remove("LoadIsawUBTest_MD");
  }
};
