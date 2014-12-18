#ifndef MANTID_LINEINTERSECTVISITTEST__
#define MANTID_LINEINTERSECTVISITTEST__

#include <cxxtest/TestSuite.h>
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Line.h"
#include "MantidGeometry/Surfaces/LineIntersectVisit.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Cone.h"

using namespace Mantid;
using namespace Geometry;
using Mantid::Kernel::V3D;

class LineIntersectVisitTest: public CxxTest::TestSuite{
public:
	void testConstructor(){
		LineIntersectVisit A(V3D(-1.0,-1.0,-1.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getNPoints(),0);
		TS_ASSERT_EQUALS(A.getPoints(),std::list<Kernel::V3D>());
		TS_ASSERT_EQUALS(A.getDistance(),std::list<double>());
	}

	void testAcceptPlane(){
		LineIntersectVisit A(V3D(-1.0,-1.0,-1.0),V3D(1.0,0.0,0.0));
		Plane B;
		B.setPlane(V3D(0.0,0.0,0.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(extractString(B),"-1 px 0\n");
		A.Accept(B);
		TS_ASSERT_EQUALS(A.getNPoints(),1);
		std::list<Kernel::V3D> Pnts;
		Pnts.push_back(V3D(0.0,-1.0,-1.0));
		TS_ASSERT_EQUALS(A.getPoints(),Pnts);
		std::list<double> Dist;
		Dist.push_back(1.0);
		TS_ASSERT_EQUALS(A.getDistance(),Dist);
	}

	void testAcceptSphere(){
		LineIntersectVisit A(V3D(0.0,0.0,0.0),V3D(1.0,0.0,0.0));

		Sphere B;
		B.setSurface("s 0.0 0.0 0.0 2");
		A.Accept(B);
		std::list<V3D> pntOut;
		// changed for forward going only intercepts on quadratice surfaces
		//pntOut.push_back(V3D(-2.0,0.0,0.0));
		pntOut.push_back(V3D(2.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getNPoints(),1);
		TS_ASSERT_EQUALS(A.getPoints(),pntOut);
		std::list<double> Dist;
		Dist.push_back(2.0);
		TS_ASSERT_EQUALS(A.getDistance(),Dist);		
	}

	void testAcceptCone(){
		LineIntersectVisit A(V3D(0.0,0.0,0.0),V3D(1.0,0.0,0.0));

		Cone B;
		TS_ASSERT_EQUALS(B.setSurface("k/y 0.0 1.0 0.0 1.0\n"),0);
		TS_ASSERT_EQUALS(B.getCentre(),V3D(0.0,1.0,0.0));
		
		A.Accept(B);
		// change for forward only intercept
		TS_ASSERT_EQUALS(A.getNPoints(),1);
		const auto &pntOut = A.getPoints();
		TS_ASSERT_DELTA(pntOut.front().X(),1,0.0000001);
		TS_ASSERT_DELTA(pntOut.front().Y(),0.0,0.0000001);
		TS_ASSERT_DELTA(pntOut.front().Z(),0.0,0.0000001);

		const auto &Dist = A.getDistance();
		TS_ASSERT_DELTA(Dist.front(),1.0,0.0000001);
	}

	void testAcceptCylinder(){
		LineIntersectVisit A(V3D(0.0,0.0,0.0),V3D(1.0,0.0,0.0));

		Cylinder B;
		B.setSurface("c/y 0.0 0.0 1.0");
		TS_ASSERT_EQUALS(B.getCentre(),V3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(B.getRadius(),1);
		TS_ASSERT_EQUALS(B.getNormal(),V3D(0,1,0));

		A.Accept(B);
		std::list<V3D> pntOut;
		// forward only
		//pntOut.push_back(V3D(-1.0,0.0,0.0));
		pntOut.push_back(V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getNPoints(),1);
		TS_ASSERT_EQUALS(A.getPoints(),pntOut);
		std::list<double> Dist;
		//Dist.push_back(1.0);
		Dist.push_back(1.0);
		TS_ASSERT_EQUALS(A.getDistance(),Dist);	

		LineIntersectVisit C(V3D(1.1,0.0,0.0),V3D(-1.0,0.0,0.0));
		C.Accept(B);
		TS_ASSERT_EQUALS(C.getNPoints(),2);
		std::list<V3D> pntOut2;
		pntOut2.push_back(V3D(-1.0,0.0,0.0));
		pntOut2.push_back(V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(C.getPoints(),pntOut2);
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
