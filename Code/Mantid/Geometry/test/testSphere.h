#ifndef MANTID_TESTSPHERE__
#define MANTID_TESTSPHERE__

#include <cxxtest/TestSuite.h>
#include <ostream>
#include <vector>
#include <algorithm>
#include <sstream>

#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"
#include "../inc/IndexIterator.h"
#include "../inc/Vec3D.h"
#include "../inc/Quadratic.h"
#include "../inc/Sphere.h"

using namespace Mantid;
using namespace Geometry;

class testSphere: public CxxTest::TestSuite
{

public:

  void testConstructor()
  {
    Sphere A;
    // both centre and radius = 0
    TS_ASSERT_EQUALS(extractString(A),"-1 so 0\n");
    TS_ASSERT_EQUALS(A.getCentre(),Vec3D(0,0,0));
    TS_ASSERT_EQUALS(A.getRadius(),0);
  }

  void testsetSurface()
  {
    Sphere A;
    A.setSurface("s 1.1 -2.1 1.1 2");
    TS_ASSERT_EQUALS(A.getCentre(),Vec3D(1.1,-2.1,1.1));
    TS_ASSERT_EQUALS(A.getRadius(),2);
    TS_ASSERT_EQUALS(extractString(A),"-1 s 1.1 -2.1 1.1 2\n");
  }

  void testCopyConstructor()
  {
    Sphere A;
    A.setSurface("s 1.1 -2.1 1.1 2");
    TS_ASSERT_EQUALS(extractString(A),"-1 s 1.1 -2.1 1.1 2\n");
    Sphere B(A);
    TS_ASSERT_EQUALS(extractString(B),"-1 s 1.1 -2.1 1.1 2\n");
  }

  void testClone()
  {
    Sphere A;
    A.setSurface("s 1.1 -2.1 1.1 2");
    TS_ASSERT_EQUALS(extractString(A),"-1 s 1.1 -2.1 1.1 2\n");
    Sphere* B = A.clone();
    TS_ASSERT_EQUALS(extractString(*B),"-1 s 1.1 -2.1 1.1 2\n");
    delete B;
  }

  void testAssignment()
  {
    Sphere A,B;
    A.setSurface("s 1.1 -2.1 1.1 2");
    TS_ASSERT_DIFFERS(extractString(B),extractString(A));
    B=A;
    TS_ASSERT_EQUALS(extractString(B),extractString(A));
  }

  /// is a point inside outside or on the side!
  void testSide()
  {    
    Sphere A;
    //radius 2 at the origin
    A.setSurface("so 2");
    TS_ASSERT_EQUALS(extractString(A),"-1 so 2\n");

    //Origin should be inside
    TS_ASSERT_EQUALS(A.side(Vec3D(0,0,0)),-1);
    TS_ASSERT_EQUALS(A.side(Vec3D(1.9,0,0)),-1);
    TS_ASSERT_EQUALS(A.side(Vec3D(0,1.9,0)),-1);
    TS_ASSERT_EQUALS(A.side(Vec3D(0,0,1.9)),-1);
    TS_ASSERT_EQUALS(A.side(Vec3D(0,0,-1.9)),-1);
    TS_ASSERT_EQUALS(A.side(Vec3D(-1.9,0,0)),-1);
    TS_ASSERT_EQUALS(A.side(Vec3D(0,-1.9,0)),-1);

    //should be on the side
    TS_ASSERT_EQUALS(A.side(Vec3D(2,0,0)),0);
    TS_ASSERT_EQUALS(A.side(Vec3D(0,2,0)),0);
    TS_ASSERT_EQUALS(A.side(Vec3D(0,0,2)),0);
    TS_ASSERT_EQUALS(A.side(Vec3D(0,0,-2)),0);
    TS_ASSERT_EQUALS(A.side(Vec3D(-2,0,0)),0);
    TS_ASSERT_EQUALS(A.side(Vec3D(0,-2,0)),0);
    //should be outside
    TS_ASSERT_EQUALS(A.side(Vec3D(2.1,0,0)),1);
    TS_ASSERT_EQUALS(A.side(Vec3D(0,2.1,0)),1);
    TS_ASSERT_EQUALS(A.side(Vec3D(0,0,2.1)),1);
    TS_ASSERT_EQUALS(A.side(Vec3D(-2.1,0,0)),1);
    TS_ASSERT_EQUALS(A.side(Vec3D(0,-2.1,0)),1);
    TS_ASSERT_EQUALS(A.side(Vec3D(0,0,-2.1)),1);
    TS_ASSERT_EQUALS(A.side(Vec3D(2,0.1,0)),1);
    TS_ASSERT_EQUALS(A.side(Vec3D(0.1,2,0)),1);
    TS_ASSERT_EQUALS(A.side(Vec3D(0,0.1,2)),1);
  }

    /// is a point inside outside or on the side!
  void testOnSurface()
  {    
    Sphere A;
    //radius 2 at the origin
    A.setSurface("so 2");
    TS_ASSERT_EQUALS(extractString(A),"-1 so 2\n");

    //Origin should be inonSurface
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0,0,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(1.9,0,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0,1.9,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0,0,1.9)),0);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0,0,-1.9)),0);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(-1.9,0,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0,-1.9,0)),0);

    //should be on the onSurface
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(2,0,0)),1);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0,2,0)),1);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0,0,2)),1);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0,0,-2)),1);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(-2,0,0)),1);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0,-2,0)),1);
    //should be outonSurface
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(2.1,0,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0,2.1,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0,0,2.1)),0);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(-2.1,0,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0,-2.1,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0,0,-2.1)),0);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(2,0.1,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0.1,2,0)),0);
    TS_ASSERT_EQUALS(A.onSurface(Vec3D(0,0.1,2)),0);
  }
  

  void testSphereDistance()
  {
    Sphere A;
    A.setSurface("so 5");// sphere at origin radius 5

    //just outside
    TS_ASSERT_DELTA(A.distance(Vec3D(5.1,0,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,5.1,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,0,5.1)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(-5.1,0,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,-5.1,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,0,-5.1)),0.1,1e-5);

    //just inside
    TS_ASSERT_DELTA(A.distance(Vec3D(4.9,0,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,4.9,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,0,4.9)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(-4.9,0,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,-4.9,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,0,-4.9)),0.1,1e-5);

    //distant
    TS_ASSERT_DELTA(A.distance(Vec3D(100,0,0)),95,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,100,0)),95,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,0,100)),95,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(-100,0,0)),95,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,-100,0)),95,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,0,-100)),95,1e-5);
  }

  void testSphereDistanceTrue()
  {
    Sphere A;
    A.setSurface("so 5");// sphere at origin radius 5

    //just outside
    TS_ASSERT_DELTA(A.distance(Vec3D(5.1,0,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,5.1,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,0,5.1)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(-5.1,0,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,-5.1,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,0,-5.1)),0.1,1e-5);

    //just inside
    TS_ASSERT_DELTA(A.distance(Vec3D(4.9,0,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,4.9,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,0,4.9)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(-4.9,0,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,-4.9,0)),0.1,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,0,-4.9)),0.1,1e-5);

    //distant
    TS_ASSERT_DELTA(A.distance(Vec3D(100,0,0)),95,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,100,0)),95,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,0,100)),95,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(-100,0,0)),95,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,-100,0)),95,1e-5);
    TS_ASSERT_DELTA(A.distance(Vec3D(0,0,-100)),95,1e-5);
  }

  void testSphereDistanceComplex()
    /*!
    Test the distance of a point from the cone
    \retval -1 :: failed build a cone
    \retval -2 :: Failed on the distance calculation
    \retval 0 :: All passed
    */
  {
    std::vector<std::string> SphStr;
    SphStr.push_back("so 1");                // sphere origin
    SphStr.push_back("s 1.5 -2.5 1.8 1");     // sphere 
    Geometry::Vec3D P(3,7,4);
    Geometry::Vec3D Q(0,0,4);
    std::vector<std::string>::const_iterator vc;
    Sphere A;

    for(vc=SphStr.begin();vc!=SphStr.end();vc++)
    {
      const int retVal=A.setSurface(*vc);
      TS_ASSERT(retVal==0);
      // Surface distance and Sphere distance are the same:
      if (fabs(A.distance(P)-A.distance(P))>1e-6)
      {
        TS_ASSERT_DELTA(A.distance(P),A.distance(P),1e-6);
        std::cout<<"Sphere == ";
        A.Surface::write(std::cout);
        std::cout<<"TestPoint == "<<P<<std::endl;
        std::cout<<"Distance == "<<A.distance(P)
          <<" === "<<A.distance(P)<<std::endl;
        std::cout<<"--------------"<<std::endl;
        std::cout<<"Distance == "<<A.distance(Q)
          <<" === "<<A.distance(Q)<<std::endl;
      }
    }
  }

  void testSurfaceNormal()
  {
    Sphere A;
    A.setSurface("so 5");

    TS_ASSERT_EQUALS(A.surfaceNormal(Vec3D(10,0,0)),Vec3D(1,0,0));
    TS_ASSERT_EQUALS(A.surfaceNormal(Vec3D(0,10,0)),Vec3D(0,1,0));
    TS_ASSERT_EQUALS(A.surfaceNormal(Vec3D(0,0,10)),Vec3D(0,0,1));
    TS_ASSERT_EQUALS(A.surfaceNormal(Vec3D(-10,0,0)),Vec3D(-1,0,0));
    TS_ASSERT_EQUALS(A.surfaceNormal(Vec3D(0,-10,0)),Vec3D(0,-1,0));
    TS_ASSERT_EQUALS(A.surfaceNormal(Vec3D(0,0,-10)),Vec3D(0,0,-1));
    
    Vec3D result = A.surfaceNormal(Vec3D(10,10,0));
    TS_ASSERT_DELTA(result.X(), 0.7071,1e-5);
    TS_ASSERT_DELTA(result.Y(), 0.7071,1e-5);
    TS_ASSERT_DELTA(result.Z(), 0.0,1e-5);
  }

  void testSetCentre()
  {
    Sphere A;
    // centre at origin and radius = 2
    TS_ASSERT_EQUALS(extractString(A),"-1 so 0\n");
    TS_ASSERT_EQUALS(A.getCentre(),Vec3D(0,0,0));
    TS_ASSERT_EQUALS(A.getRadius(),0);

    Vec3D point(1,1,1);
    A.setCentre(point);    
    TS_ASSERT_EQUALS(extractString(A),"-1 s 1 1 1 0\n");
    TS_ASSERT_EQUALS(A.getCentre(),point);
    TS_ASSERT_EQUALS(A.getRadius(),0);

    Vec3D point2(-12.1,51.6,-563.1);
    A.setCentre(point2);    
    TS_ASSERT_EQUALS(extractString(A),"-1 s -12.1 51.6 -563.1 0\n");
    TS_ASSERT_EQUALS(A.getCentre(),point2);
    TS_ASSERT_EQUALS(A.getRadius(),0);
  }

private:

std::string extractString(const Surface& pv)
  {
    //dump output to sting
    std::ostringstream output;
    output.exceptions( std::ios::failbit | std::ios::badbit );
    TS_ASSERT_THROWS_NOTHING(pv.write(output));
    return output.str();
  }

};

#endif //MANTID_TESTSPHERE__
