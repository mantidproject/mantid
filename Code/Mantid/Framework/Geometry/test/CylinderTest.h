#ifndef MANTID_TESTCYLINDER__
#define MANTID_TESTCYLINDER__

#include <cxxtest/TestSuite.h>
#include <ostream>
#include <vector>
#include <algorithm>
#include <sstream>

#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Surfaces/Cylinder.h"

using namespace Mantid;
using namespace Geometry;

class CylinderTest: public CxxTest::TestSuite
{

public:

  void testConstructor()
  {
    Cylinder A;
    // both centre and radius = 0
    TS_ASSERT_EQUALS(extractString(A),"-1 cx 0\n");
    TS_ASSERT_EQUALS(A.getCentre(),V3D(0,0,0));
    TS_ASSERT_EQUALS(A.getNormal(),V3D(1,0,0));
    TS_ASSERT_EQUALS(A.getRadius(),0);
  }

  void testsetSurface()
  {
    Cylinder A;
    A.setSurface("c/x 0.5 0.5 1.0");
    TS_ASSERT_EQUALS(A.getCentre(),V3D(0,0.5,0.5));
    TS_ASSERT_EQUALS(A.getRadius(),1);
    TS_ASSERT_EQUALS(A.getNormal(),V3D(1,0,0));
    TS_ASSERT_EQUALS(extractString(A),"-1  c/x 0.5 0.5 1\n");
  }

  void testCopyConstructor()
  {
    Cylinder A;
    A.setSurface("c/x 0.5 0.5 1.0");
    TS_ASSERT_EQUALS(extractString(A),"-1  c/x 0.5 0.5 1\n");
    Cylinder B(A);
    TS_ASSERT_EQUALS(extractString(B),extractString(A));
  }

  void testClone()
  {
    Cylinder A;
    A.setSurface("c/x 0.5 0.5 1.0");
    TS_ASSERT_EQUALS(extractString(A),"-1  c/x 0.5 0.5 1\n");
    Cylinder* B = A.clone();
    TS_ASSERT_EQUALS(extractString(*B),extractString(A));
    delete B;
  }

  void testAssignment()
  {
    Cylinder A,B;
    A.setSurface("c/x 0.5 0.5 1.0");
    TS_ASSERT_DIFFERS(extractString(B),extractString(A));
    B=A;
    TS_ASSERT_EQUALS(extractString(B),extractString(A));
  }

  /// is a point inside outside or on the side!
  void testSide()
  {    
    Cylinder A;
    //radius 2 at the origin
    A.setSurface("cx 2.0");

    //Origin should be inside
    TS_ASSERT_EQUALS(A.side(V3D(0,0,0)),-1);
    TS_ASSERT_EQUALS(A.side(V3D(1.9,0,0)),-1);
    TS_ASSERT_EQUALS(A.side(V3D(0,1.9,0)),-1);
    TS_ASSERT_EQUALS(A.side(V3D(0,0,1.9)),-1);
    TS_ASSERT_EQUALS(A.side(V3D(0,0,-1.9)),-1);
    TS_ASSERT_EQUALS(A.side(V3D(-1.9,0,0)),-1);
    TS_ASSERT_EQUALS(A.side(V3D(0,-1.9,0)),-1);

    //all these are inside - infinite Cylinder on x
    TS_ASSERT_EQUALS(A.side(V3D(2,0,0)),-1);
    TS_ASSERT_EQUALS(A.side(V3D(-2,0,0)),-1);
    TS_ASSERT_EQUALS(A.side(V3D(2.1,0,0)),-1);
    TS_ASSERT_EQUALS(A.side(V3D(-2.1,0,0)),-1);

    //should be on the side
    TS_ASSERT_EQUALS(A.side(V3D(0,2,0)),0);
    TS_ASSERT_EQUALS(A.side(V3D(0,0,2)),0);
    TS_ASSERT_EQUALS(A.side(V3D(0,0,-2)),0);
    TS_ASSERT_EQUALS(A.side(V3D(0,-2,0)),0);
	// test tolerance at default 1e-6
    TS_ASSERT_EQUALS(A.side(V3D(0,-2+1e-7,0)),0);
    TS_ASSERT_EQUALS(A.side(V3D(0,-2-1e-7,0)),0);
    TS_ASSERT_EQUALS(A.side(V3D(0,-2-2e-6,0)),1);
    TS_ASSERT_EQUALS(A.side(V3D(0,-2+2e-6,0)),-1);
    //should be outside
    TS_ASSERT_EQUALS(A.side(V3D(0,2.1,0)),1);
    TS_ASSERT_EQUALS(A.side(V3D(0,0,2.1)),1);
    TS_ASSERT_EQUALS(A.side(V3D(0,-2.1,0)),1);
    TS_ASSERT_EQUALS(A.side(V3D(0,0,-2.1)),1);
    TS_ASSERT_EQUALS(A.side(V3D(0,0.1,2)),1);
  }

    /// is a point inside outside or on the side!
  void testOnSurface()
  {    
    Cylinder A;
    //radius 2 at the origin
    A.setSurface("cx 2.0");
    TS_ASSERT_EQUALS(extractString(A),"-1 cx 2\n");

    //inside
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,0,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(V3D(1.9,0,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,1.9,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,0,1.9)),0);
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,0,-1.9)),0);
    TS_ASSERT_EQUALS(A.onSurface(V3D(-1.9,0,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,-1.9,0)),0);

    //all these are inoside - infinite Cylinder on x
    TS_ASSERT_EQUALS(A.onSurface(V3D(2,0,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(V3D(-2,0,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(V3D(2.1,0,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(V3D(-2.1,0,0)),0);

    //should be on the surface
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,2,0)),1);
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,0,2)),1);
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,0,-2)),1);
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,-2,0)),1);
	// test tolerance at default 1e-6
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,-2+1e-7,0)),1);
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,-2-1e-7,0)),1);
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,-2-2e-6,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,-2+2e-6,0)),0);
    //should be outside
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,2.1,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,0,2.1)),0);
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,-2.1,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,0,-2.1)),0);
    TS_ASSERT_EQUALS(A.onSurface(V3D(0,0.1,2)),0);
  }
  

  void testCylinderDistance()
  {
    Cylinder A;
    A.setSurface("cx 5");// infinite cylinder along, radius 5

    //exactly centre
    TS_ASSERT_DELTA(A.distance(V3D(5.1,0,0)),5.0,1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(-5.1,0,0)),5.0,1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(4.9,0,0)),5.0,1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(-4.9,0,0)),5.0,1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(100,0,0)),5,1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(-100,0,0)),5,1e-5);

    //just outside
    TS_ASSERT_DELTA(A.distance(V3D(0,5.1,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0,0,5.1)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0,-5.1,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0,0,-5.1)),0.1,1e-5);

    //just inside
    TS_ASSERT_DELTA(A.distance(V3D(0,4.9,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0,0,4.9)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0,-4.9,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0,0,-4.9)),0.1,1e-5);

    //distant
    TS_ASSERT_DELTA(A.distance(V3D(0,100,0)),95,1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0,0,100)),95,1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0,-100,0)),95,1e-5);
    TS_ASSERT_DELTA(A.distance(V3D(0,0,-100)),95,1e-5);
  }

  void testCylinderDistanceComplex()
    /**
    Test the distance of a point from the cylinder
    */
  {
    std::vector<std::string> CylStr;
    CylStr.push_back("cx 1");                // Cylinder origin
    CylStr.push_back("c/x 1.0 1.0 1.0");     // also cylinder at ?origin?
    Geometry::V3D P(0,-1.2,0);
    double results[]={ 1.2-1,  1.41661 };

    std::vector<std::string>::const_iterator vc;
    Cylinder A;
    int cnt(0);
    for(vc=CylStr.begin();vc!=CylStr.end();vc++,cnt++)
    { 
      TS_ASSERT_EQUALS(A.setSurface(*vc),0);
      if (fabs(A.distance(P)-results[cnt])>1e-6)
      {
        TS_ASSERT_DELTA(A.distance(P),results[cnt],0.0001);
        TS_ASSERT_EQUALS(A.setSurface(*vc),0);
        std::cout<<"Cylinder == ";
        A.write(std::cout);
        std::cout<<"TestPoint == "<<P<<std::endl;
        std::cout<<"Distance == "<<A.distance(P)<<std::endl;;
      }
    }
  }
  void testSurfaceNormal()
  {
    Cylinder A;
    A.setSurface("cx 5");

    TS_ASSERT_EQUALS(A.surfaceNormal(V3D(10,0,0)),V3D(0,0,0));
    TS_ASSERT_EQUALS(A.surfaceNormal(V3D(0,10,0)),V3D(0,1,0));
    TS_ASSERT_EQUALS(A.surfaceNormal(V3D(0,0,10)),V3D(0,0,1));
    TS_ASSERT_EQUALS(A.surfaceNormal(V3D(-10,0,0)),V3D(0,0,0));
    TS_ASSERT_EQUALS(A.surfaceNormal(V3D(0,-10,0)),V3D(0,-1,0));
    TS_ASSERT_EQUALS(A.surfaceNormal(V3D(0,0,-10)),V3D(0,0,-1));
    
    V3D result = A.surfaceNormal(V3D(0,10,10));
    TS_ASSERT_DELTA(result.X(), 0.0,1e-5);
    TS_ASSERT_DELTA(result.Y(), 0.7071,1e-5);
    TS_ASSERT_DELTA(result.Z(), 0.7071,1e-5);
  }

  void testSetCentre()
  {
    Cylinder A;
    // centre at origin and radius = 2
    TS_ASSERT_EQUALS(extractString(A),"-1 cx 0\n");
    TS_ASSERT_EQUALS(A.getCentre(),V3D(0,0,0));
    TS_ASSERT_EQUALS(A.getNormal(),V3D(1,0,0));
    TS_ASSERT_EQUALS(A.getRadius(),0);

    V3D point(1,1,1);
    A.setCentre(point);    
    TS_ASSERT_EQUALS(extractString(A),"-1  c/x 1 1 0\n");
    TS_ASSERT_EQUALS(A.getCentre(),point);
    TS_ASSERT_EQUALS(A.getNormal(),V3D(1,0,0));
    TS_ASSERT_EQUALS(A.getRadius(),0);

    V3D point2(-12.1,51.6,-563.1);
    A.setCentre(point2);    
    TS_ASSERT_EQUALS(extractString(A),"-1  c/x 51.6 -563.1 0\n");
    TS_ASSERT_EQUALS(A.getCentre(),point2);
    TS_ASSERT_EQUALS(A.getNormal(),V3D(1,0,0));
    TS_ASSERT_EQUALS(A.getRadius(),0);
  }

  void testSetNorm()
  {
    Cylinder A;
    // centre at origin and radius = 2
    TS_ASSERT_EQUALS(extractString(A),"-1 cx 0\n");
    TS_ASSERT_EQUALS(A.getCentre(),V3D(0,0,0));
    TS_ASSERT_EQUALS(A.getNormal(),V3D(1,0,0));
    TS_ASSERT_EQUALS(A.getRadius(),0);

    V3D point(0,1,0);
    A.setNorm(point);    
    TS_ASSERT_EQUALS(extractString(A),"-1 cy 0\n");
    TS_ASSERT_EQUALS(A.getCentre(),V3D(0,0,0));
    TS_ASSERT_EQUALS(A.getNormal(),point);
    TS_ASSERT_EQUALS(A.getRadius(),0);

    V3D point2(0,0,1);
    A.setNorm(point2);    
    TS_ASSERT_EQUALS(extractString(A),"-1 cz 0\n");
    TS_ASSERT_EQUALS(A.getCentre(),V3D(0,0,0));
    TS_ASSERT_EQUALS(A.getNormal(),point2);
    TS_ASSERT_EQUALS(A.getRadius(),0);

    V3D point3(0.5,0,0);
    A.setNorm(point3);    
    TS_ASSERT_EQUALS(extractString(A),"-1 cx 0\n");
    TS_ASSERT_EQUALS(A.getCentre(),V3D(0,0,0));
    TS_ASSERT_EQUALS(A.getNormal(),V3D(1,0,0));
    TS_ASSERT_EQUALS(A.getRadius(),0);
  }

  void testGetBoundingBox()
  {
    Cylinder A;
    A.setSurface("cx 5");
	double xmax,ymax,zmax,xmin,ymin,zmin;
	xmax=ymax=zmax=100;
	xmin=ymin=zmin=-100;
	A.getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
	TS_ASSERT_DELTA(xmax,100,0.0001);
	TS_ASSERT_DELTA(ymax,  5,0.0001);
	TS_ASSERT_DELTA(zmax,  5,0.0001);
	TS_ASSERT_DELTA(xmin,-100,0.0001);
	TS_ASSERT_DELTA(ymin, -5,0.0001);
	TS_ASSERT_DELTA(zmin, -5,0.0001);
  }

  void testEvalValue()
  {
	Cylinder A;
	A.setCentre(V3D(0.0,0.0,0.0));
	A.setNorm(V3D(1.0,0.0,0.0));
	A.setRadius(1.0);
	TS_ASSERT_DELTA(A.eqnValue(V3D(0.0,0.0,0.0)),-1.0,0.0001);
  }

private:

  std::string extractString(Surface& pv)
  {
    //dump output to sting
    std::ostringstream output;
    output.exceptions( std::ios::failbit | std::ios::badbit );
    TS_ASSERT_THROWS_NOTHING(pv.write(output));
    return output.str();
  }

};

#endif //MANTID_TESTCYLINDER__
