// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/V3D.h"

#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidDataObjects/Peak.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace boost {
template <class CharType, class CharTrait>
std::basic_ostream<CharType, CharTrait> &operator<<(std::basic_ostream<CharType, CharTrait> &out,
                                                    std::optional<double> const &maybe) {
  if (maybe)
    out << maybe;
  return out;
}
} // namespace boost

class PeakTest : public CxxTest::TestSuite {
private:
  /// Common instrument
  Instrument_sptr inst;
  Instrument_sptr m_minimalInstrument;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakTest *createSuite() { return new PeakTest(); }
  static void destroySuite(PeakTest *suite) { delete suite; }

  // Constructor
  PeakTest() : inst(ComponentCreationHelper::createTestInstrumentRectangular(5, 100)) {}

  void test_constructor() {
    // detector IDs start at 10000
    Peak p(inst, 10000, 2.0);
    TS_ASSERT_DELTA(p.getH(), 0.0, 1e-5)
    TS_ASSERT_DELTA(p.getK(), 0.0, 1e-5)
    TS_ASSERT_DELTA(p.getL(), 0.0, 1e-5)
    TS_ASSERT_EQUALS(p.getDetectorID(), 10000)
    TS_ASSERT_EQUALS(p.getDetector()->getID(), 10000)
    TS_ASSERT_EQUALS(p.getInstrument(), inst)
    check_Contributing_Detectors(p, std::vector<int>(1, 10000));
  }

  void test_constructorHKL() {
    // detector IDs start at 10000
    Peak p(inst, 10000, 2.0, V3D(1, 2, 3));
    TS_ASSERT_DELTA(p.getH(), 1.0, 1e-5)
    TS_ASSERT_DELTA(p.getK(), 2.0, 1e-5)
    TS_ASSERT_DELTA(p.getL(), 3.0, 1e-5)
    TS_ASSERT_EQUALS(p.getDetectorID(), 10000)
    TS_ASSERT_EQUALS(p.getDetector()->getID(), 10000)
    TS_ASSERT_EQUALS(p.getInstrument(), inst)
    check_Contributing_Detectors(p, std::vector<int>(1, 10000));
  }

  void test_constructorHKLGon() {
    Matrix<double> mats(3, 3), mat(3, 3);
    for (int x = 0; x < 3; x++)
      for (int y = 0; y < 3; y++)
        mats[x][y] = 1.0 * x + 1.0 * y;
    mat[0][0] = 1.0;
    mat[1][2] = 1.0;
    mat[2][1] = 1.0;

    // detector IDs start at 10000
    TS_ASSERT_THROWS_ANYTHING(Peak ps(inst, 10000, 2.0, V3D(1, 2, 3), mats);)
    TS_ASSERT_THROWS_NOTHING(Peak p(inst, 10000, 2.0, V3D(1, 2, 3), mat);)
    Peak p(inst, 10000, 2.0, V3D(1, 2, 3), mat);
    TS_ASSERT_DELTA(p.getH(), 1.0, 1e-5)
    TS_ASSERT_DELTA(p.getK(), 2.0, 1e-5)
    TS_ASSERT_DELTA(p.getL(), 3.0, 1e-5)
    TS_ASSERT_EQUALS(p.getDetectorID(), 10000)
    TS_ASSERT_EQUALS(p.getDetector()->getID(), 10000)
    TS_ASSERT_EQUALS(p.getInstrument(), inst)
    TS_ASSERT_EQUALS(p.getGoniometerMatrix(), mat);
    check_Contributing_Detectors(p, std::vector<int>(1, 10000));
  }

  void test_ConstructorFromIPeakInterface() {
    Peak p(inst, 10102, 2.0);
    p.setHKL(1, 2, 3);
    p.setRunNumber(1234);
    p.addContributingDetID(10103);

    const Mantid::Geometry::IPeak &ipeak = p;
    Peak p2(ipeak);
    TS_ASSERT_EQUALS(p.getRow(), p2.getRow());
    TS_ASSERT_EQUALS(p.getCol(), p2.getCol());
    TS_ASSERT_EQUALS(p.getH(), p2.getH());
    TS_ASSERT_EQUALS(p.getK(), p2.getK());
    TS_ASSERT_EQUALS(p.getL(), p2.getL());
    TS_ASSERT_EQUALS(p.getGoniometerMatrix(), p2.getGoniometerMatrix());
    TS_ASSERT_EQUALS(p.getRunNumber(), p2.getRunNumber());
    TS_ASSERT_EQUALS(p.getDetector(), p2.getDetector())
    TS_ASSERT_EQUALS(p.getInstrument(), p2.getInstrument())
    auto expectedIDs = std::vector<int>(2, 10102);
    expectedIDs[1] = 10103;
    check_Contributing_Detectors(p2, expectedIDs);
  }

  void test_ConstructorFromLeanElasticPeak() {
    // step_1: constructing a peak (follow example in LeanElasticPeakTest)
    Matrix<double> r(3, 3, false);
    r[0][2] = 1;
    r[1][1] = 1;
    r[2][0] = -1;
    // NOTE: the detector ID here (19999) is an arbitrary number and will most
    //       likely no the same as the one from find_detector(). DO NOT compare
    //       detector verbatim
    Peak peak(inst, 19999, 2.0, V3D(1, 2, 3), r);
    peak.setRunNumber(1234);
    peak.setPeakNumber(42);
    peak.setIntensity(900);
    peak.setSigmaIntensity(30);
    peak.setBinCount(90);

    // step_2: extract qsample, goniometer, [wavelength] to construct a leanpeak
    V3D qsample = peak.getQSampleFrame();
    // NOTE: the goniometer matrix should be handled by BasePeak, and it should
    // be an exact copy of r created above
    auto goniometerMatrix = peak.getGoniometerMatrix();
    // construct the LeanPeak using QSample and goniometerMatrix
    const LeanElasticPeak &lpeak = LeanElasticPeak(qsample, goniometerMatrix);

    // step_3: construct Peak based on leanpeak and check
    //           - qlab
    //           - qsample
    //           - goniometer
    //           - sacttering
    //           - wavelength
    //           - d-spacing
    //           - initial and final energy
    //           - getAzimuthal angle
    //           - check detector id is any number (using LeanPeak test peak)
    const double tolerance{1e-10};
    Peak plp(lpeak, inst); // peak->leanpeak->peak
    TS_ASSERT_EQUALS(plp.getQLabFrame(), peak.getQLabFrame());
    TS_ASSERT_EQUALS(plp.getQSampleFrame(), peak.getQSampleFrame());
    TS_ASSERT_EQUALS(plp.getGoniometerMatrix(), r);
    TS_ASSERT_EQUALS(plp.getGoniometerMatrix(), goniometerMatrix);
    TS_ASSERT_EQUALS(plp.getScattering(), peak.getScattering());
    // NOTE: reasons to use TS_ASSERT_DELTA for some values
    //                  LeanPeak          Peak
    // wavelength    2.000000000000018    2
    // dspacing      9.093899818222381    9.093899818222283
    // initialEnergy 20.45105062499033    20.45105062499069
    // finalEnergy   20.45105062499033    20.45105062499069
    TS_ASSERT_DELTA(plp.getWavelength(), peak.getWavelength(), tolerance);
    TS_ASSERT_DELTA(plp.getDSpacing(), peak.getDSpacing(), tolerance);
    TS_ASSERT_DELTA(plp.getInitialEnergy(), peak.getInitialEnergy(), tolerance);
    TS_ASSERT_DELTA(plp.getFinalEnergy(), peak.getFinalEnergy(), tolerance);
    //
    TS_ASSERT_EQUALS(plp.getAzimuthal(), peak.getAzimuthal());
    // Actually check that we found the same detector ID
    TS_ASSERT_EQUALS(plp.getDetectorID(), 19999);

    std::ostringstream msg;
    msg.precision(16);
    msg << "\t\tLeanPeak\t\tPeak\n"
        << "wavelength\t\t" << plp.getWavelength() << "\t\t" << peak.getWavelength() << "\n"
        << "dspacing\t\t" << plp.getDSpacing() << "\t\t" << peak.getDSpacing() << "\n"
        << "initialEnergy\t\t" << plp.getInitialEnergy() << "\t\t" << peak.getInitialEnergy() << "\n"
        << "finalEnergy\t\t" << plp.getFinalEnergy() << "\t\t" << peak.getFinalEnergy() << "\n";
    std::cout << msg.str();
  }

  void test_copyConstructor() {
    Peak p(inst, 10102, 2.0);
    p.setHKL(1, 2, 3);
    p.setRunNumber(1234);
    // Default (not-explicit) copy constructor
    Peak p2(p);
    TS_ASSERT_EQUALS(p.getRow(), p2.getRow());
    TS_ASSERT_EQUALS(p.getCol(), p2.getCol());
    TS_ASSERT_EQUALS(p.getH(), p2.getH());
    TS_ASSERT_EQUALS(p.getK(), p2.getK());
    TS_ASSERT_EQUALS(p.getL(), p2.getL());
    TS_ASSERT_EQUALS(p.getGoniometerMatrix(), p2.getGoniometerMatrix());
    TS_ASSERT_EQUALS(p.getRunNumber(), p2.getRunNumber());
    TS_ASSERT_EQUALS(p.getDetector(), p2.getDetector());
    TS_ASSERT_EQUALS(p.getInstrument(), p2.getInstrument());
    TS_ASSERT_EQUALS(p.getPeakShape().shapeName(), p2.getPeakShape().shapeName());
    check_Contributing_Detectors(p2, std::vector<int>(1, 10102));
  }

  void test_getValueByColName() {
    Peak p(inst, 10102, 2.0);
    p.setHKL(1, 2, 3);
    p.setRunNumber(1234);
    TS_ASSERT_EQUALS(p.getValueByColName("Row"), p.getRow());
    TS_ASSERT_EQUALS(p.getValueByColName("Col"), p.getCol());
    TS_ASSERT_EQUALS(p.getValueByColName("H"), p.getH());
    TS_ASSERT_EQUALS(p.getValueByColName("K"), p.getK());
    TS_ASSERT_EQUALS(p.getValueByColName("L"), p.getL());
    TS_ASSERT_EQUALS(p.getValueByColName("RunNumber"), p.getRunNumber());
    TS_ASSERT_EQUALS(p.getValueByColName("DetID"), p.getDetectorID())
    TS_ASSERT_THROWS_ANYTHING(p.getValueByColName("bankname"));
  }

  /** Set the wavelength and see the other "versions" of it get calculated. */
  void test_wavelength_conversion() {
    // 1 angstroms wavelength, and at the opposite corner of the detector
    Peak p(inst, 19999, 1.0);
    // Energy in meV
    TS_ASSERT_DELTA(p.getInitialEnergy(), 81.805, 1e-3) // Conversion table at :
    // www.ncnr.nist.gov/instruments/dcs/dcs_usersguide/Conversion_Factors.pdf
    TS_ASSERT_DELTA(p.getFinalEnergy(), p.getInitialEnergy(), 1e-5)
    V3D dp = p.getDetPos();
    double tt = dp.angle(V3D(0, 0, 1));
    double d = 0.5 / sin(0.5 * tt); // d=lambda/2/sin(theta)=4.5469
    TS_ASSERT_DELTA(p.getDSpacing(), d, 1e-3);
    TS_ASSERT_DELTA(p.getTOF(), 3823, 1);

    // Back-converting to wavelength should give you the same.
    TS_ASSERT_DELTA(p.getWavelength(), 1.00, 1e-2);
  }

  void test_badDetectorID_throws() {
    Peak p(inst, 10000, 2.0);
    TS_ASSERT_THROWS_ANYTHING(p.setDetectorID(7));
  }

  void test_setDetector_Adds_ID_To_Contributing_List_And_Does_Not_Remove_Old_From_Contrib_List() {
    int expectedIDs[2] = {10000, 10001};
    Peak peak(inst, expectedIDs[0], 2.0);
    peak.setDetectorID(expectedIDs[1]);

    check_Contributing_Detectors(peak, std::vector<int>(expectedIDs, expectedIDs + 2));
  }

  void test_runNumber() {
    Peak p(inst, 10000, 2.0);
    p.setRunNumber(12345);
    TS_ASSERT_EQUALS(p.getRunNumber(), 12345);
  }

  void test_GoniometerMatrix() {
    Peak p(inst, 10000, 2.0);
    Matrix<double> mats(3, 3), mat(3, 3);
    for (int x = 0; x < 3; x++)
      for (int y = 0; y < 3; y++)
        mats[x][y] = 1.0 * x + 1.0 * y;
    TS_ASSERT_THROWS_ANYTHING(p.setGoniometerMatrix(mats)); // matrix is
                                                            // singular
    TS_ASSERT_EQUALS(p.getGoniometerMatrix(), mats);
    mat[0][0] = 1.0;
    mat[1][2] = 1.0;
    mat[2][1] = 1.0;
    TS_ASSERT_THROWS_NOTHING(p.setGoniometerMatrix(mat)); // matrix is not singular
    TS_ASSERT_EQUALS(p.getGoniometerMatrix(), mat);
    // Matrix must be 3x3
    Matrix<double> mat2(4, 3);
    TS_ASSERT_THROWS_ANYTHING(p.setGoniometerMatrix(mat2));
  }

  void test_HKL() {
    Peak p(inst, 10000, 2.0);
    p.setHKL(1.0, 2.0, 3.0);
    TS_ASSERT_EQUALS(p.getH(), 1.0);
    TS_ASSERT_EQUALS(p.getK(), 2.0);
    TS_ASSERT_EQUALS(p.getL(), 3.0);
    p.setH(5);
    p.setK(6);
    p.setL(7);
    TS_ASSERT_EQUALS(p.getH(), 5.0);
    TS_ASSERT_EQUALS(p.getK(), 6.0);
    TS_ASSERT_EQUALS(p.getL(), 7.0);
    p.setHKL(V3D(1.0, 2.0, 3.0));
    TS_ASSERT_EQUALS(p.getH(), 1.0);
    TS_ASSERT_EQUALS(p.getK(), 2.0);
    TS_ASSERT_EQUALS(p.getL(), 3.0);
    TS_ASSERT_EQUALS(p.getHKL(), V3D(1.0, 2.0, 3.0));
  }

  void test_isIndexed() {
    Peak p(inst, 10000, 2.0);
    TS_ASSERT_EQUALS(false, p.isIndexed());
    p.setHKL(1, 2, 3);
    TS_ASSERT_EQUALS(true, p.isIndexed());
  }

  void test_samplePos() {
    Peak p(inst, 10000, 2.0);
    p.setSamplePos(1.0, 1.0, 1.0);
    TS_ASSERT_EQUALS(p.getSamplePos(), V3D(1.0, 1.0, 1.0));
    p.setSamplePos(V3D(2.0, 2.0, 2.0));
    TS_ASSERT_EQUALS(p.getSamplePos(), V3D(2.0, 2.0, 2.0));
  }

  void test_getBank_and_row() {
    Peak p(inst, 10000, 2.0);
    TS_ASSERT_EQUALS(p.getBankName(), "bank1")
    TS_ASSERT_EQUALS(p.getRow(), 0)
    TS_ASSERT_EQUALS(p.getCol(), 0)
    p.setDetectorID(10050);
    TS_ASSERT_EQUALS(p.getRow(), 50)
    TS_ASSERT_EQUALS(p.getCol(), 0)
    p.setDetectorID(10100);
    TS_ASSERT_EQUALS(p.getRow(), 0)
    TS_ASSERT_EQUALS(p.getCol(), 1)
  }

  void test_getQSampleFrame() {
    // Peak 3 is phi,chi,omega of 90,0,0; giving this matrix:
    Matrix<double> r2(3, 3, false);
    r2[0][2] = 1;
    r2[1][1] = 1;
    r2[2][0] = -1;

    Peak p(inst, 10000, 2.0);
    p.setGoniometerMatrix(r2);

    // Q in the lab frame
    V3D qLab = p.getQLabFrame();
    // q in the sample frame.
    V3D qSample = p.getQSampleFrame();
    // If we re-rotate q in the sample frame by the gonio matrix, we should get
    // q in the lab frame
    V3D qSampleRotated = r2 * qSample;

    // Did the peak properly invert the rotation matrix?
    TS_ASSERT_EQUALS(qLab, qSampleRotated);
  }

  void test_getQLabFrame() {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
    Peak p(inst, 0, 1.5);
    p.setQLabFrame(V3D(1, 1, 1));
    auto q = p.getQLabFrame();
    // should be the same
    TS_ASSERT_DELTA(q[0], 1, 1e-5);
    TS_ASSERT_DELTA(q[1], 1, 1e-5);
    TS_ASSERT_DELTA(q[2], 1, 1e-5);
  }

  void test_getSourceDirectionSampleFrame() {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
    Peak p(inst, 0, 1.5);
    p.setQLabFrame(V3D(1, 2, 3));

    Matrix<double> r2(3, 3, false);
    r2[0][2] = 1;
    r2[1][1] = 1;
    r2[2][0] = -1;

    p.setGoniometerMatrix(r2);

    V3D dir = p.getSourceDirectionSampleFrame();

    TS_ASSERT_DELTA(dir[0], 1, 1e-5);
    TS_ASSERT_DELTA(dir[1], 0, 1e-5);
    TS_ASSERT_DELTA(dir[2], 0, 1e-5);
  }

  void test_getDetectorDirectionSampleFrame() {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
    Peak p(inst, 0, 1.5);
    p.setQLabFrame(V3D(1, 2, 3));

    Matrix<double> r2(3, 3, false);
    r2[0][2] = 1;
    r2[1][1] = 1;
    r2[2][0] = -1;

    p.setGoniometerMatrix(r2);

    V3D dir = p.getDetectorDirectionSampleFrame();

    TS_ASSERT_DELTA(dir[0], -cos(p.getScattering()), 1e-5);
    TS_ASSERT_DELTA(dir[1], sin(p.getScattering()) * sin(p.getAzimuthal()), 1e-5);
    TS_ASSERT_DELTA(dir[2], sin(p.getScattering()) * cos(p.getAzimuthal()), 1e-5);
  }

  //------------------------------------------------------------------------------------
  /** Can't have Q = 0,0,0 or 0 in the Z direction when creating */
  void test_setQLabFrame_ThrowsIfQIsNull() {
    Peak p1(inst, 10000, 2.0);
    const std::optional<double> distance = 1.0;
    TS_ASSERT_THROWS_ANYTHING(Peak p2(inst, V3D(0, 0, 0), distance));
    TS_ASSERT_THROWS_ANYTHING(Peak p2(inst, V3D(1, 2, 0), distance));
  }

  /** Compare two peaks, but not the detector IDs etc. */
  void comparePeaks(Peak &p1, Peak &p2) {
    // TODO. Peak should implement bool operator==(const Peak&) and that should
    // be tested, rather than having external functionality here.
    TS_ASSERT_EQUALS(p1.getQLabFrame(), p2.getQLabFrame());
    TS_ASSERT_EQUALS(p1.getQSampleFrame(), p2.getQSampleFrame());
    TS_ASSERT_EQUALS(p1.getDetPos(), p2.getDetPos());
    TS_ASSERT_EQUALS(p1.getHKL(), p2.getHKL());
    TS_ASSERT_DELTA(p1.getWavelength(), p2.getWavelength(), 1e-5);
    TS_ASSERT_DELTA(p1.getL1(), p2.getL1(), 1e-5);
    TS_ASSERT_DELTA(p1.getL2(), p2.getL2(), 1e-5);
    TS_ASSERT_DELTA(p1.getTOF(), p2.getTOF(), 1e-5);
    TS_ASSERT_DELTA(p1.getInitialEnergy(), p2.getInitialEnergy(), 1e-5);
    TS_ASSERT_DELTA(p1.getFinalEnergy(), p2.getFinalEnergy(), 1e-5);
    TS_ASSERT(p1.getGoniometerMatrix().equals(p2.getGoniometerMatrix(), 1e-5));
  }

  /** Create peaks using Q in the lab frame */
  void test_setQLabFrame() {
    Peak p1(inst, 19999, 2.0);
    V3D Qlab1 = p1.getQLabFrame();
    V3D detPos1 = p1.getDetPos();

    // Construct using just Q
    Peak p2(inst, Qlab1, std::optional<double>(detPos1.norm()));
    comparePeaks(p1, p2);
    TS_ASSERT_EQUALS(p2.getBankName(), "None");
    TS_ASSERT_EQUALS(p2.getRow(), -1);
    TS_ASSERT_EQUALS(p2.getCol(), -1);
    TS_ASSERT_EQUALS(p2.getDetectorID(), -1);
  }

  void test_setQLabFrame2() {
    // Create fictional instrument
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

    Peak peak;                             // Everything will be default
    peak.setInstrument(minimalInstrument); // Can't do anything without the instrument
    peak.setQLabFrame(qLab);
    auto detector = peak.getDetector();

    TSM_ASSERT("No detector", detector);
    TS_ASSERT_EQUALS(1, detector->getID());
    TS_ASSERT_EQUALS(detectorPos, detector->getPos());
  }

  /** Create peaks using Q in sample frame + a goniometer rotation matrix*/
  void test_setQSampleFrame() {
    // A goniometer rotation matrix
    Matrix<double> r2(3, 3, false);
    r2[0][2] = 1;
    r2[1][1] = 1;
    r2[2][0] = -1;

    Peak p1(inst, 19999, 2.0, V3D(1, 2, 3), r2);
    V3D q = p1.getQSampleFrame();
    V3D detPos1 = p1.getDetPos();

    // Construct using Q + rotation matrix
    Peak p2(inst, q, r2, detPos1.norm());
    p2.setHKL(V3D(1, 2, 3)); // Make sure HKL matches too.
    comparePeaks(p1, p2);
    TS_ASSERT_EQUALS(p2.getBankName(), "None");
    TS_ASSERT_EQUALS(p2.getRow(), -1);
    TS_ASSERT_EQUALS(p2.getCol(), -1);
    TS_ASSERT_EQUALS(p2.getDetectorID(), -1);
  }

  void test_setQSampleFrameVirtualDetectorWithQLab() {
    constexpr auto radius = 10.;
    auto sphereInst = ComponentCreationHelper::createTestInstrumentRectangular(5, 100);
    auto extendedSpaceObj = ComponentCreationHelper::createSphere(10., V3D(0, 0, 0));
    auto extendedSpace = std::make_unique<ObjComponent>("extended-detector-space", extendedSpaceObj, sphereInst.get());
    extendedSpace->setPos(V3D(0.0, 0.0, 0.0));
    sphereInst->add(extendedSpace.release());
    const auto refFrame = sphereInst->getReferenceFrame();
    const auto refBeamDir = refFrame->vecPointingAlongBeam();

    // test with & without extended detector space
    // extended space is a sphere, so all points should fall radius*detector
    // direction away from the detector direction with extended space
    auto testQ = [&](const V3D &q) {
      // Compute expected direction
      const auto qBeam = q.scalar_prod(refBeamDir);
      const double norm_q = q.norm();
      const double one_over_wl = (norm_q * norm_q) / (2.0 * qBeam);

      V3D detectorDir = q * -1.0;
      detectorDir[refFrame->pointingAlongBeam()] = one_over_wl - qBeam;
      detectorDir.normalize();

      // test without extended detector space
      // should be a unit vector in the direction of the virtual detector
      // position
      Peak peak1(inst, q);

      // skip tests for which Q actually does intersect with a valid detector
      if (peak1.getDetectorID() > 0) {
        return;
      }

      TS_ASSERT_EQUALS(peak1.getDetectorID(), -1)
      TS_ASSERT_EQUALS(peak1.getDetPos(), detectorDir)

      // test with extended detector space
      // should be the full vector to the virtual detector position
      Peak peak2(sphereInst, q);
      TS_ASSERT_EQUALS(peak2.getDetectorID(), -1)
      TS_ASSERT_EQUALS(peak2.getDetPos(), detectorDir * radius)
    };

    // Make hemisphere of q vectors to test
    std::vector<double> xDirections(20);
    std::vector<double> yDirections(20);
    std::vector<double> zDirections(10);

    // create x values of the range -1 to 1
    int index = 0;
    double startValue = -1;
    std::generate(xDirections.begin(), xDirections.end(),
                  [&index, &startValue]() { return startValue + index++ * 0.1; });

    // create z values of the range 0.1 to 1
    // ignore negative z values as these are not physical!
    index = 0;
    startValue = 0.1;
    std::generate(zDirections.begin(), zDirections.end(),
                  [&index, &startValue]() { return startValue + index++ * 0.1; });

    yDirections = xDirections;

    for (auto &x : xDirections) {
      for (auto &y : yDirections) {
        for (auto &z : zDirections) {
          testQ(V3D(x, y, z));
        }
      }
    }
  }

  void test_setQSampleFrameVirtualDetectorWithScatteringAngle() {
    auto sphereInst = ComponentCreationHelper::createTestInstrumentRectangular(5, 100);
    auto extendedSpaceObj = ComponentCreationHelper::createSphere(10., V3D(0, 0, 0));
    auto extendedSpace = std::make_unique<ObjComponent>("extended-detector-space", extendedSpaceObj, sphereInst.get());
    extendedSpace->setPos(V3D(0.0, 0.0, 0.0));
    sphereInst->add(extendedSpace.release());

    // test with & without extended detector space
    // extended space is a sphere, so all points should fall radius*detector
    // direction away from the detector direction with extended space
    auto testTheta = [this, &sphereInst](const double theta) {
      constexpr auto radius = 10.;
      const auto expectedDir = V3D(sin(theta), 0., cos(theta));

      // test without extended detector space
      // should be {sin(theta), 0, cos(theta)}
      Peak p1(this->inst, theta, 2.0);
      V3D detPos1 = p1.getDetPos();
      TS_ASSERT_EQUALS(detPos1, expectedDir);

      // test with extended detector space
      // should be radius*{sin(theta), 0, cos(theta)}
      Peak p2(sphereInst, theta, 2.0);
      V3D detPos2 = p2.getDetPos();
      TS_ASSERT_EQUALS(detPos2, expectedDir * radius);
    };

    // generate & test a range of angles from 0 - 360
    int index = 0;
    std::vector<double> angles(8);
    std::generate(angles.begin(), angles.end(), [&index, &angles]() {
      return static_cast<double>(index++) * M_PI / static_cast<double>(angles.size());
    });

    std::for_each(angles.begin(), angles.end(), testTheta);
  }

  /** Create peaks using Q in the lab frame,
   * then find the corresponding detector ID */
  void test_findDetector() {
    Peak p1(inst, 19999, 2.0);
    V3D Qlab1 = p1.getQLabFrame();
    V3D detPos1 = p1.getDetPos();

    // Construct using just Q
    Peak p2(inst, Qlab1, std::optional<double>(detPos1.norm()));
    TS_ASSERT(p2.findDetector());
    comparePeaks(p1, p2);
    TS_ASSERT_EQUALS(p2.getBankName(), "bank1");
    TS_ASSERT_EQUALS(p2.getRow(), 99);
    TS_ASSERT_EQUALS(p2.getCol(), 99);
    TS_ASSERT_EQUALS(p2.getDetectorID(), 19999);
  }

  void test_getDetectorPosition() {
    const int detectorId = 19999;
    const double wavelength = 2;
    Peak p(inst, detectorId, wavelength);

    V3D a = p.getDetectorPosition();
    V3D b = p.getDetectorPositionNoCheck();

    TSM_ASSERT_EQUALS("Results should be the same", a, b);
  }

  void test_getDetectorPositionThrows() {
    const int detectorId = 19999;
    const double wavelength = 2;
    Peak p(inst, detectorId, wavelength);
    TSM_ASSERT_THROWS_NOTHING("Nothing wrong here, detector is valid", p.getDetectorPosition());
    p.setQLabFrame(V3D(1, 1, 1),
                   1.0); // This sets the detector pointer to null and detector id to -1;
    TSM_ASSERT_THROWS("Detector is not valid", p.getDetectorPosition(),
                      Mantid::Kernel::Exception::NullPointerException &);
  }

  void test_get_peak_shape_default() {
    Peak peak;
    const PeakShape &integratedShape = peak.getPeakShape();
    TS_ASSERT_EQUALS("none", integratedShape.shapeName());
  }

  void test_set_peak_shape() {
    using namespace testing;

    Peak peak;

    MockPeakShape *replacementShape = new MockPeakShape;
    EXPECT_CALL(*replacementShape, shapeName()).Times(1);
    peak.setPeakShape(replacementShape);

    const PeakShape &currentShape = peak.getPeakShape();
    currentShape.shapeName();

    TS_ASSERT(Mock::VerifyAndClearExpectations(replacementShape));
  }

  void test_get_intensity_over_sigma() {
    const int detectorId = 19999;
    const double wavelength = 2;
    const double intensity{100};
    const double sigma{10};
    Peak p(inst, detectorId, wavelength);

    p.setIntensity(intensity);
    p.setSigmaIntensity(sigma);

    TS_ASSERT_EQUALS(p.getIntensityOverSigma(), intensity / sigma);
  }

  void test_get_intensity_over_sigma_empty_sigma() {
    const int detectorId = 19999;
    const double wavelength = 2;
    const double intensity{10};
    const double sigma{0};
    Peak p(inst, detectorId, wavelength);

    p.setIntensity(intensity);
    p.setSigmaIntensity(sigma);

    const double expectedResult{0.0};
    const double tolerance{1e-10};
    TS_ASSERT_DELTA(p.getIntensityOverSigma(), expectedResult, tolerance);
  }

  void test_get_energy() {
    const int detectorId = 19999;
    const double wavelength = 2;
    const double initialEnergy{100};
    const double finalEnergy{110};
    Peak p(inst, detectorId, wavelength);

    p.setInitialEnergy(initialEnergy);
    p.setFinalEnergy(finalEnergy);

    TS_ASSERT_EQUALS(p.getEnergyTransfer(), initialEnergy - finalEnergy);
  }

private:
  void check_Contributing_Detectors(const Peak &peak, const std::vector<int> &expected) {
    auto peakIDs = peak.getContributingDetIDs();
    for (int id : expected) {
      TSM_ASSERT_EQUALS("Expected " + boost::lexical_cast<std::string>(id) + " in contribution list", 1,
                        peakIDs.count(id))
    }
  }
};
