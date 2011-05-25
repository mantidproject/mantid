#ifndef MANTID_DATAOBJECTS_PEAKTEST_H_
#define MANTID_DATAOBJECTS_PEAKTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataObjects/Peak.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class PeakTest : public CxxTest::TestSuite
{
public:
  /// Common instrument
  IInstrument_sptr inst;
  void setUp()
  {
    inst = ComponentCreationHelper::createTestInstrumentRectangular(5, 100);
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
    TS_ASSERT_EQUALS(p.getDetector(), p2.getDetector())
    TS_ASSERT_EQUALS(p.getInstrument(), p2.getInstrument())
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

  void test_runNumber()
  {
    Peak p(inst, 10000, 2.0);
    p.setRunNumber(12345);
    TS_ASSERT_EQUALS( p.getRunNumber(), 12345);
  }

  void test_GoniometerMatrix()
  {
    Peak p(inst, 10000, 2.0);
    Matrix<double> mat(3,3);
    for (int x=0; x<3; x++)
      for (int y=0; y<3; y++)
        mat[x][y]=x+y;
    p.setGoniometerMatrix(mat);
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

};


#endif /* MANTID_DATAOBJECTS_PEAKTEST_H_ */

