#ifndef MANTID_DATAOBJECTS_PEAKTEST_H_
#define MANTID_DATAOBJECTS_PEAKTEST_H_

#include <cxxtest/TestSuite.h>
#include "MockObjects.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include <iostream>
#include <iomanip>
#include <gmock/gmock.h>

#include "MantidDataObjects/Peak.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class PeakTest : public CxxTest::TestSuite
{
private:
    /// Common instrument
    Instrument_sptr inst;
    Instrument_sptr m_minimalInstrument;
public:


  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakTest *createSuite() {
    return new PeakTest();
  }
  static void destroySuite(PeakTest *suite) { delete suite; }


  // Constructor
  PeakTest() : inst(ComponentCreationHelper::createTestInstrumentRectangular(5, 100))
  {

  }

  void test_constructor()
  {
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

  void test_constructorHKL()
  {
    // detector IDs start at 10000
    Peak p(inst, 10000, 2.0, V3D(1,2,3) );
    TS_ASSERT_DELTA(p.getH(), 1.0, 1e-5)
    TS_ASSERT_DELTA(p.getK(), 2.0, 1e-5)
    TS_ASSERT_DELTA(p.getL(), 3.0, 1e-5)
    TS_ASSERT_EQUALS(p.getDetectorID(), 10000)
    TS_ASSERT_EQUALS(p.getDetector()->getID(), 10000)
    TS_ASSERT_EQUALS(p.getInstrument(), inst)
    check_Contributing_Detectors(p, std::vector<int>(1, 10000));
  }

  void test_constructorHKLGon()
  {
    Matrix<double> mats(3,3),mat(3,3);
    for (int x=0; x<3; x++)
      for (int y=0; y<3; y++)
        mats[x][y]=1.0*x+1.0*y;
    mat[0][0]=1.0;mat[1][2]=1.0;mat[2][1]=1.0;
    
    // detector IDs start at 10000
    TS_ASSERT_THROWS_ANYTHING(Peak ps(inst, 10000, 2.0, V3D(1,2,3),mats );)
    TS_ASSERT_THROWS_NOTHING(Peak p(inst, 10000, 2.0, V3D(1,2,3),mat );) 
    Peak p(inst, 10000, 2.0, V3D(1,2,3),mat );
    TS_ASSERT_DELTA(p.getH(), 1.0, 1e-5)
    TS_ASSERT_DELTA(p.getK(), 2.0, 1e-5)
    TS_ASSERT_DELTA(p.getL(), 3.0, 1e-5)
    TS_ASSERT_EQUALS(p.getDetectorID(), 10000)
    TS_ASSERT_EQUALS(p.getDetector()->getID(), 10000)
    TS_ASSERT_EQUALS(p.getInstrument(), inst)
    TS_ASSERT_EQUALS( p.getGoniometerMatrix(), mat);
    check_Contributing_Detectors(p, std::vector<int>(1, 10000));
  }

  void test_ConstructorFromIPeakInterface()
  {
    Peak p(inst, 10102, 2.0);
    p.setHKL(1,2,3);
    p.setRunNumber(1234);
    p.addContributingDetID(10103);

    const Mantid::API::IPeak & ipeak = p;
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


  void test_copyConstructor()
  {
    Peak p(inst, 10102, 2.0);
    p.setHKL(1,2,3);
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

  void test_getValueByColName()
  {
    Peak p(inst, 10102, 2.0);
    p.setHKL(1,2,3);
    p.setRunNumber(1234);
    TS_ASSERT_EQUALS(p.getValueByColName("Row"), p.getRow());
    TS_ASSERT_EQUALS(p.getValueByColName("Col"), p.getCol());
    TS_ASSERT_EQUALS(p.getValueByColName("H"), p.getH());
    TS_ASSERT_EQUALS(p.getValueByColName("K"), p.getK());
    TS_ASSERT_EQUALS(p.getValueByColName("L"), p.getL());
    TS_ASSERT_EQUALS(p.getValueByColName("RunNumber"), p.getRunNumber());
    TS_ASSERT_EQUALS(p.getValueByColName("DetId"), p.getDetectorID())
    TS_ASSERT_THROWS_ANYTHING( p.getValueByColName("bankname") );
  }

  /** Set the wavelength and see the other "versions" of it get calculated. */
  void test_wavelength_conversion()
  {
    //1 angstroms wavelength, and at the opposite corner of the detector
    Peak p(inst, 19999, 1.0);
    // Energy in meV
    TS_ASSERT_DELTA(p.getInitialEnergy(), 81.805, 1e-3) // Conversion table at : www.ncnr.nist.gov/instruments/dcs/dcs_usersguide/Conversion_Factors.pdf
    TS_ASSERT_DELTA(p.getFinalEnergy(), p.getInitialEnergy(), 1e-5)
    V3D dp=p.getDetPos();
    double tt=dp.angle(V3D(0,0,1));
    double d=0.5/sin(0.5*tt);  //d=lambda/2/sin(theta)=4.5469
    TS_ASSERT_DELTA(p.getDSpacing(), d, 1e-3);
    TS_ASSERT_DELTA(p.getTOF(), 3823, 1);

    // Back-converting to wavelength should give you the same.
    TS_ASSERT_DELTA(p.getWavelength(), 1.00, 1e-2);

  }

  void test_badDetectorID_throws()
  {
    Peak p(inst, 10000, 2.0);
    TS_ASSERT_THROWS_ANYTHING( p.setDetectorID(7) );
  }

  void test_setDetector_Adds_ID_To_Contributing_List_And_Does_Not_Remove_Old_From_Contrib_List()
  {
    int expectedIDs[2] = {10000, 10001};
    Peak peak(inst, expectedIDs[0], 2.0);
    peak.setDetectorID(expectedIDs[1]);

    check_Contributing_Detectors(peak, std::vector<int>(expectedIDs,expectedIDs+2));
  }

  void test_runNumber()
  {
    Peak p(inst, 10000, 2.0);
    p.setRunNumber(12345);
    TS_ASSERT_EQUALS( p.getRunNumber(), 12345);
  }

  void test_GoniometerMatrix()
  {
    Peak p(inst, 10000, 2.0);
    Matrix<double> mats(3,3),mat(3,3);
    for (int x=0; x<3; x++)
      for (int y=0; y<3; y++)
        mats[x][y]=1.0*x+1.0*y;
    TS_ASSERT_THROWS_ANYTHING(p.setGoniometerMatrix(mats)); //matrix is singular
    TS_ASSERT_EQUALS( p.getGoniometerMatrix(), mats);
    mat[0][0]=1.0;mat[1][2]=1.0;mat[2][1]=1.0;
    TS_ASSERT_THROWS_NOTHING(p.setGoniometerMatrix(mat)); //matrix is not singular
    TS_ASSERT_EQUALS( p.getGoniometerMatrix(), mat);
    // Matrix must be 3x3
    Matrix<double> mat2(4,3);
    TS_ASSERT_THROWS_ANYTHING( p.setGoniometerMatrix(mat2) );
  }

  void test_HKL()
  {
    Peak p(inst, 10000, 2.0);
    p.setHKL(1.0, 2.0, 3.0);
    TS_ASSERT_EQUALS( p.getH(), 1.0);
    TS_ASSERT_EQUALS( p.getK(), 2.0);
    TS_ASSERT_EQUALS( p.getL(), 3.0);
    p.setH(5);
    p.setK(6);
    p.setL(7);
    TS_ASSERT_EQUALS( p.getH(), 5.0);
    TS_ASSERT_EQUALS( p.getK(), 6.0);
    TS_ASSERT_EQUALS( p.getL(), 7.0);
    p.setHKL(V3D(1.0, 2.0, 3.0));
    TS_ASSERT_EQUALS( p.getH(), 1.0);
    TS_ASSERT_EQUALS( p.getK(), 2.0);
    TS_ASSERT_EQUALS( p.getL(), 3.0);
    TS_ASSERT_EQUALS( p.getHKL(), V3D(1.0, 2.0, 3.0));
  }

  void test_getBank_and_row()
  {
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

  void test_getQSampleFrame()
  {
    // Peak 3 is phi,chi,omega of 90,0,0; giving this matrix:
    Matrix<double> r2(3,3,false);
    r2[0][2] = 1;
    r2[1][1] = 1;
    r2[2][0] = -1;

    Peak p(inst, 10000, 2.0);
    p.setGoniometerMatrix(r2);

    // Q in the lab frame
    V3D qLab = p.getQLabFrame();
    // q in the sample frame.
    V3D qSample = p.getQSampleFrame();
    // If we re-rotate q in the sample frame by the gonio matrix, we should get q in the lab frame
    V3D qSampleRotated = r2 * qSample;

    // Did the peak properly invert the rotation matrix?
    TS_ASSERT_EQUALS(qLab, qSampleRotated);
  }


  //------------------------------------------------------------------------------------
  /** Can't have Q = 0,0,0 or 0 in the Z direction when creating */
  void test_setQLabFrame_ThrowsIfQIsNull()
  {
    Peak p1(inst, 10000, 2.0);
    TS_ASSERT_THROWS_ANYTHING(Peak p2(inst, V3D(0,0,0), 1.0));
    TS_ASSERT_THROWS_ANYTHING(Peak p2(inst, V3D(1,2,0), 1.0));
  }


  /** Compare two peaks, but not the detector IDs etc. */
  void comparePeaks(Peak & p1, Peak & p2)
  {
    // TODO. Peak should implement bool operator==(const Peak&) and that should be tested, rather than having external functionality here.
    TS_ASSERT_EQUALS( p1.getQLabFrame(), p2.getQLabFrame() );
    TS_ASSERT_EQUALS( p1.getQSampleFrame(), p2.getQSampleFrame() );
    TS_ASSERT_EQUALS( p1.getDetPos(), p2.getDetPos() );
    TS_ASSERT_EQUALS( p1.getHKL(), p2.getHKL() );
    TS_ASSERT_DELTA( p1.getWavelength(), p2.getWavelength(), 1e-5 );
    TS_ASSERT_DELTA( p1.getL1(), p2.getL1(), 1e-5 );
    TS_ASSERT_DELTA( p1.getL2(), p2.getL2(), 1e-5 );
    TS_ASSERT_DELTA( p1.getTOF(), p2.getTOF(), 1e-5 );
    TS_ASSERT_DELTA( p1.getInitialEnergy(), p2.getInitialEnergy(), 1e-5 );
    TS_ASSERT_DELTA( p1.getFinalEnergy(), p2.getFinalEnergy(), 1e-5 );
    TS_ASSERT( p1.getGoniometerMatrix().equals(p2.getGoniometerMatrix(), 1e-5) );
  }

  /** Create peaks using Q in the lab frame */
  void test_setQLabFrame()
  {
    Peak p1(inst, 19999, 2.0);
    V3D Qlab1 = p1.getQLabFrame();
    V3D detPos1 = p1.getDetPos();

    // Construct using just Q
    Peak p2(inst, Qlab1, detPos1.norm());
    comparePeaks(p1, p2);
    TS_ASSERT_EQUALS( p2.getBankName(), "None");
    TS_ASSERT_EQUALS( p2.getRow(), -1);
    TS_ASSERT_EQUALS( p2.getCol(), -1);
    TS_ASSERT_EQUALS( p2.getDetectorID(), -1);
  }

  void test_setQLabFrame2()
  {

      // Create fictional instrument
      const V3D source(0,0,0);
      const V3D sample(15, 0, 0);
      const V3D detectorPos(20, 5, 0);
      const V3D beam1 = sample - source;
      const V3D beam2 = detectorPos - sample;
      auto minimalInstrument = ComponentCreationHelper::createMinimalInstrument( source, sample, detectorPos );

      // Calculate energy of neutron based on velocity
      const double velocity = 1.1 * 10e3; // m/sec
      double efixed = 0.5 * Mantid::PhysicalConstants::NeutronMass * velocity * velocity  ; // In Joules
      efixed = efixed / Mantid::PhysicalConstants::meV;

      // Derive distances and angles
      const double l1 = beam1.norm();
      const double l2 = beam2.norm();
      const double scatteringAngle2 = beam2.angle(beam1);
      const V3D qLabDir = (beam1/l1) - (beam2/l2);

      // Derive the wavelength
      std::vector<double> x;
      const double microSecsInSec = 1e6;
      x.push_back( ( (l1 + l2) / velocity ) * microSecsInSec ); // Make a TOF
      std::vector<double> y;
      Unit_sptr unitOfLambda = UnitFactory::Instance().create("Wavelength");
      unitOfLambda->fromTOF(x, y, l1, l2, scatteringAngle2, 0, efixed, 0);

      // Derive QLab for diffraction
      const double wavenumber_in_angstrom_times_tof_in_microsec =
          (Mantid::PhysicalConstants::NeutronMass * (l1 + l2) * 1e-10 * microSecsInSec) /
           Mantid::PhysicalConstants::h_bar;

      V3D qLab = qLabDir * wavenumber_in_angstrom_times_tof_in_microsec;

      Peak peak; // Everything will be default
      peak.setInstrument(minimalInstrument); // Can't do anything without the instrument
      peak.setQLabFrame(qLab);
      auto detector = peak.getDetector();

      TSM_ASSERT("No detector", detector);
      TS_ASSERT_EQUALS(1, detector->getID());
      TS_ASSERT_EQUALS(detectorPos, detector->getPos());
  }

  /** Create peaks using Q in sample frame + a goniometer rotation matrix*/
  void test_setQSampleFrame()
  {
    // A goniometer rotation matrix
    Matrix<double> r2(3,3,false);
    r2[0][2] = 1;
    r2[1][1] = 1;
    r2[2][0] = -1;

    Peak p1(inst, 19999, 2.0, V3D(1,2,3), r2);
    V3D q = p1.getQSampleFrame();
    V3D detPos1 = p1.getDetPos();

    // Construct using Q + rotation matrix
    Peak p2(inst, q, r2, detPos1.norm());
    p2.setHKL(V3D(1,2,3)); // Make sure HKL matches too.
    comparePeaks(p1, p2);
    TS_ASSERT_EQUALS( p2.getBankName(), "None");
    TS_ASSERT_EQUALS( p2.getRow(), -1);
    TS_ASSERT_EQUALS( p2.getCol(), -1);
    TS_ASSERT_EQUALS( p2.getDetectorID(), -1);
  }


  /** Create peaks using Q in the lab frame,
   * then find the corresponding detector ID */
  void test_findDetector()
  {
    Peak p1(inst, 19999, 2.0);
    V3D Qlab1 = p1.getQLabFrame();
    V3D detPos1 = p1.getDetPos();

    // Construct using just Q
    Peak p2(inst, Qlab1, detPos1.norm());
    TS_ASSERT( p2.findDetector() );
    comparePeaks(p1, p2);
    TS_ASSERT_EQUALS( p2.getBankName(), "bank1");
    TS_ASSERT_EQUALS( p2.getRow(), 99);
    TS_ASSERT_EQUALS( p2.getCol(), 99);
    TS_ASSERT_EQUALS( p2.getDetectorID(), 19999);
  }

  void test_getDetectorPosition()
  {
    const int detectorId = 19999;
    const double wavelength = 2;
    Peak p(inst, detectorId, wavelength);

    V3D a = p.getDetectorPosition();
    V3D b = p.getDetectorPositionNoCheck();

    TSM_ASSERT_EQUALS("Results should be the same", a, b);
  }

  void test_getDetectorPositionThrows()
  {
    const int detectorId = 19999;
    const double wavelength = 2;
    Peak p(inst, detectorId, wavelength);
    TSM_ASSERT_THROWS_NOTHING("Nothing wrong here, detector is valid", p.getDetectorPosition());
    p.setQLabFrame(V3D(1,1,1), 1.0); // This sets the detector pointer to null and detector id to -1;
    TSM_ASSERT_THROWS("Detector is not valid", p.getDetectorPosition(), Mantid::Kernel::Exception::NullPointerException&);
  }

  void test_get_peak_shape_default()
  {
      Peak peak;
      const PeakShape& integratedShape = peak.getPeakShape();
      TS_ASSERT_EQUALS("none", integratedShape.shapeName());
  }

  void test_set_peak_shape()
  {
      using namespace testing;

      Peak peak;

      MockPeakShape* replacementShape = new MockPeakShape;
      EXPECT_CALL(*replacementShape, shapeName()).Times(1);
      peak.setPeakShape(replacementShape);

      const PeakShape& currentShape = peak.getPeakShape();
      currentShape.shapeName();

      TS_ASSERT(Mock::VerifyAndClearExpectations(replacementShape));
  }

private:
  void check_Contributing_Detectors(const Peak & peak, const std::vector<int> & expected)
  {
    auto peakIDs = peak.getContributingDetIDs();
    for(auto it = expected.begin(); it != expected.end(); ++it)
    {
      const int id = *it;
      TSM_ASSERT_EQUALS("Expected " + boost::lexical_cast<std::string>(id) + " in contribution list", 1, peakIDs.count(id))
    }
  }
};


#endif /* MANTID_DATAOBJECTS_PEAKTEST_H_ */

