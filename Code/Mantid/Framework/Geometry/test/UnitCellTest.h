#ifndef MANTID_GEOMETRY_UNITCELLTEST_H_
#define MANTID_GEOMETRY_UNITCELLTEST_H_

#include <cxxtest/TestSuite.h>
#include <MantidKernel/Timer.h>
#include <MantidKernel/System.h>
#include <iostream>
#include <iomanip>
#include <MantidKernel/Matrix.h>

#include <MantidGeometry/Crystal/UnitCell.h>

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::Matrix;


class UnitCellTest : public CxxTest::TestSuite
{
public:

  void testInvalidParametersThrow()
  {
    TSM_ASSERT_THROWS("Should throw if matrix is not invertible!", UnitCell(0, 0, 0, 0, 0, 0), std::range_error);
  }

  void test_Simple()
  {
    // test constructors, access to some of the variables
    UnitCell u1,u2(3,4,5),u3(2,3,4,85.,95.,100),u4;
    u4=u2;
    TS_ASSERT_EQUALS(u1.a1(),1);
    TS_ASSERT_EQUALS(u1.alpha(),90);
    TS_ASSERT_DELTA(u2.b1(),1./3.,1e-10);
    TS_ASSERT_DELTA(u2.alphastar(),90,1e-10);
    TS_ASSERT_DELTA(u4.volume(),1./u2.recVolume(),1e-10);
    u2.seta(3);
    TS_ASSERT_DELTA(u2.a(),3,1e-10);
  }

  void test_Uncertainties()
  {
      UnitCell u(2,3,4,85.,95.,100);
      TS_ASSERT_DELTA(u.errora(),0,1e-10);
      TS_ASSERT_DELTA(u.errorb(),0,1e-10);
      TS_ASSERT_DELTA(u.errorc(),0,1e-10);
      TS_ASSERT_DELTA(u.erroralpha(),0,1e-10);
      TS_ASSERT_DELTA(u.errorbeta(),0,1e-10);
      TS_ASSERT_DELTA(u.errorgamma(),0,1e-10);
      u.setError(0.1,0.2,0.3,5,6,7);
      TS_ASSERT_DELTA(u.errora(),0.1,1e-10);
      TS_ASSERT_DELTA(u.errorb(),0.2,1e-10);
      TS_ASSERT_DELTA(u.errorc(),0.3,1e-10);
      TS_ASSERT_DELTA(u.erroralpha(),5,1e-10);
      TS_ASSERT_DELTA(u.errorbeta(),6,1e-10);
      TS_ASSERT_DELTA(u.errorgamma(),7,1e-10);
      u.setErrora(0.01);
      u.setErrorb(0.02);
      u.setErrorc(0.03);
      u.setErroralpha(0.11);
      u.setErrorbeta(0.12);
      u.setErrorgamma(0.15,angRadians);
      TS_ASSERT_DELTA(u.errora(),0.01,1e-10);
      TS_ASSERT_DELTA(u.errorb(),0.02,1e-10);
      TS_ASSERT_DELTA(u.errorc(),0.03,1e-10);
      TS_ASSERT_DELTA(u.erroralpha(),0.11,1e-10);
      TS_ASSERT_DELTA(u.errorbeta(),0.12,1e-10);
      TS_ASSERT_DELTA(u.errorgamma(angRadians),0.15,1e-10);
  }

  void checkCell(UnitCell & u)
  {
    TS_ASSERT_DELTA(u.a(),2.5,1e-10);
    TS_ASSERT_DELTA(u.b(),6,1e-10);
    TS_ASSERT_DELTA(u.c(),8,1e-10);
    TS_ASSERT_DELTA(u.alpha(),93,1e-10);
    TS_ASSERT_DELTA(u.beta(),88,1e-10);
    TS_ASSERT_DELTA(u.gamma(),97,1e-10);

    // get the some elements of the B matrix
    TS_ASSERT_DELTA(u.getB()[0][0],0.403170877311,1e-10);
    TS_ASSERT_DELTA(u.getB()[2][0],0.0,1e-10);
    TS_ASSERT_DELTA(u.getB()[0][2],-0.00360329991666,1e-10);
    TS_ASSERT_DELTA(u.getB()[2][2],0.125,1e-10);

    // Inverse B matrix
    DblMatrix I = u.getB() * u.getBinv();
    TS_ASSERT_EQUALS( I, Matrix<double>(3,3,true));

    // d spacing for direct lattice at (1,1,1) (will automatically check dstar)
    TS_ASSERT_DELTA(u.d(1.,1.,1.),2.1227107587,1e-10);
    TS_ASSERT_DELTA(u.d(V3D(1.,1.,1.)),2.1227107587,1e-10);
    // angle
    TS_ASSERT_DELTA(u.recAngle(1,1,1,1,0,0,angRadians),0.471054990614,1e-10);
  }

  void test_Advanced()
  {
    // test more advanced calculations
    // the new Gstar shold yield a=2.5, b=6, c=8, alpha=93, beta=88, gamma=97.
    DblMatrix newGstar(3,3);
    newGstar[0][0]=0.162546756312;
    newGstar[0][1]=0.00815256992072;
    newGstar[0][2]=-0.00145274558861;
    newGstar[1][0]=newGstar[0][1];
    newGstar[1][1]=0.028262965555;
    newGstar[1][2]=0.00102046431298;
    newGstar[2][0]=newGstar[0][2];
    newGstar[2][1]=newGstar[1][2];
    newGstar[2][2]=0.0156808990098;

    UnitCell u;
    u.recalculateFromGstar(newGstar);

    // Check the directly-created one
    checkCell(u);

    // Check if copy constructor is also good.
    UnitCell u2 = u;
    checkCell(u2);

  }
  void test_UnitCellCrash(){
    TS_ASSERT_THROWS(UnitCell(10.4165,3.4165,10.4165,30,45,80);,std::invalid_argument);
  }

  void test_printing()
  {
    // w/o uncertainties
    UnitCell cell(2.,3.,4.,80.,90.,100.);
    {
      std::stringstream msg;
      msg << cell;
      TS_ASSERT_EQUALS(msg.str(),
                       "Lattice Parameters:    2.000    3.000    4.000   80.000   90.000  100.000");
    }

    // w/ uncertainties
    cell.setError(1.,2.,3.,4.,5.,6.);
    {
      std::stringstream msg;
      msg << cell;
      TS_ASSERT_EQUALS(msg.str(),
                       "Lattice Parameters:    2.000    3.000    4.000   80.000   90.000  100.000\nParameter Errors  :    1.000    2.000    3.000    4.000    5.000    6.000");
    }
  }

  void testStrToUnitCell()
  {
      UnitCell cell(2.0, 4.0, 5.0, 90.0, 100.0, 102.0);
      std::string cellString = unitCellToStr(cell);
      UnitCell other = strToUnitCell(cellString);

      TS_ASSERT_EQUALS(cell.getG(), other.getG());

      UnitCell precisionLimit(2.1234567891, 3.0, 4.1234567891, 90.0, 90.0, 90.0);
      std::string precisionLimitString = unitCellToStr(precisionLimit);
      UnitCell precisionLimitOther = strToUnitCell(precisionLimitString);

      TS_ASSERT_DIFFERS(precisionLimit.a(), precisionLimitOther.a());
      TS_ASSERT_DELTA(precisionLimit.a(), precisionLimitOther.a(), 1e-9);

      TS_ASSERT_DIFFERS(precisionLimit.c(), precisionLimitOther.c());
      TS_ASSERT_DELTA(precisionLimit.c(), precisionLimitOther.c(), 1e-9);
  }

};


#endif /* MANTID_GEOMETRY_UNITCELLTEST_H_ */

