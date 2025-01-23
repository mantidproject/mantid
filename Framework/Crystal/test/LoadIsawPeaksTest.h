// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidCrystal/SaveIsawPeaks.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Crystal;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class LoadIsawPeaksTest : public CxxTest::TestSuite {
public:
  void test_Confidence() {
    LoadIsawPeaks alg;
    alg.initialize();
    alg.setPropertyValue("Filename", "TOPAZ_1241.integrate");

    Mantid::Kernel::FileDescriptor descriptor(alg.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(95, alg.confidence(descriptor));
  }

  void test_Init() {
    LoadIsawPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  /** Test for the older TOPAZ geometry. */
  void xtest_exec_TOPAZ_1241() {
    LoadIsawPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setPropertyValue("Filename", "TOPAZ_1241.integrate");
    alg.setPropertyValue("OutputWorkspace", "TOPAZ_1241");

    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve("TOPAZ_1241")));
    TS_ASSERT(ws);
    if (!ws)
      return;
    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 271);

    Peak p = ws->getPeaks()[0];
    TS_ASSERT_EQUALS(p.getRunNumber(), 1241)
    TS_ASSERT_DELTA(p.getH(), -3, 1e-4);
    TS_ASSERT_DELTA(p.getK(), 1, 1e-4);
    TS_ASSERT_DELTA(p.getL(), 1, 1e-4);
    TS_ASSERT_EQUALS(p.getBankName(), "bank1");
    TS_ASSERT_DELTA(p.getCol(), 34, 1e-4);
    TS_ASSERT_DELTA(p.getRow(), 232, 1e-4);
    TS_ASSERT_DELTA(p.getIntensity(), 8334.62, 0.01);
    TS_ASSERT_DELTA(p.getSigmaIntensity(), 97, 0.01);
    TS_ASSERT_DELTA(p.getBinCount(), 49, 0.01);

    TS_ASSERT_DELTA(p.getWavelength(), 1.757, 0.001);
    TS_ASSERT_DELTA(p.getL1(), 18.0, 1e-3);
    TS_ASSERT_DELTA(p.getL2(), 0.39801, 0.01);

    TS_ASSERT_DELTA(p.getDSpacing(), 4.3241, 0.1);
    TS_ASSERT_DELTA(ws->getPeaks()[30].getDSpacing(), 2.8410, 0.12);
    TS_ASSERT_DELTA(ws->getPeaks()[30].getL2(), 0.45, 0.01);
  }

  /* Test for the newer TOPAZ geometry */
  void test_exec_TOPAZ_2479() {
    LoadIsawPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setPropertyValue("Filename", "TOPAZ_2479.peaks");
    alg.setPropertyValue("OutputWorkspace", "TOPAZ_2479");

    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve("TOPAZ_2479")));
    TS_ASSERT(ws);
    if (!ws)
      return;
    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 46);

    Peak p = ws->getPeaks()[0];
    TS_ASSERT_EQUALS(p.getRunNumber(), 2479)
    TS_ASSERT_DELTA(p.getH(), -1, 1e-4);
    TS_ASSERT_DELTA(p.getK(), -2, 1e-4);
    TS_ASSERT_DELTA(p.getL(), -27, 1e-4);
    TS_ASSERT_EQUALS(p.getBankName(), "bank17");
    TS_ASSERT_DELTA(p.getCol(), 87, 1e-4);
    TS_ASSERT_DELTA(p.getRow(), 16, 1e-4);
    TS_ASSERT_DELTA(p.getIntensity(), 221.83, 0.01);
    TS_ASSERT_DELTA(p.getSigmaIntensity(), 15.02, 0.01);
    TS_ASSERT_DELTA(p.getBinCount(), 8, 0.01);
    TS_ASSERT_DELTA(p.getWavelength(), 0.761095, 0.001);
    TS_ASSERT_DELTA(p.getL1(), 18.0, 1e-3);
    TS_ASSERT_DELTA(p.getL2(), 0.461, 1e-3);
    TS_ASSERT_DELTA(p.getTOF(), 3560., 10.); // channel number is about TOF

    TS_ASSERT_DELTA(p.getDSpacing(), 0.4723, 0.001);
    TS_ASSERT_DELTA(ws->getPeaks()[1].getDSpacing(), 0.6425, 0.001);
    TS_ASSERT_DELTA(ws->getPeaks()[2].getDSpacing(), 0.8138, 0.001);

    // Now test the goniometer matrix
    Matrix<double> r1(3, 3, true);
    // First peak has 0,0,0 angles so identity matrix
    TS_ASSERT(p.getGoniometerMatrix().equals(r1, 1e-5));

    // Peak 3 is phi,chi,omega of 90,0,0; giving this matrix:
    Matrix<double> r2(3, 3, false);
    r2[0][2] = 1;
    r2[1][1] = 1;
    r2[2][0] = -1;
    TS_ASSERT(ws->getPeaks()[2].getGoniometerMatrix().equals(r2, 1e-5));
    const Goniometer &G = ws->mutableRun().getGoniometer();
    TS_ASSERT_EQUALS(G.getNumberAxes(), 3);
    TS_ASSERT_EQUALS(G.getAxis(2).name, "phi");
    TS_ASSERT_EQUALS(G.getAxis(1).name, "chi");
    TS_ASSERT_EQUALS(G.getAxis(0).name, "omega");
  }

  /* Test for the calibrated geometry */
  void test_exec_calibrated() {
    LoadIsawPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setPropertyValue("Filename", "calibrated.peaks");
    alg.setPropertyValue("OutputWorkspace", "calibrated");

    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve("calibrated")));
    TS_ASSERT(ws);
    if (!ws)
      return;
    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 14);

    Peak p = ws->getPeaks()[0];
    TS_ASSERT_EQUALS(p.getRunNumber(), 71907)
    TS_ASSERT_DELTA(p.getH(), 0, 1e-4);
    TS_ASSERT_DELTA(p.getK(), 0, 1e-4);
    TS_ASSERT_DELTA(p.getL(), 0, 1e-4);
    TS_ASSERT_EQUALS(p.getBankName(), "bank22");
    TS_ASSERT_DELTA(p.getCol(), 5, 1e-4);
    TS_ASSERT_DELTA(p.getRow(), 154, 1e-4);
    TS_ASSERT_DELTA(p.getIntensity(), 0, 0.01);
    TS_ASSERT_DELTA(p.getSigmaIntensity(), 0, 0.01);
    TS_ASSERT_DELTA(p.getBinCount(), 8, 53);
    TS_ASSERT_DELTA(p.getWavelength(), 0.893676, 0.001);
    TS_ASSERT_DELTA(p.getL1(), 20.0, 1e-3);
    TS_ASSERT_DELTA(p.getL2(), 2.51, 1e-3);
    TS_ASSERT_DELTA(p.getTOF(), 5085.05, 0.1); // channel number is about TOF

    TS_ASSERT_DELTA(p.getDSpacing(), 2.0360, 0.001);
    TS_ASSERT_DELTA(ws->getPeaks()[1].getDSpacing(), 2.3261, 0.001);
    TS_ASSERT_DELTA(ws->getPeaks()[2].getDSpacing(), 2.3329, 0.001);
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
        ws = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve("peaks")));
    TS_ASSERT(ws);
    if (!ws)
      return;
    TS_ASSERT_EQUALS(ws->getNumberPeaks(), 18);

    Peak p = ws->getPeaks()[0];
    TS_ASSERT_EQUALS(p.getRunNumber(), 24281)
    TS_ASSERT_DELTA(p.getH(), 1, 1e-4);
    TS_ASSERT_DELTA(p.getK(), -1, 1e-4);
    TS_ASSERT_DELTA(p.getL(), 2, 1e-4);
    TS_ASSERT_EQUALS(p.getBankName(), "bank19");
    TS_ASSERT_DELTA(p.getCol(), 45, 1e-4);
    TS_ASSERT_DELTA(p.getRow(), 56, 1e-4);
    TS_ASSERT_DELTA(p.getIntensity(), 0, 0.01);
    TS_ASSERT_DELTA(p.getSigmaIntensity(), 0, 0.01);
    TS_ASSERT_DELTA(p.getBinCount(), 4859, 1);
    TS_ASSERT_DELTA(p.getWavelength(), 2.534970, 0.001);
    TS_ASSERT_DELTA(p.getL1(), 18.04795, 1e-3);
    TS_ASSERT_DELTA(p.getL2(), 0.4626, 1e-3);
    TS_ASSERT_DELTA(p.getTOF(), 11861.32, 0.1); // channel number is about TOF

    TS_ASSERT_DELTA(p.getDSpacing(), 2.9288, 0.001);
    TS_ASSERT_DELTA(ws->getPeaks()[1].getDSpacing(), 2.4928, 0.001);
    TS_ASSERT_DELTA(ws->getPeaks()[2].getDSpacing(), 2.9677, 0.001);
  }
  void test_mod_skip_UB() {
    LoadIsawPeaks alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setPropertyValue("Filename", "TOPAZ_2479.peaks");
    alg.setPropertyValue("OutputWorkspace", "TOPAZ_2479");

    TS_ASSERT(alg.execute());
    TS_ASSERT(alg.isExecuted());

    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve("TOPAZ_2479")));
    TS_ASSERT(ws);

    ws->getPeak(0).setIntMNP(V3D(1, -1, 2));

    std::string outfile = "./SaveIsawModulated.peaks";
    SaveIsawPeaks save;
    TS_ASSERT_THROWS_NOTHING(save.initialize());
    TS_ASSERT(save.isInitialized());
    TS_ASSERT_THROWS_NOTHING(save.setProperty("InputWorkspace", "TOPAZ_2479"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Filename", outfile));
    TS_ASSERT_THROWS_NOTHING(save.execute());
    TS_ASSERT(save.isExecuted());

    LoadIsawPeaks load;
    TS_ASSERT_THROWS_NOTHING(load.initialize());
    TS_ASSERT(load.isInitialized());
    load.setPropertyValue("Filename", outfile);
    load.setPropertyValue("OutputWorkspace", "peaks");

    TS_ASSERT(load.execute());
    TS_ASSERT(load.isExecuted());
  }
};
