// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_
#define MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LogManager.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/NexusTestHelper.h"
#include "PropertyManagerHelper.h"
#include <cmath>
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <stdio.h>

#include <Poco/File.h>

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class PeaksWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeaksWorkspaceTest *createSuite() { return new PeaksWorkspaceTest(); }
  static void destroySuite(PeaksWorkspaceTest *suite) { delete suite; }

  /** Build a test PeaksWorkspace with one peak (others peaks can be added)
   *
   * @return PeaksWorkspace
   */
  PeaksWorkspace_sptr buildPW() {
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
    inst->setName("SillyInstrument");
    auto pw = PeaksWorkspace_sptr(new PeaksWorkspace);
    pw->setInstrument(inst);
    std::string val = "value";
    pw->mutableRun().addProperty("TestProp", val);
    Peak p(inst, 1, 3.0);
    pw->addPeak(p);
    return pw;
  }

  /** Check that the PeaksWorkspace build by buildPW() is correct */
  void checkPW(const PeaksWorkspace &pw) {
    TS_ASSERT_EQUALS(pw.columnCount(), 18);
    TS_ASSERT_EQUALS(pw.rowCount(), 1);
    TS_ASSERT_EQUALS(pw.getNumberPeaks(), 1);
    if (pw.getNumberPeaks() != 1)
      return;
    TS_ASSERT_DELTA(pw.getPeak(0).getWavelength(), 3.0, 1e-4);
    // Experiment info stuff got copied
    TS_ASSERT_EQUALS(pw.getInstrument()->getName(), "SillyInstrument");
    TS_ASSERT(pw.run().hasProperty("TestProp"));
  }

  void test_defaultConstructor() {
    auto pw = buildPW();
    checkPW(*pw);
  }

  class TestablePeaksWorkspace : public PeaksWorkspace {
  public:
    TestablePeaksWorkspace(const PeaksWorkspace &other)
        : PeaksWorkspace(other) {}
  };

  void test_copyConstructor() {
    auto pw = buildPW();
    auto pw2 = PeaksWorkspace_sptr(new TestablePeaksWorkspace(*pw));
    checkPW(*pw2);
  }

  void test_clone() {
    auto pw = buildPW();
    auto pw2 = pw->clone();
    checkPW(*pw2);
  }

  void test_sort() {
    auto pw = buildPW();
    Instrument_const_sptr inst = pw->getInstrument();
    Peak p0 = Peak(pw->getPeak(0)); // Peak(inst, 1, 3.0)
    Peak p1(inst, 1, 4.0);
    Peak p2(inst, 1, 5.0);
    Peak p3(inst, 2, 3.0);
    Peak p4(inst, 3, 3.0);
    pw->addPeak(p1);
    pw->addPeak(p2);
    pw->addPeak(p3);
    pw->addPeak(p4);

    std::vector<std::pair<std::string, bool>> criteria;
    // Sort by detector ID then descending wavelength
    criteria.push_back(std::pair<std::string, bool>("detid", true));
    criteria.push_back(std::pair<std::string, bool>("wavelength", false));
    pw->sort(criteria);
    TS_ASSERT_EQUALS(pw->getPeak(0).getDetectorID(), 1);
    TS_ASSERT_DELTA(pw->getPeak(0).getWavelength(), 5.0, 1e-5);
    TS_ASSERT_EQUALS(pw->getPeak(1).getDetectorID(), 1);
    TS_ASSERT_DELTA(pw->getPeak(1).getWavelength(), 4.0, 1e-5);
    TS_ASSERT_EQUALS(pw->getPeak(2).getDetectorID(), 1);
    TS_ASSERT_DELTA(pw->getPeak(2).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS(pw->getPeak(3).getDetectorID(), 2);
    TS_ASSERT_DELTA(pw->getPeak(3).getWavelength(), 3.0, 1e-5);

    // Sort by wavelength ascending then detID
    criteria.clear();
    criteria.push_back(std::pair<std::string, bool>("wavelength", true));
    criteria.push_back(std::pair<std::string, bool>("detid", true));
    pw->sort(criteria);
    TS_ASSERT_EQUALS(pw->getPeak(0).getDetectorID(), 1);
    TS_ASSERT_DELTA(pw->getPeak(0).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS(pw->getPeak(1).getDetectorID(), 2);
    TS_ASSERT_DELTA(pw->getPeak(1).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS(pw->getPeak(2).getDetectorID(), 3);
    TS_ASSERT_DELTA(pw->getPeak(2).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS(pw->getPeak(3).getDetectorID(), 1);
    TS_ASSERT_DELTA(pw->getPeak(3).getWavelength(), 4.0, 1e-5);
    TS_ASSERT_EQUALS(pw->getPeak(4).getDetectorID(), 1);
    TS_ASSERT_DELTA(pw->getPeak(4).getWavelength(), 5.0, 1e-5);
  }

  void test_Save_Unmodified_PeaksWorkspace_Nexus() {

    const std::string filename =
        "test_Save_Unmodified_PeaksWorkspace_Nexus.nxs";
    auto testPWS = createSaveTestPeaksWorkspace();
    NexusTestHelper nexusHelper(true);
    nexusHelper.createFile("testSavePeaksWorkspace.nxs");

    testPWS->saveNexus(nexusHelper.file.get());
    nexusHelper.reopenFile();

    // Verify that this test_entry has a peaks_workspace entry
    TS_ASSERT_THROWS_NOTHING(
        nexusHelper.file->openGroup("peaks_workspace", "NXentry"));

    // Check detector IDs
    TS_ASSERT_THROWS_NOTHING(nexusHelper.file->openData("column_1"));
    std::string columnName;
    TS_ASSERT_THROWS_NOTHING(nexusHelper.file->getAttr("name", columnName));
    TS_ASSERT_EQUALS(columnName, "Detector ID");
    std::vector<int> detIDs;
    TS_ASSERT_THROWS_NOTHING(nexusHelper.file->getData(detIDs));
    nexusHelper.file->closeData();
    TS_ASSERT_EQUALS(detIDs.size(), 5); // We have 5 detectors
    if (detIDs.size() >= 5) {
      TS_ASSERT_EQUALS(detIDs[0], 1);
      TS_ASSERT_EQUALS(detIDs[1], 10);
      TS_ASSERT_EQUALS(detIDs[2], 10);
      TS_ASSERT_EQUALS(detIDs[3], 20);
      TS_ASSERT_EQUALS(detIDs[4], 50);
    }

    // Check wavelengths
    TS_ASSERT_THROWS_NOTHING(nexusHelper.file->openData("column_10"));
    std::vector<double> waveLengths;
    TS_ASSERT_THROWS_NOTHING(nexusHelper.file->getData(waveLengths));
    nexusHelper.file->closeData();
    TS_ASSERT_EQUALS(waveLengths.size(), 5); // We have 5 wave lengths
    if (waveLengths.size() >= 5) {
      TS_ASSERT_DELTA(waveLengths[0], 3.0, 1e-5);
      TS_ASSERT_DELTA(waveLengths[1], 4.0, 1e-5);
      TS_ASSERT_DELTA(waveLengths[2], 5.0, 1e-5);
      TS_ASSERT_DELTA(waveLengths[3], 3.0, 1e-5);
      TS_ASSERT_DELTA(waveLengths[4], 3.0, 1e-5);
    }
  }

  void test_getSetLogAccess() {
    bool trueSwitch(true);
    auto pw = buildPW();

    LogManager_const_sptr props = pw->getLogs();
    std::string existingVal;

    TS_ASSERT_THROWS_NOTHING(
        existingVal = props->getPropertyValueAsType<std::string>("TestProp"));
    TS_ASSERT_EQUALS("value", existingVal);

    // define local scope;
    if (trueSwitch) {
      // get mutable pointer to existing values;
      LogManager_sptr mprops = pw->logs();

      TS_ASSERT_THROWS_NOTHING(
          mprops->addProperty<std::string>("TestProp2", "value2"));

      TS_ASSERT(mprops->hasProperty("TestProp2"));
      TS_ASSERT(!props->hasProperty("TestProp2"));
      TS_ASSERT(pw->run().hasProperty("TestProp2"));
    }
    // nothing terrible happened and workspace still have this property
    TS_ASSERT(pw->run().hasProperty("TestProp2"));

    auto pw1 = pw->clone();
    if (trueSwitch) {
      // get mutable pointer to existing values, which would be taken from the
      // cash
      LogManager_sptr mprops1 = pw->logs();
      // and in ideal world this should cause CowPtr to diverge but it does not
      TS_ASSERT_THROWS_NOTHING(
          mprops1->addProperty<std::string>("TestProp1-3", "value1-3"));
      TS_ASSERT(mprops1->hasProperty("TestProp1-3"));
      // The changes to pw should not affect pw1
      TS_ASSERT(pw->run().hasProperty("TestProp1-3"));
      TS_ASSERT(!pw1->run().hasProperty("TestProp1-3"));
    }
    TS_ASSERT(!pw1->run().hasProperty("TestProp1-3"));
    if (trueSwitch) {
      // but this will cause it to diverge
      LogManager_sptr mprops2 = pw1->logs();
      // and this  causes CowPtr to diverge
      TS_ASSERT_THROWS_NOTHING(
          mprops2->addProperty<std::string>("TestProp2-3", "value2-3"));
      TS_ASSERT(mprops2->hasProperty("TestProp2-3"));
      TS_ASSERT(!pw->run().hasProperty("TestProp2-3"));
      TS_ASSERT(pw1->run().hasProperty("TestProp2-3"));
    }
  }

  void test_hasIntegratedPeaks_without_property() {
    PeaksWorkspace ws;
    TSM_ASSERT(
        "Should not indicate that there are integrated peaks without property.",
        !ws.hasIntegratedPeaks());
  }

  void test_hasIntegratedPeaks_with_property_when_false() {
    PeaksWorkspace ws;
    bool hasIntegratedPeaks(false);
    ws.mutableRun().addProperty("PeaksIntegrated", hasIntegratedPeaks);
    TS_ASSERT_EQUALS(hasIntegratedPeaks, ws.hasIntegratedPeaks());
  }

  void test_hasIntegratedPeaks_with_property_when_true() {
    PeaksWorkspace ws;
    bool hasIntegratedPeaks(true);
    ws.mutableRun().addProperty("PeaksIntegrated", hasIntegratedPeaks);
    TS_ASSERT_EQUALS(hasIntegratedPeaks, ws.hasIntegratedPeaks());
  }

  void
  test_createDetectorTable_With_SinglePeak_And_Centre_Det_Has_Single_Row() {
    auto pw = buildPW(); // single peak with single detector
    auto detTable = pw->createDetectorTable();
    TSM_ASSERT("No table has been created", detTable);
    if (!detTable)
      return;
    check_Detector_Table_Metadata(*detTable, 1);

    auto column0 = detTable->getColumn(0);
    auto column1 = detTable->getColumn(1);
    // Contents
    TS_ASSERT_EQUALS(0, column0->cell<int>(0));
    TS_ASSERT_EQUALS(1, column1->cell<int>(0));
  }

  void
  test_createDetectorTable_With_SinglePeak_And_Multiple_Det_Has_Same_Num_Rows_As_Dets() {
    auto pw = buildPW(); // 1 peaks each with single detector
    // Add a detector to the peak
    Mantid::Geometry::IPeak &ipeak = pw->getPeak(0);
    auto &peak = static_cast<Peak &>(ipeak);
    peak.addContributingDetID(2);
    peak.addContributingDetID(3);

    auto detTable = pw->createDetectorTable();
    TSM_ASSERT("No table has been created", detTable);
    if (!detTable)
      return;
    check_Detector_Table_Metadata(*detTable, 3);

    auto column0 = detTable->getColumn(0);
    auto column1 = detTable->getColumn(1);
    // Contents
    // Peak 1
    TS_ASSERT_EQUALS(0, column0->cell<int>(0)); // Index 0
    TS_ASSERT_EQUALS(1, column1->cell<int>(0)); // Id 1
    TS_ASSERT_EQUALS(0, column0->cell<int>(1)); // Index 0
    TS_ASSERT_EQUALS(2, column1->cell<int>(1)); // Id 2
    TS_ASSERT_EQUALS(0, column0->cell<int>(2)); // Index 0
    TS_ASSERT_EQUALS(3, column1->cell<int>(2)); // Id 3
  }

  void test_createDetectorTable_With_Many_Peaks_And_Multiple_Dets() {
    auto pw =
        createSaveTestPeaksWorkspace(); // 5 peaks each with single detector

    // Add some detectors
    Mantid::Geometry::IPeak &ipeak3 = pw->getPeak(2);
    auto &peak3 = static_cast<Peak &>(ipeak3);
    peak3.addContributingDetID(11);
    Mantid::Geometry::IPeak &ipeak5 = pw->getPeak(4);
    auto &peak5 = static_cast<Peak &>(ipeak5);
    peak5.addContributingDetID(51);
    peak5.addContributingDetID(52);

    auto detTable = pw->createDetectorTable();
    TSM_ASSERT("No table has been created", detTable);
    if (!detTable)
      return;
    check_Detector_Table_Metadata(*detTable, 8);

    auto column0 = detTable->getColumn(0);
    auto column1 = detTable->getColumn(1);
    // Contents -- Be verbose, it's easier to understand
    // Peak 1
    TS_ASSERT_EQUALS(0, column0->cell<int>(0)); // Index 0
    TS_ASSERT_EQUALS(1, column1->cell<int>(0)); // Id 1
    // Peak 2
    TS_ASSERT_EQUALS(1, column0->cell<int>(1));  // Index 1
    TS_ASSERT_EQUALS(10, column1->cell<int>(1)); // Id 10

    // Peak 3
    TS_ASSERT_EQUALS(2, column0->cell<int>(2));  // Index 2
    TS_ASSERT_EQUALS(10, column1->cell<int>(2)); // Id 10
    TS_ASSERT_EQUALS(2, column0->cell<int>(3));  // Index 2
    TS_ASSERT_EQUALS(11, column1->cell<int>(3)); // Id 11

    // Peak 4
    TS_ASSERT_EQUALS(3, column0->cell<int>(4));  // Index 3
    TS_ASSERT_EQUALS(20, column1->cell<int>(4)); // Id 20

    // Peak 5
    TS_ASSERT_EQUALS(4, column0->cell<int>(5));  // Index 4
    TS_ASSERT_EQUALS(50, column1->cell<int>(5)); // Id 50
    TS_ASSERT_EQUALS(4, column0->cell<int>(6));  // Index 4
    TS_ASSERT_EQUALS(51, column1->cell<int>(6)); // Id 51
    TS_ASSERT_EQUALS(4, column0->cell<int>(7));  // Index 4
    TS_ASSERT_EQUALS(52, column1->cell<int>(7)); // Id 52
  }

  void test_default_getSpecialCoordinates() {
    auto pw = PeaksWorkspace_sptr(new PeaksWorkspace);
    TS_ASSERT_EQUALS(None, pw->getSpecialCoordinateSystem());
  }

  void test_setSpecialCoordinates() {
    auto pw = PeaksWorkspace_sptr(new PeaksWorkspace);
    SpecialCoordinateSystem coordSystem = Mantid::Kernel::HKL;
    pw->setCoordinateSystem(coordSystem);
    TS_ASSERT_EQUALS(coordSystem, pw->getSpecialCoordinateSystem());
  }

  void test_createPeakHKL() {
    const auto params = makePeakParameters();
    auto ws = makeWorkspace(params);
    // Create the peak
    Peak *peak = ws->createPeakHKL(params.hkl);

    /*
     Now we check we have made a self - consistent peak
     */
    TSM_ASSERT_EQUALS("New peak should have HKL we demanded.", params.hkl,
                      peak->getHKL());
    TSM_ASSERT_EQUALS("New peak should have QLab we expected.", params.qLab,
                      peak->getQLabFrame());
    TSM_ASSERT_EQUALS("New peak should have QSample we expected.",
                      params.qSample, peak->getQSampleFrame());

    auto detector = peak->getDetector();
    TSM_ASSERT("No detector", detector);
    TSM_ASSERT_EQUALS("This detector id does not match what we expect from the "
                      "instrument definition",
                      1, detector->getID());
    TSM_ASSERT_EQUALS("Thie detector position is wrong",
                      params.detectorPosition, detector->getPos());
    TSM_ASSERT_EQUALS("Goniometer has not been set properly",
                      params.goniometer.getR(), peak->getGoniometerMatrix());

    // Clean up.
    delete peak;
  }

  void test_create_peak_with_position_hkl() {
    const auto params = makePeakParameters();
    const auto ws = makeWorkspace(params);

    const auto peak = ws->createPeak(params.hkl, Mantid::Kernel::HKL);

    TSM_ASSERT_EQUALS("New peak should have HKL we demanded.", params.hkl,
                      peak->getHKL());
    TSM_ASSERT_EQUALS("New peak should have QLab we expected.", params.qLab,
                      peak->getQLabFrame());
    TSM_ASSERT_EQUALS("New peak should have QSample we expected.",
                      params.qSample, peak->getQSampleFrame());
  }

  void test_create_peak_with_position_qsample() {
    const auto params = makePeakParameters();
    const auto ws = makeWorkspace(params);

    const auto peak = ws->createPeak(params.qSample, Mantid::Kernel::QSample);

    TSM_ASSERT_EQUALS("New peak should have QLab we expected.", params.qLab,
                      peak->getQLabFrame());
    TSM_ASSERT_EQUALS("New peak should have QSample we expected.",
                      params.qSample, peak->getQSampleFrame());
  }

  void test_create_peak_with_position_qlab() {
    const auto params = makePeakParameters();
    const auto ws = makeWorkspace(params);

    const auto peak = ws->createPeak(params.qLab, Mantid::Kernel::QLab);

    TSM_ASSERT_EQUALS("New peak should have QLab we expected.", params.qLab,
                      peak->getQLabFrame());
    TSM_ASSERT_EQUALS("New peak should have QSample we expected.",
                      params.qSample, peak->getQSampleFrame());
  }

  void test_add_peak_with_position_hkl() {
    const auto params = makePeakParameters();
    const auto ws = makeWorkspace(params);

    ws->addPeak(params.hkl, Mantid::Kernel::HKL);
    const auto &peak = ws->getPeak(0);

    TSM_ASSERT_EQUALS("New peak should have HKL we demanded.", params.hkl,
                      peak.getHKL());
    TSM_ASSERT_EQUALS("New peak should have QLab we expected.", params.qLab,
                      peak.getQLabFrame());
    TSM_ASSERT_EQUALS("New peak should have QSample we expected.",
                      params.qSample, peak.getQSampleFrame());
  }

  void test_add_peak_with_position_qlab() {
    const auto params = makePeakParameters();
    const auto ws = makeWorkspace(params);

    ws->addPeak(params.qLab, Mantid::Kernel::QLab);
    const auto &peak = ws->getPeak(0);

    TSM_ASSERT_EQUALS("New peak should have QLab we expected.", params.qLab,
                      peak.getQLabFrame());
    TSM_ASSERT_EQUALS("New peak should have QSample we expected.",
                      params.qSample, peak.getQSampleFrame());
  }

  void test_add_peak_with_position_qsample() {
    const auto params = makePeakParameters();
    const auto ws = makeWorkspace(params);

    ws->addPeak(params.qSample, Mantid::Kernel::QSample);
    const auto &peak = ws->getPeak(0);

    TSM_ASSERT_EQUALS("New peak should have QLab we expected.", params.qLab,
                      peak.getQLabFrame());
    TSM_ASSERT_EQUALS("New peak should have QSample we expected.",
                      params.qSample, peak.getQSampleFrame());
  }

  /**
   * Test declaring an input PeaksWorkspace and retrieving it as const_sptr or
   * sptr
   */
  void testGetProperty_const_sptr() {
    const std::string wsName = "InputWorkspace";
    PeaksWorkspace_sptr wsInput(new PeaksWorkspace());
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Mantid::Kernel::Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    PeaksWorkspace_const_sptr wsConst;
    PeaksWorkspace_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(
        wsConst = manager.getValue<PeaksWorkspace_const_sptr>(wsName));
    TS_ASSERT(wsConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsNonConst =
                                 manager.getValue<PeaksWorkspace_sptr>(wsName));
    TS_ASSERT(wsNonConst != nullptr);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    PeaksWorkspace_const_sptr wsCastConst;
    PeaksWorkspace_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (PeaksWorkspace_const_sptr)val);
    TS_ASSERT(wsCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (PeaksWorkspace_sptr)val);
    TS_ASSERT(wsCastNonConst != nullptr);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }

  /**
   * Test declaring an input IPeaksWorkspace and retrieving it as const_sptr or
   * sptr
   */
  void testGetProperty_IPeaksWS_const_sptr() {
    const std::string wsName = "InputWorkspace";
    IPeaksWorkspace_sptr wsInput(new PeaksWorkspace());
    PropertyManagerHelper manager;
    manager.declareProperty(wsName, wsInput, Mantid::Kernel::Direction::Input);

    // Check property can be obtained as const_sptr or sptr
    IPeaksWorkspace_const_sptr wsConst;
    IPeaksWorkspace_sptr wsNonConst;
    TS_ASSERT_THROWS_NOTHING(
        wsConst = manager.getValue<IPeaksWorkspace_const_sptr>(wsName));
    TS_ASSERT(wsConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(
        wsNonConst = manager.getValue<IPeaksWorkspace_sptr>(wsName));
    TS_ASSERT(wsNonConst != nullptr);
    TS_ASSERT_EQUALS(wsConst, wsNonConst);

    // Check TypedValue can be cast to const_sptr or to sptr
    PropertyManagerHelper::TypedValue val(manager, wsName);
    IPeaksWorkspace_const_sptr wsCastConst;
    IPeaksWorkspace_sptr wsCastNonConst;
    TS_ASSERT_THROWS_NOTHING(wsCastConst = (IPeaksWorkspace_const_sptr)val);
    TS_ASSERT(wsCastConst != nullptr);
    TS_ASSERT_THROWS_NOTHING(wsCastNonConst = (IPeaksWorkspace_sptr)val);
    TS_ASSERT(wsCastNonConst != nullptr);
    TS_ASSERT_EQUALS(wsCastConst, wsCastNonConst);
  }

  void test_removePeaks() {
    // build peaksworkspace (note number of peaks = 1)
    auto pw = buildPW();
    Instrument_const_sptr inst = pw->getInstrument();

    // add peaks
    Peak p(inst, 1, 3.0);
    Peak p2(inst, 2, 6.0);
    Peak p3(inst, 3, 9.0);
    pw->addPeak(p);
    pw->addPeak(p2);
    pw->addPeak(p3);

    // number of peaks = 4, now remove 3
    std::vector<int> badPeaks{0, 2, 3};
    pw->removePeaks(std::move(badPeaks));
    TS_ASSERT_EQUALS(pw->getNumberPeaks(), 1);
  }

private:
  struct PeakParameters {
    Instrument_const_sptr instrument;
    Goniometer goniometer;
    OrientedLattice lattice;
    V3D hkl;
    V3D qLab;
    V3D qSample;
    V3D detectorPosition;
  };

  PeakParameters makePeakParameters() {
    // Create simple fictional instrument
    const V3D source(0, 0, 0);
    const V3D sample(15, 0, 0);
    const V3D detectorPos(20, 5, 0);
    const V3D beam1 = sample - source;
    const V3D beam2 = detectorPos - sample;
    auto minimalInstrument = ComponentCreationHelper::createMinimalInstrument(
        source, sample, detectorPos);

    // Derive distances and angles
    const double l1 = beam1.norm();
    const double l2 = beam2.norm();
    const V3D qLabDir = (beam1 / l1) - (beam2 / l2);

    const double microSecsInSec = 1e6;

    // Derive QLab for diffraction
    const double wavenumber_in_angstrom_times_tof_in_microsec =
        (Mantid::PhysicalConstants::NeutronMass * (l1 + l2) * 1e-10 *
         microSecsInSec) /
        Mantid::PhysicalConstants::h_bar;

    Mantid::Geometry::Goniometer goniometer;
    goniometer.pushAxis("axis1", 0, 1, 0);
    goniometer.setRotationAngle(0, 5);
    auto Rinv = goniometer.getR();
    Rinv.Invert();

    V3D qLab = qLabDir * wavenumber_in_angstrom_times_tof_in_microsec;

    Mantid::Geometry::OrientedLattice orientedLattice(
        1, 1, 1, 90, 90, 90); // U is identity, real and reciprocal lattice
                              // vectors are identical.

    V3D qSample = Rinv * qLab;
    V3D hkl = qSample / (2 * M_PI); // Given our settings above, this is the
                                    // simplified relationship between qLab and
                                    // hkl.

    return PeakParameters{
        minimalInstrument, goniometer, orientedLattice, hkl, qLab,
        qSample,           detectorPos};
  }

  PeaksWorkspace_sptr makeWorkspace(const PeakParameters &params) {
    auto lattice = new OrientedLattice(params.lattice);
    auto ws = boost::make_shared<PeaksWorkspace>();
    ws->setInstrument(params.instrument);
    ws->mutableSample().setOrientedLattice(lattice);
    ws->mutableRun().setGoniometer(params.goniometer, false);
    return ws;
  }

  PeaksWorkspace_sptr createSaveTestPeaksWorkspace() {
    // Create peak workspace
    auto pw = buildPW();
    Instrument_const_sptr inst = pw->getInstrument();

    // Add peaks (one peak already at detector ID 1)
    Peak p1(inst, 10, 4.0);
    Peak p2(inst, 10, 5.0);
    Peak p3(inst, 20, 3.0);
    Peak p4(inst, 50, 3.0);
    pw->addPeak(p1);
    pw->addPeak(p2);
    pw->addPeak(p3);
    pw->addPeak(p4);

    return pw;
  }

  void
  check_Detector_Table_Metadata(const Mantid::API::ITableWorkspace &detTable,
                                const size_t expectedNRows) {
    TS_ASSERT_EQUALS(expectedNRows, detTable.rowCount());
    TS_ASSERT_EQUALS(2, detTable.columnCount());
    if (detTable.columnCount() != 2)
      return;

    auto column0 = detTable.getColumn(0);
    auto column1 = detTable.getColumn(1);
    TS_ASSERT_EQUALS("Index", column0->name());
    TS_ASSERT_EQUALS("DetectorID", column1->name());
  }
};

#endif /* MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_ */
