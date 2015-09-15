#ifndef MANTID_LINETEST__
#define MANTID_LINETEST__
#include <cxxtest/TestSuite.h>
#include <cmath>
#include <vector>
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Line.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Plane.h"

using namespace Mantid;
using namespace Geometry;
using Mantid::Kernel::V3D;
using Mantid::Kernel::Matrix;


class LineTest: public CxxTest::TestSuite
{
public:
	void testConstructor(){
		Line A;
		TS_ASSERT_EQUALS(A.getOrigin(),V3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getDirect(),V3D(0.0,0.0,0.0));
	}

	void testParamConstructor(){
		Line A(V3D(1.0,1.0,1.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),V3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(A.getDirect(),V3D(1.0,0.0,0.0));
	}

	void testLineConstructor(){
		Line A(V3D(1.0,1.0,1.0),V3D(1.0,0.0,0.0));
		Line B(A);
		TS_ASSERT_EQUALS(B.getOrigin(),V3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(B.getDirect(),V3D(1.0,0.0,0.0));
	}

	void testAssignment(){
		Line A(V3D(1.0,1.0,1.0),V3D(1.0,0.0,0.0));
		Line B;
		TS_ASSERT_DIFFERS(B.getOrigin(),V3D(1.0,1.0,1.0));
		TS_ASSERT_DIFFERS(B.getDirect(),V3D(1.0,0.0,0.0));
		B=A;
		TS_ASSERT_EQUALS(B.getOrigin(),V3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(B.getDirect(),V3D(1.0,0.0,0.0));
	}

	void testGetPoint(){
		Line A(V3D(1.0,1.0,1.0),V3D(1.0,0.0,0.0));
		//0 lambda should return origin
		TS_ASSERT_EQUALS(A.getPoint(0.0),V3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(A.getPoint(-1.0),V3D(0.0,1.0,1.0));
		TS_ASSERT_EQUALS(A.getPoint(1.0),V3D(2.0,1.0,1.0));
	}

	void testDistance(){
		Line A(V3D(1.0,1.0,1.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.distance(V3D(0.0,0.0,0.0)),sqrt(2.0));
		TS_ASSERT_EQUALS(A.distance(V3D(1.0,0.0,0.0)),sqrt(2.0));
		TS_ASSERT_EQUALS(A.distance(V3D(1.0,1.0,0.0)),1.0);
	}

	void makeMatrix(Matrix<double>& A) const
	{
		A.setMem(3,3);
		A[0][0]=1.0;
		A[1][0]=0.0; 
		A[0][1]=0.0;
		A[0][2]=0.0;
		A[2][0]=0.0;
		A[1][1]=cos(90.0*M_PI/180.0);
		A[2][1]=-sin(90.0*M_PI/180.0);
		A[1][2]=sin(90.0*M_PI/180.0);
		A[2][2]=cos(90.0*M_PI/180.0);
		return;
	}

	void testRotate(){
		Line A(V3D(1.0,1.0,1.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),V3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(A.getDirect(),V3D(1.0,0.0,0.0));

		Matrix<double> rotMat(3,3);
		makeMatrix(rotMat);
		A.rotate(rotMat);
		TS_ASSERT_EQUALS(A.getOrigin(),V3D(1.0,1.0,-1.0));
		TS_ASSERT_EQUALS(A.getDirect(),V3D(1.0,0.0,0.0));
	}

	void testDisplace(){
		Line A(V3D(1.0,1.0,1.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),V3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(A.getDirect(),V3D(1.0,0.0,0.0));
		A.displace(V3D(2.0,1.0,2.0));
		TS_ASSERT_EQUALS(A.getOrigin(),V3D(3.0,2.0,3.0));
		TS_ASSERT_EQUALS(A.getDirect(),V3D(1.0,0.0,0.0));
	}

	void testIsValid(){
		Line A(V3D(1.0,1.0,1.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),V3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(A.getDirect(),V3D(1.0,0.0,0.0));
		
		TS_ASSERT_EQUALS(A.isValid(V3D(1.1,1.0,1.0)),1);
		TS_ASSERT_EQUALS(A.isValid(V3D(0.9,1.0,1.0)),1);
		// test tolerance default 1e-6
		TS_ASSERT_EQUALS(A.isValid(V3D(0.9,1.0+1e-7,1.0+1e-7)),1);
		TS_ASSERT_EQUALS(A.isValid(V3D(0.9,1.0+2e-6,1.0+2e-6)),0);
		TS_ASSERT_EQUALS(A.isValid(V3D(0.9,1.0-1e-7,1.0-1e-7)),1);
		TS_ASSERT_EQUALS(A.isValid(V3D(0.9,1.0-2e-6,1.0-2e-6)),0);

		TS_ASSERT_EQUALS(A.isValid(V3D(1.0,0.9,1.0)),0);
		TS_ASSERT_EQUALS(A.isValid(V3D(1.0,1.1,1.0)),0);
		TS_ASSERT_EQUALS(A.isValid(V3D(1.0,1.0,0.9)),0);
		TS_ASSERT_EQUALS(A.isValid(V3D(1.0,1.0,1.1)),0);
	}

	void testSetLine(){
		Line A;
		TS_ASSERT_EQUALS(A.getOrigin(),V3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getDirect(),V3D(0.0,0.0,0.0));

		A.setLine(V3D(1.0,1.0,1.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),V3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(A.getDirect(),V3D(1.0,0.0,0.0));
	}

	//A Line with equation equivalent to x axis will cut A Cylinder with 1 radius with center at 0,0,0  y axis normal 
	//at two points
	void testIntersectCylinder(){
		Line A(V3D(0.0,0.0,0.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),V3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getDirect(),V3D(1.0,0.0,0.0));

		Cylinder B;
		B.setSurface("c/y 0.0 0.0 1.0");
		TS_ASSERT_EQUALS(B.getCentre(),V3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(B.getRadius(),1);
		TS_ASSERT_EQUALS(B.getNormal(),V3D(0,1,0));

		std::list<V3D> pntOut;
		A.intersect(pntOut,B);

		// forward only solution for cylinders
		TS_ASSERT_EQUALS(pntOut.size(),1);
		TS_ASSERT_EQUALS(pntOut.front(),V3D(1.0,0.0,0.0));
	}

  	//A Line with equation equivalent to x axis will cut A Cylinder with 1 radius with center at 0,0,0  y axis normal 
	//at two points
	void testNotOriginIntersectCylinder(){
		Line A(V3D(-10.0,0.0,0.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),V3D(-10.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getDirect(),V3D(1.0,0.0,0.0));

		Cylinder B;
		B.setSurface("c/y 0.0 0.0 1.0");
		TS_ASSERT_EQUALS(B.getCentre(),V3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(B.getRadius(),1);
		TS_ASSERT_EQUALS(B.getNormal(),V3D(0,1,0));

		std::list<V3D> pntOut;
		A.intersect(pntOut,B);

		TS_ASSERT_EQUALS(pntOut.size(),2);
		auto itr = pntOut.begin();
		TS_ASSERT_EQUALS(*(itr++), V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(*itr,V3D(-1.0,0.0,0.0));
	}

	//A Line with equation equivalent to x axis will cut a plane YZ with equation x=5 will cut at one point 5,0,0
	//Some problem here the negative axis plane is not cut need to investigate
	void testIntersectPlane(){
		Line A(V3D(0.0,0.0,0.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),V3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getDirect(),V3D(1.0,0.0,0.0));

		Plane B;
		TS_ASSERT_EQUALS(B.setSurface("px 5 0 0"),0);
		std::list<V3D> pntOut;
		A.intersect(pntOut,B);

		TS_ASSERT_EQUALS(pntOut.size(),1);
		TS_ASSERT_EQUALS(pntOut.front(),V3D(5.0,0.0,0.0));
	}

	//A Line with equation equivalent to x axis will cut A sphere with 2 radius with center at 0,0,0 
	//at two points
	void testIntersectSphere(){
		Line A(V3D(0.0,0.0,0.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),V3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getDirect(),V3D(1.0,0.0,0.0));

		Sphere B;
		B.setSurface("s 0.0 0.0 0.0 2");
		std::list<V3D> pntOut;
		A.intersect(pntOut,B);
		// forward only solutions
		TS_ASSERT_EQUALS(pntOut.size(),1);
		TS_ASSERT_EQUALS(pntOut.front(),V3D(2.0,0.0,0.0));
	}

  //A Line with equation equivalent to x axis starting at -10 will cut A sphere with 2 radius with center at 0,0,0 
	//at two points
	void testNotOriginIntersectSphere(){
		Line A(V3D(-10.0,0.0,0.0),V3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),V3D(-10.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getDirect(),V3D(1.0,0.0,0.0));

		Sphere B;
		B.setSurface("s 0.0 0.0 0.0 2");
		std::list<V3D> pntOut;
		A.intersect(pntOut,B);
		TS_ASSERT_EQUALS(pntOut.size(),2);
    auto itr = pntOut.begin();
		TS_ASSERT_EQUALS(*(itr++), V3D(2.0,0.0,0.0));
		TS_ASSERT_EQUALS(*itr, V3D(-2.0,0.0,0.0));
	}
};

#endif
