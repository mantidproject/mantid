// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/AddPeakHKL.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

using Mantid::Crystal::AddPeakHKL;
using Mantid::Geometry::IPeak;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class AddPeakHKLTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AddPeakHKLTest *createSuite() { return new AddPeakHKLTest(); }
  static void destroySuite(AddPeakHKLTest *suite) { delete suite; }

  void test_Init() {
    AddPeakHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_hkl_validation() {
    AddPeakHKL alg;
    alg.initialize();
    std::vector<double> hkl_bad(4); // Too big!
    TS_ASSERT_THROWS(alg.setProperty("HKL", hkl_bad), std::invalid_argument &);

    std::vector<double> hkl_good(3, 0); // Right size.
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("HKL", hkl_good));
  }

  void test_exec() {
    // Create simple fictional instrument
    const V3D source(0, 0, 0);
    const V3D sample(15, 0, 0);
    const V3D detectorPos(20, 5, 0);
    const V3D beam1 = sample - source;
    const V3D beam2 = detectorPos - sample;
    auto minimalInstrument = ComponentCreationHelper::createMinimalInstrument(source, sample, detectorPos);

    // Derive distances and angles
    const double l1 = beam1.norm();
    const double l2 = beam2.norm();
    const V3D qLabDir = (beam1 / l1) - (beam2 / l2);

    const double microSecsInSec = 1e6;

    // Derive QLab for diffraction
    const double wavenumber_in_angstrom_times_tof_in_microsec =
        (Mantid::PhysicalConstants::NeutronMass * (l1 + l2) * 1e-10 * microSecsInSec) /
        Mantid::PhysicalConstants::h_bar;
    V3D qLab = qLabDir * wavenumber_in_angstrom_times_tof_in_microsec;

    Mantid::Geometry::Goniometer goniometer; // identity
    V3D hkl = qLab / (2 * M_PI);             // Given our settings above, this is the
                                             // simplified relationship between qLab and
                                             // hkl.

    // Now create a peaks workspace around the simple fictional instrument
    PeaksWorkspace_sptr ws = std::make_shared<PeaksWorkspace>();
    ws->setInstrument(minimalInstrument);
    ws->mutableSample().setOrientedLattice(std::make_unique<Mantid::Geometry::OrientedLattice>(1, 1, 1, 90, 90, 90));
    ws->mutableRun().setGoniometer(goniometer, false);

    AddPeakHKL alg;
    alg.setChild(true);
    alg.initialize();
    std::vector<double> hklVec;
    hklVec.emplace_back(hkl.X());
    hklVec.emplace_back(hkl.Y());
    hklVec.emplace_back(hkl.Z());
    alg.setProperty("HKL", hklVec);
    alg.setProperty("Workspace", ws);
    alg.execute();
    IPeaksWorkspace_sptr ws_out = alg.getProperty("Workspace");

    // Get the peak just added.
    auto peak = dynamic_cast<const Peak &>(ws_out->getPeak(0));
    /*
     Now we check we have made a self - consistent peak
     */
    TSM_ASSERT_EQUALS("New peak should have HKL we demanded.", hkl, peak.getHKL());
    TSM_ASSERT_EQUALS("New peak should have QLab we expected.", qLab, peak.getQLabFrame());
    TSM_ASSERT_EQUALS("QSample and QLab should be identical given the identity "
                      "goniometer settings.",
                      peak.getQLabFrame(), peak.getQSampleFrame());
    auto detector = peak.getDetector();
    TSM_ASSERT("No detector", detector);
    TSM_ASSERT_EQUALS("This detector id does not match what we expect from the "
                      "instrument definition",
                      1, detector->getID());
    TSM_ASSERT_EQUALS("This detector position is wrong", detectorPos, detector->getPos());
    TSM_ASSERT_EQUALS("Goniometer has not been set properly", goniometer.getR(), peak.getGoniometerMatrix());
  }
};
