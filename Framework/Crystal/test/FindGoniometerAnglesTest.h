// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/FindGoniometerAngles.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/V3D.h"

using Mantid::Crystal::FindGoniometerAngles;

class FindGoniometerAnglesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindGoniometerAnglesTest *createSuite() { return new FindGoniometerAnglesTest(); }
  static void destroySuite(FindGoniometerAnglesTest *suite) { delete suite; }

  auto create_goniometer(const double phi, const double chi, const double omega) {
    Mantid::Geometry::Goniometer g;
    g.pushAxis("omega", 0., 1., 0., omega);
    g.pushAxis("chi", 0., 0., 1., chi);
    g.pushAxis("phi", 0., 1., 0., phi);
    return g;
  }

  auto get_ws_angles(const Mantid::DataObjects::PeaksWorkspace_sptr ws) {
    auto g = ws->run().getGoniometer();
    return g.getEulerAngles("YZY");
  }

  auto get_peak_angles(const Mantid::DataObjects::Peak p) {
    return Mantid::Kernel::Quat(p.getGoniometerMatrix()).getEulerAngles("YZY");
  }
  void test_Init() {
    FindGoniometerAngles alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void runTest(double delta_phi, double delta_chi, double delta_omega, double max_angle = 5., bool apply = false,
               bool expect_failure = false) {
    /* This test works by using PredictPeaks to create a peaks workspace using the goniometer and lattice provided.
     * After which a new peaks workspace with everything the same except the goniometer is created. The q_lab value from
     * the first "correct" peaks workspace is copied to the new "wrong" one. FindGoniometerAngles is then use to get
     * back the correct goniometer.
     */

    // first create a workspace with a particular goniometer and lattice
    const double phi{15.};
    const double chi{30.};
    const double omega{45.};

    Mantid::API::MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    Mantid::Geometry::Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 200);
    inWS->setInstrument(inst);
    inWS->mutableSample().setOrientedLattice(
        std::make_unique<Mantid::Geometry::OrientedLattice>(14.1526, 19.2903, 8.5813, 90, 105.074, 90));

    inWS->mutableRun().setGoniometer(create_goniometer(phi, chi, omega), false);

    // create peakworkspace using predict peaks
    auto predictPeaks = Mantid::API::AlgorithmFactory::Instance().create("PredictPeaks", 1);
    predictPeaks->initialize();
    predictPeaks->setProperty("InputWorkspace", inWS);
    predictPeaks->setProperty("OutputWorkspace", "FindGoniometerAnglesTestPeaks");
    predictPeaks->execute();

    auto peaksWS = std::dynamic_pointer_cast<Mantid::DataObjects::PeaksWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve("FindGoniometerAnglesTestPeaks"));

    // now create new peaks workspace with same q-lab but slightly wrong goniometer
    auto pw = std::make_shared<Mantid::DataObjects::PeaksWorkspace>();
    pw->setInstrument(inst);
    pw->mutableSample().setOrientedLattice(
        std::make_unique<Mantid::Geometry::OrientedLattice>(14.1526, 19.2903, 8.5813, 90, 105.074, 90));

    pw->mutableRun().setGoniometer(create_goniometer(phi + delta_phi, chi + delta_chi, omega + delta_omega), false);

    // copy peaks by their QLab so that a new q_sample is calculated with the different goniometer
    for (int i{0}; i < peaksWS->getNumberPeaks(); i++) {
      pw->addPeak(peaksWS->getPeak(i).getQLabFrame(), Mantid::Kernel::QLab);
    }

    // check that the goniometer angles are set correctly on the workspace and the peaks
    auto angles = get_ws_angles(pw);
    TS_ASSERT_DELTA(phi + delta_phi, angles[2], 5e-3)
    TS_ASSERT_DELTA(chi + delta_chi, angles[1], 5e-3)
    TS_ASSERT_DELTA(omega + delta_omega, angles[0], 5e-3)

    auto peak_angles = get_peak_angles(pw->getPeak(0));
    TS_ASSERT_DELTA(phi + delta_phi, peak_angles[2], 5e-3)
    TS_ASSERT_DELTA(chi + delta_chi, peak_angles[1], 5e-3)
    TS_ASSERT_DELTA(omega + delta_omega, peak_angles[0], 5e-3)

    // see if FindGoniometerAngles can get the origonal goniometer back
    FindGoniometerAngles alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeaksWorkspace", pw));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Apply", apply));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxAngle", max_angle));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // check that we got back what we expected
    if (expect_failure) {
      TS_ASSERT(abs(phi - static_cast<double>(alg.getProperty("Phi"))) > 1)
      TS_ASSERT(abs(chi - static_cast<double>(alg.getProperty("Chi"))) > 1)
      TS_ASSERT(abs(omega - static_cast<double>(alg.getProperty("Omega"))) > 1)
    } else {
      TS_ASSERT_DELTA(phi, static_cast<double>(alg.getProperty("Phi")), 5e-3)
      TS_ASSERT_DELTA(chi, static_cast<double>(alg.getProperty("Chi")), 5e-3)
      TS_ASSERT_DELTA(omega, static_cast<double>(alg.getProperty("Omega")), 5e-3)
    }

    // check goniometer angles after running
    if (apply) {
      // we expect the goniometer to be updated on the peaks workspace and the peaks
      auto angles = get_ws_angles(pw);
      TS_ASSERT_DELTA(phi, angles[2], 5e-3)
      TS_ASSERT_DELTA(chi, angles[1], 5e-3)
      TS_ASSERT_DELTA(omega, angles[0], 5e-3)

      auto peak_angles = get_peak_angles(pw->getPeak(0));
      TS_ASSERT_DELTA(phi, peak_angles[2], 5e-3)
      TS_ASSERT_DELTA(chi, peak_angles[1], 5e-3)
      TS_ASSERT_DELTA(omega, peak_angles[0], 5e-3)
    } else {
      // we expect the goniometer to NOT be updated on the peaks workspace and the peaks
      auto angles = get_ws_angles(pw);
      TS_ASSERT_DELTA(phi + delta_phi, angles[2], 5e-3)
      TS_ASSERT_DELTA(chi + delta_chi, angles[1], 5e-3)
      TS_ASSERT_DELTA(omega + delta_omega, angles[0], 5e-3)

      auto peak_angles = get_peak_angles(pw->getPeak(0));
      TS_ASSERT_DELTA(phi + delta_phi, peak_angles[2], 5e-3)
      TS_ASSERT_DELTA(chi + delta_chi, peak_angles[1], 5e-3)
      TS_ASSERT_DELTA(omega + delta_omega, peak_angles[0], 5e-3)
    }
  }

  void test_phi() { runTest(4., 0., 0.); }
  void test_phi_apply() { runTest(4., 0., 0., 5., true); }
  void test_chi() { runTest(0., 4., 0.); }
  void test_chi_apply() { runTest(0., 4., 0., 5., true); }
  void test_omega() { runTest(0., 0., 4.); }
  void test_omega_apply() { runTest(0., 0., 4., 5., true); }
  void test_all() { runTest(-2., 2., 2.); }
  void test_all_apply() { runTest(-2., 2., 2., 5., true); }
  void test_all_large_error() { runTest(30., 28., -27., 35); }
  void test_all_large_error2() { runTest(-27., 12., -32., 35); }

  // error in goniometer is larger than max_angle
  void test_failure() { runTest(-7., 8., -7., 5, false, true); }
};
