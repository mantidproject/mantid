#ifndef MANTID_GEOMETRY_UNITCELLTEST_H_
#define MANTID_GEOMETRY_UNITCELLTEST_H_

#include <cxxtest/TestSuite.h>
#include <MantidKernel/Timer.h>
#include <MantidKernel/System.h>
#include <iostream>
#include <iomanip>
#include <MantidGeometry/Math/Matrix.h>
#include <MantidGeometry/Crystal/UnitCell.h>

using namespace Mantid::Geometry;

class UnitCellTest : public CxxTest::TestSuite
{
public:

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

  void test_Advanced()
  {
    // test more advanced calculations
    // the new Gstar shold yield a=2.5, b=6, c=8, alpha=93, beta=88, gamma=97.
    MantidMat newGstar(3,3);
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
    // d spacing for direct lattice at (1,1,1) (will automatically check dstar)
    TS_ASSERT_DELTA(u.d(1.,1.,1.),2.1227107587,1e-10);
    // angle
    TS_ASSERT_DELTA(u.recAngle(1,1,1,1,0,0,latRadians),0.471054990614,1e-10);
  }
};


#endif /* MANTID_GEOMETRY_UNITCELLTEST_H_ */

