// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidTestHelpers/NexusTestHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {
/// static logger object
Mantid::Kernel::Logger g_log("LeanElasticPeaksWorkspaceTest");
} // namespace

class LeanElasticPeaksWorkspaceTest : public CxxTest::TestSuite {
private:
  class TestableLeanElasticPeaksWorkspace : public LeanElasticPeaksWorkspace {
  public:
    TestableLeanElasticPeaksWorkspace(const LeanElasticPeaksWorkspace &other) : LeanElasticPeaksWorkspace(other) {}
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LeanElasticPeaksWorkspaceTest *createSuite() { return new LeanElasticPeaksWorkspaceTest(); }
  static void destroySuite(LeanElasticPeaksWorkspaceTest *suite) { delete suite; }

  void test_defaultConstructor() {
    auto pw = std::make_shared<LeanElasticPeaksWorkspace>();
    LeanElasticPeak p(V3D(1, 0, 0), 3.0);
    pw->addPeak(p);

    TS_ASSERT_EQUALS(pw->columnCount(), 14);
    TS_ASSERT_EQUALS(pw->rowCount(), 1);
    TS_ASSERT_EQUALS(pw->getNumberPeaks(), 1);
    TS_ASSERT_DELTA(pw->getPeak(0).getWavelength(), 3.0, 1e-9);
  }

  void test_copyConstructor() {
    auto pw = std::make_shared<LeanElasticPeaksWorkspace>();
    LeanElasticPeak p(V3D(1, 0, 0), 3.0);
    pw->addPeak(p);

    auto pw2 = std::make_shared<TestableLeanElasticPeaksWorkspace>(*pw);
    TS_ASSERT_EQUALS(pw2->rowCount(), 1);
    TS_ASSERT_EQUALS(pw2->getNumberPeaks(), 1);
    TS_ASSERT_DELTA(pw2->getPeak(0).getWavelength(), 3.0, 1e-9);
  }

  void test_clone() {
    auto pw = std::make_shared<LeanElasticPeaksWorkspace>();
    LeanElasticPeak p(V3D(1, 0, 0), 3.0);
    pw->addPeak(p);

    auto pw2 = pw->clone();
    TS_ASSERT_EQUALS(pw2->rowCount(), 1);
    TS_ASSERT_EQUALS(pw2->getNumberPeaks(), 1);
    TS_ASSERT_DELTA(pw2->getPeak(0).getWavelength(), 3.0, 1e-9);
  }

  void test_createPeak() {
    auto pw = std::make_shared<LeanElasticPeaksWorkspace>();
    Mantid::Geometry::Goniometer goniometer;
    goniometer.pushAxis("axis1", 0, 1, 0);
    goniometer.setRotationAngle(0, 90);

    pw->mutableRun().setGoniometer(goniometer, false);

    // cannot create peak using q-lab
    TS_ASSERT_THROWS(pw->createPeak(V3D(1, 1, 0)), const Exception::NotImplementedError &)

    auto peak = pw->createPeakQSample(V3D(1, 1, 0));

    auto qSample = peak->getQSampleFrame();
    TS_ASSERT_DELTA(qSample.X(), 1, 1e-7)
    TS_ASSERT_DELTA(qSample.Y(), 1, 1e-7)
    TS_ASSERT_DELTA(qSample.Z(), 0, 1e-7)
    auto qLab = peak->getQLabFrame();
    TS_ASSERT_DELTA(qLab.X(), 0, 1e-7)
    TS_ASSERT_DELTA(qLab.Y(), 1, 1e-7)
    TS_ASSERT_DELTA(qLab.Z(), -1, 1e-7)
  }

  void test_createPeakHKL() {
    auto pw = std::make_shared<LeanElasticPeaksWorkspace>();
    Mantid::Geometry::Goniometer goniometer;
    goniometer.pushAxis("axis1", 0, 1, 0);
    goniometer.setRotationAngle(0, 90);

    pw->mutableSample().setOrientedLattice(std::make_unique<OrientedLattice>(5, 5, 5, 90, 90, 90));
    pw->mutableRun().setGoniometer(goniometer, false);
    auto peak = pw->createPeakHKL(V3D(1, 0, 0));

    TS_ASSERT_EQUALS(peak->getH(), 1)
    TS_ASSERT_EQUALS(peak->getK(), 0)
    TS_ASSERT_EQUALS(peak->getL(), 0)
    auto qSample = peak->getQSampleFrame();
    TS_ASSERT_DELTA(qSample.X(), 2 * M_PI / 5, 1e-7)
    TS_ASSERT_DELTA(qSample.Y(), 0, 1e-7)
    TS_ASSERT_DELTA(qSample.Z(), 0, 1e-7)
    auto qLab = peak->getQLabFrame();
    TS_ASSERT_DELTA(qLab.X(), 0, 1e-7)
    TS_ASSERT_DELTA(qLab.Y(), 0, 1e-7)
    TS_ASSERT_DELTA(qLab.Z(), -2 * M_PI / 5, 1e-7)
  }

  void test_addPeakSpecialCoordinate() {
    auto pw = std::make_shared<LeanElasticPeaksWorkspace>();
    Mantid::Geometry::Goniometer goniometer;
    goniometer.pushAxis("axis1", 0, 1, 0);
    goniometer.setRotationAngle(0, 90);

    pw->mutableSample().setOrientedLattice(std::make_unique<OrientedLattice>(5, 5, 5, 90, 90, 90));
    pw->mutableRun().setGoniometer(goniometer, false);

    TS_ASSERT_THROWS(pw->addPeak(V3D(1, 0, 0), Mantid::Kernel::QLab), const Exception::NotImplementedError &)

    pw->addPeak(V3D(1, 1, 0), Mantid::Kernel::QSample);
    pw->addPeak(V3D(1, 0, 0), Mantid::Kernel::HKL);

    TS_ASSERT_EQUALS(pw->getNumberPeaks(), 2);

    auto peak = pw->getPeak(0);
    TS_ASSERT_EQUALS(peak.getH(), 0)
    TS_ASSERT_EQUALS(peak.getK(), 0)
    TS_ASSERT_EQUALS(peak.getL(), 0)
    auto qSample = peak.getQSampleFrame();
    TS_ASSERT_DELTA(qSample.X(), 1, 1e-7)
    TS_ASSERT_DELTA(qSample.Y(), 1, 1e-7)
    TS_ASSERT_DELTA(qSample.Z(), 0, 1e-7)
    auto qLab = peak.getQLabFrame();
    TS_ASSERT_DELTA(qLab.X(), 0, 1e-7)
    TS_ASSERT_DELTA(qLab.Y(), 1, 1e-7)
    TS_ASSERT_DELTA(qLab.Z(), -1, 1e-7)

    peak = pw->getPeak(1);
    TS_ASSERT_EQUALS(peak.getH(), 1)
    TS_ASSERT_EQUALS(peak.getK(), 0)
    TS_ASSERT_EQUALS(peak.getL(), 0)
    qSample = peak.getQSampleFrame();
    TS_ASSERT_DELTA(qSample.X(), 2 * M_PI / 5, 1e-7)
    TS_ASSERT_DELTA(qSample.Y(), 0, 1e-7)
    TS_ASSERT_DELTA(qSample.Z(), 0, 1e-7)
    qLab = peak.getQLabFrame();
    TS_ASSERT_DELTA(qLab.X(), 0, 1e-7)
    TS_ASSERT_DELTA(qLab.Y(), 0, 1e-7)
    TS_ASSERT_DELTA(qLab.Z(), -2 * M_PI / 5, 1e-7)
  }
  void test_addRemovePeaks() {
    // build peaksworkspace (note number of peaks = 1)
    auto pw = std::make_shared<LeanElasticPeaksWorkspace>();

    // add peaks
    LeanElasticPeak p(V3D(1, 0, 0));
    LeanElasticPeak p2(V3D(0, 1, 0));
    LeanElasticPeak p3(V3D(0, 0, 1));
    pw->addPeak(p);
    pw->addPeak(p2);
    pw->addPeak(p3);

    TS_ASSERT_EQUALS(pw->getNumberPeaks(), 3);

    // number of peaks = 4, now remove 3
    std::vector<int> badPeaks{0, 2, 3};
    pw->removePeaks(std::move(badPeaks));
    TS_ASSERT_EQUALS(pw->getNumberPeaks(), 1);
  }

  void test_sort() {
    auto pw = std::make_shared<LeanElasticPeaksWorkspace>();
    LeanElasticPeak p0(V3D(1, 0, 0), 3.0);
    LeanElasticPeak p1(V3D(1, 0, 0), 4.0);
    LeanElasticPeak p2(V3D(1, 0, 0), 5.0);
    LeanElasticPeak p3(V3D(1, 1, 0), 3.0);
    LeanElasticPeak p4(V3D(2, 0, 0), 3.0);
    pw->addPeak(p0);
    pw->addPeak(p1);
    pw->addPeak(p2);
    pw->addPeak(p3);
    pw->addPeak(p4);

    std::vector<std::pair<std::string, bool>> criteria;
    // Sort by decending wavelength then decending dspacing
    criteria.emplace_back(std::pair<std::string, bool>("wavelength", false));
    criteria.emplace_back(std::pair<std::string, bool>("dspacing", false));
    pw->sort(criteria);
    TS_ASSERT_DELTA(pw->getPeak(0).getWavelength(), 5.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(0).getDSpacing(), 2 * M_PI, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(1).getWavelength(), 4.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(1).getDSpacing(), 2 * M_PI, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(2).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(2).getDSpacing(), 2 * M_PI, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(3).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(3).getDSpacing(), M_PI * M_SQRT2, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(4).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(4).getDSpacing(), M_PI, 1e-5);

    // Sort by wavelength ascending then decending dspacing
    criteria.clear();
    criteria.emplace_back(std::pair<std::string, bool>("wavelength", true));
    criteria.emplace_back(std::pair<std::string, bool>("dspacing", false));
    pw->sort(criteria);
    TS_ASSERT_DELTA(pw->getPeak(0).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(0).getDSpacing(), 2 * M_PI, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(1).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(1).getDSpacing(), M_PI * M_SQRT2, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(2).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(2).getDSpacing(), M_PI, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(3).getWavelength(), 4.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(3).getDSpacing(), 2 * M_PI, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(4).getWavelength(), 5.0, 1e-5);
    TS_ASSERT_DELTA(pw->getPeak(4).getDSpacing(), 2 * M_PI, 1e-5);
  }

  void test_saveToNexus() {
    // build peaksworkspace (note number of peaks = 1)
    auto lpws = std::make_shared<LeanElasticPeaksWorkspace>();
    // add peaks
    LeanElasticPeak p(V3D(1, 0, 0), 3.0);
    LeanElasticPeak p2(V3D(0, 1, 0), 4.0);
    LeanElasticPeak p3(V3D(0, 0, 1), 5.0);
    lpws->addPeak(p);
    lpws->addPeak(p2);
    lpws->addPeak(p3);

    // save to nexus
    NexusTestHelper nexusHelper(true);
    nexusHelper.createFile("testSaveLeanElasticPeaksWorkspace.nxs");
    lpws->saveNexus(nexusHelper.file.get());
    nexusHelper.reopenFile();

    // Verify that this test_entry has a peaks_workspace entry
    TS_ASSERT_THROWS_NOTHING(nexusHelper.file->openGroup("peaks_workspace", "NXentry"));

    // Check wavelengths
    TS_ASSERT_THROWS_NOTHING(nexusHelper.file->openData("column_7"));
    std::vector<double> waveLengths;
    TS_ASSERT_THROWS_NOTHING(nexusHelper.file->getData(waveLengths));
    nexusHelper.file->closeData();
    TS_ASSERT_EQUALS(waveLengths.size(), 3);
    TS_ASSERT_DELTA(waveLengths[0], 3.0, 1e-5);
    TS_ASSERT_DELTA(waveLengths[1], 4.0, 1e-5);
    TS_ASSERT_DELTA(waveLengths[2], 5.0, 1e-5);
  }
};
