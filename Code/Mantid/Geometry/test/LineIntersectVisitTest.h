#ifndef MANTID_LINEINTERSECTVISITTEST__
#define MANTID_LINEINTERSECTVISITTEST__

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quadratic.h"
#include "MantidGeometry/Line.h"
#include "MantidGeometry/LineIntersectVisit.h"
#include "MantidGeometry/Plane.h"
#include "MantidGeometry/Sphere.h"
#include "MantidGeometry/Cylinder.h"
#include "MantidGeometry/Cone.h"

using namespace Mantid;
using namespace Geometry;

class testLineIntersectVisit: public CxxTest::TestSuite{
public:
	void testConstructor(){
		LineIntersectVisit A(V3D(-1.0,-1.0,-1.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getNPoints(),0);
		TS_ASSERT_EQUALS(A.getPoints(),std::vector<Geometry::V3D>());
		TS_ASSERT_EQUALS(A.getDistance(),std::vector<double>());
	}

	void testAcceptPlane(){
		LineIntersectVisit A(V3D(-1.0,-1.0,-1.0),V3D(1.0,0.0,0.0));
		Plane B;
		B.setPlane(V3D(0.0,0.0,0.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(extractString(B),"-1 px 0\n");
		A.Accept(B);
		TS_ASSERT_EQUALS(A.getNPoints(),1);
		std::vector<Geometry::V3D> Pnts;
		Pnts.push_back(V3D(0.0,-1.0,-1.0));
		TS_ASSERT_EQUALS(A.getPoints(),Pnts);
		std::vector<double> Dist;
		Dist.push_back(1.0);
		TS_ASSERT_EQUALS(A.getDistance(),Dist);
	}

	void testAcceptSphere(){
		LineIntersectVisit A(V3D(0.0,0.0,0.0),V3D(1.0,0.0,0.0));

		Sphere B;
		B.setSurface("s 0.0 0.0 0.0 2");
		A.Accept(B);
		std::vector<V3D> pntOut;		
		pntOut.push_back(V3D(-2.0,0.0,0.0));
		pntOut.push_back(V3D(2.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getNPoints(),2);
		TS_ASSERT_EQUALS(A.getPoints(),pntOut);
		std::vector<double> Dist;
		Dist.push_back(2.0);
		Dist.push_back(2.0);
		TS_ASSERT_EQUALS(A.getDistance(),Dist);		
	}

	void testAcceptCone(){
		LineIntersectVisit A(V3D(0.0,0.0,0.0),V3D(1.0,0.0,0.0));

		Cone B;
		TS_ASSERT_EQUALS(B.setSurface("k/y 0.0 1.0 0.0 1.0\n"),0);
		TS_ASSERT_EQUALS(B.getCentre(),V3D(0.0,1.0,0.0));
		
		A.Accept(B);
		TS_ASSERT_EQUALS(A.getNPoints(),2);
		std::vector<V3D> pntOut;
		pntOut=A.getPoints();
		TS_ASSERT_DELTA(pntOut[0].X(),-1,0.0000001);
		TS_ASSERT_DELTA(pntOut[0].Y(),0.0,0.0000001);
		TS_ASSERT_DELTA(pntOut[0].Z(),0.0,0.0000001);
		TS_ASSERT_DELTA(pntOut[1].X(),1,0.0000001);
		TS_ASSERT_DELTA(pntOut[1].Y(),0.0,0.0000001);
		TS_ASSERT_DELTA(pntOut[1].Z(),0.0,0.0000001);

		std::vector<double> Dist;
		Dist=A.getDistance();
		TS_ASSERT_DELTA(Dist[0],1.0,0.0000001);
		TS_ASSERT_DELTA(Dist[1],1.0,0.0000001);		
	}

	void testAcceptCylinder(){
		LineIntersectVisit A(V3D(0.0,0.0,0.0),V3D(1.0,0.0,0.0));

		Cylinder B;
		B.setSurface("c/y 0.0 0.0 1.0");
		TS_ASSERT_EQUALS(B.getCentre(),V3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(B.getRadius(),1);
		TS_ASSERT_EQUALS(B.getNormal(),V3D(0,1,0));

		A.Accept(B);
		std::vector<V3D> pntOut;
		pntOut.push_back(V3D(-1.0,0.0,0.0));
		pntOut.push_back(V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getNPoints(),2);
		TS_ASSERT_EQUALS(A.getPoints(),pntOut);
		std::vector<double> Dist;
		Dist.push_back(1.0);
		Dist.push_back(1.0);
		TS_ASSERT_EQUALS(A.getDistance(),Dist);	

		LineIntersectVisit C(V3D(1.1,0.0,0.0),V3D(-1.0,0.0,0.0));
		C.Accept(B);
		TS_ASSERT_EQUALS(C.getNPoints(),2);
		TS_ASSERT_EQUALS(C.getPoints(),pntOut);
	}

	void testAcceptGeneral(){
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
#endif
