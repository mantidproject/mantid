#ifndef MANTID_TESTGONIOMETER__
#define MANTID_TESTGONIOMETER__

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/Instrument/Goniometer.h"
#include <iostream>
#include <stdexcept>
#include <string>

using namespace Mantid::Geometry;

class GoniometerTest : public CxxTest::TestSuite
{
public:
  void test_AxisConstructor()
  {
    GoniometerAxis a("axis1",V3D(1.,0,0),3.,CW,angRadians);
    TS_ASSERT_EQUALS(a.name,"axis1");
    TS_ASSERT_EQUALS(a.rotationaxis[0],1.);
    TS_ASSERT_EQUALS(a.angle,3.);
    TS_ASSERT_EQUALS(a.sense,-1);
    TS_ASSERT_DIFFERS(a.sense,angDegrees);
  }

  void test_Goniometer()
  {
    Goniometer G;
    MantidMat M(3,3);
    
    // Check simple constructor    
    M.identityMatrix();
    TS_ASSERT_EQUALS(G.getR(),M);
    TS_ASSERT_THROWS(G.setRotationAngle("Axis4",3),std::invalid_argument);
    TS_ASSERT_THROWS_ANYTHING(G.setRotationAngle(1,2));
    TS_ASSERT_EQUALS((G.axesInfo()).compare("No axis is found\n"),0); 
    TS_ASSERT_THROWS_NOTHING(G.pushAxis("Axis1", 1., 0., 0.,30));
    TS_ASSERT_THROWS_NOTHING(G.pushAxis("Axis2", 0., 0., 1.,30));
    TS_ASSERT_THROWS(G.pushAxis("Axis2", 0., 0., 1.,30),std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(G.setRotationAngle("Axis2", 25));
    TS_ASSERT_THROWS_NOTHING(G.setRotationAngle(0, -17));
    TS_ASSERT_EQUALS(G.getAxis(1).angle,25.);
    TS_ASSERT_EQUALS(G.getAxis("Axis1").angle,-17);
    TS_ASSERT_DELTA(G.axesInfo().find("-17"),52,20);
    M=G.getR();
    //test some matrix elements
    TS_ASSERT_DELTA(M[0][0],9.063078e-01,1e-6);
    TS_ASSERT_DELTA(M[0][1],-4.226183e-01,1e-6);
    TS_ASSERT_DELTA(M[0][2],0,1e-6);
    TS_ASSERT_DELTA(M[1][1],8.667064e-01,1e-6);
    TS_ASSERT_DELTA(M[1][2],2.923717e-01,1e-6);
    //goniometer from a rotation matrix or copied
    Goniometer G1(M),G2(G);
    TS_ASSERT_EQUALS(M,G1.getR());
    TS_ASSERT_EQUALS(G1.axesInfo(),std::string("Goniometer was initialized from a rotation matrix. No information about axis is available.\n"));
    TS_ASSERT_THROWS_ANYTHING(G.pushAxis("Axis2", 0., 0., 1.,30));
    TS_ASSERT_EQUALS(M,G2.getR());
  }

  void test_makeUniversalGoniometer()
  {
    Goniometer G;
    G.makeUniversalGoniometer();
    TS_ASSERT_EQUALS(G.getNumberAxes(), 3);
    TS_ASSERT_EQUALS(G.getAxis(0).name, "phi");
    TS_ASSERT_EQUALS(G.getAxis(1).name, "chi");
    TS_ASSERT_EQUALS(G.getAxis(2).name, "omega");
  }

  void test_copy()
  {
    Goniometer G, G1;
    G1.makeUniversalGoniometer();
    G = G1;
    TS_ASSERT_EQUALS(G.getNumberAxes(), 3);
    TS_ASSERT_EQUALS(G.getAxis(0).name, "phi");
    TS_ASSERT_EQUALS(G.getAxis(1).name, "chi");
    TS_ASSERT_EQUALS(G.getAxis(2).name, "omega");
  }
};

#endif
