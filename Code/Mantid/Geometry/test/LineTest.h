#ifndef MANTID_LINETEST__
#define MANTID_LINETEST__
#include <cxxtest/TestSuite.h>
#include <cmath>
#include <vector>
#include "../inc/Vec3D.h"
#include "MantidGeometry/Quadratic.h"
#include "MantidGeometry/Line.h"
#include "MantidGeometry/Cylinder.h"
#include "MantidGeometry/Sphere.h"
#include "MantidGeometry/Plane.h"

using namespace Mantid;
using namespace Geometry;


class testLine: public CxxTest::TestSuite
{
public:
	void testConstructor(){
		Line A;
		TS_ASSERT_EQUALS(A.getOrigin(),Vec3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getDirect(),Vec3D(0.0,0.0,0.0));
	}

	void testParamConstructor(){
		Line A(Vec3D(1.0,1.0,1.0),Vec3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),Vec3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(A.getDirect(),Vec3D(1.0,0.0,0.0));
	}

	void testLineConstructor(){
		Line A(Vec3D(1.0,1.0,1.0),Vec3D(1.0,0.0,0.0));
		Line B(A);
		TS_ASSERT_EQUALS(B.getOrigin(),Vec3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(B.getDirect(),Vec3D(1.0,0.0,0.0));
	}
    //There is no implementation for clone
	void xtestClone(){
		Line A(Vec3D(1.0,1.0,1.0),Vec3D(1.0,0.0,0.0));
		Line *B=A.clone();;
		TS_ASSERT_EQUALS(B->getOrigin(),Vec3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(B->getDirect(),Vec3D(1.0,0.0,0.0));
	}

	void testAssignment(){
		Line A(Vec3D(1.0,1.0,1.0),Vec3D(1.0,0.0,0.0));
		Line B;
		TS_ASSERT_DIFFERS(B.getOrigin(),Vec3D(1.0,1.0,1.0));
		TS_ASSERT_DIFFERS(B.getDirect(),Vec3D(1.0,0.0,0.0));
		B=A;
		TS_ASSERT_EQUALS(B.getOrigin(),Vec3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(B.getDirect(),Vec3D(1.0,0.0,0.0));
	}

	void testGetPoint(){
		Line A(Vec3D(1.0,1.0,1.0),Vec3D(1.0,0.0,0.0));
		//0 lambda should return origin
		TS_ASSERT_EQUALS(A.getPoint(0.0),Vec3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(A.getPoint(-1.0),Vec3D(0.0,1.0,1.0));
		TS_ASSERT_EQUALS(A.getPoint(1.0),Vec3D(2.0,1.0,1.0));
	}

	void testDistance(){
		Line A(Vec3D(1.0,1.0,1.0),Vec3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.distance(Vec3D(0.0,0.0,0.0)),sqrt(2.0));
		TS_ASSERT_EQUALS(A.distance(Vec3D(1.0,0.0,0.0)),sqrt(2.0));
		TS_ASSERT_EQUALS(A.distance(Vec3D(1.0,1.0,0.0)),1.0);
	}

	void makeMatrix(Matrix<double>& A) const
	{
	 const double pi=3.141592653589793238462643383279502884197169399375 ;
		A.setMem(3,3);
		A[0][0]=1.0;
		A[1][0]=0.0; 
		A[0][1]=0.0;
		A[0][2]=0.0;
		A[2][0]=0.0;
		A[1][1]=cos(90.0*pi/180.0);
		A[2][1]=-sin(90.0*pi/180.0);
		A[1][2]=sin(90.0*pi/180.0);
		A[2][2]=cos(90.0*pi/180.0);
		return;
	}

	void testRotate(){
		Line A(Vec3D(1.0,1.0,1.0),Vec3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),Vec3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(A.getDirect(),Vec3D(1.0,0.0,0.0));

		Matrix<double> rotMat(3,3);
		makeMatrix(rotMat);
		A.rotate(rotMat);
		TS_ASSERT_EQUALS(A.getOrigin(),Vec3D(1.0,1.0,-1.0));
		TS_ASSERT_EQUALS(A.getDirect(),Vec3D(1.0,0.0,0.0));
	}

	void testDisplace(){
		Line A(Vec3D(1.0,1.0,1.0),Vec3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),Vec3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(A.getDirect(),Vec3D(1.0,0.0,0.0));
		A.displace(Vec3D(2.0,1.0,2.0));
		TS_ASSERT_EQUALS(A.getOrigin(),Vec3D(3.0,2.0,3.0));
		TS_ASSERT_EQUALS(A.getDirect(),Vec3D(1.0,0.0,0.0));
	}

	void testIsValid(){
		Line A(Vec3D(1.0,1.0,1.0),Vec3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),Vec3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(A.getDirect(),Vec3D(1.0,0.0,0.0));
		
		TS_ASSERT_EQUALS(A.isValid(Vec3D(1.1,1.0,1.0)),1);
		TS_ASSERT_EQUALS(A.isValid(Vec3D(0.9,1.0,1.0)),1);

		TS_ASSERT_EQUALS(A.isValid(Vec3D(1.0,0.9,1.0)),0);
		TS_ASSERT_EQUALS(A.isValid(Vec3D(1.0,1.1,1.0)),0);
		TS_ASSERT_EQUALS(A.isValid(Vec3D(1.0,1.0,0.9)),0);
		TS_ASSERT_EQUALS(A.isValid(Vec3D(1.0,1.0,1.1)),0);
	}

	void testSetLine(){
		Line A;
		TS_ASSERT_EQUALS(A.getOrigin(),Vec3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getDirect(),Vec3D(0.0,0.0,0.0));

		A.setLine(Vec3D(1.0,1.0,1.0),Vec3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),Vec3D(1.0,1.0,1.0));
		TS_ASSERT_EQUALS(A.getDirect(),Vec3D(1.0,0.0,0.0));
	}

	//A Line with equation equivalent to x axis will cut A Cylinder with 1 radius with center at 0,0,0  y axis normal 
	//at two points
	void testIntersectCylinder(){
		Line A(Vec3D(0.0,0.0,0.0),Vec3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),Vec3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getDirect(),Vec3D(1.0,0.0,0.0));

		Cylinder B;
		B.setSurface("c/y 0.0 0.0 1.0");
		TS_ASSERT_EQUALS(B.getCentre(),Vec3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(B.getRadius(),1);
		TS_ASSERT_EQUALS(B.getNormal(),Vec3D(0,1,0));

		std::vector<Vec3D> pntOut;
		A.intersect(pntOut,B);

		TS_ASSERT_EQUALS(pntOut.size(),2);		
		TS_ASSERT_EQUALS(pntOut[0],Vec3D(-1.0,0.0,0.0));
		TS_ASSERT_EQUALS(pntOut[1],Vec3D(1.0,0.0,0.0));
	}

	//A Line with equation equivalent to x axis will cut a plane YZ with equation x=5 will cut at one point 5,0,0
	//Some problem here the negative axis plane is not cut need to investigate
	void testIntersectPlane(){
		Line A(Vec3D(0.0,0.0,0.0),Vec3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),Vec3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getDirect(),Vec3D(1.0,0.0,0.0));

		Plane B;
		TS_ASSERT_EQUALS(B.setSurface("px 5 0 0"),0);
		std::vector<Vec3D> pntOut;
		A.intersect(pntOut,B);

		TS_ASSERT_EQUALS(pntOut.size(),1);		
		TS_ASSERT_EQUALS(pntOut[0],Vec3D(5.0,0.0,0.0));
	}

	//A Line with equation equivalent to x axis will cut A sphere with 2 radius with center at 0,0,0 
	//at two points
	void testIntersectSphere(){
		Line A(Vec3D(0.0,0.0,0.0),Vec3D(1.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getOrigin(),Vec3D(0.0,0.0,0.0));
		TS_ASSERT_EQUALS(A.getDirect(),Vec3D(1.0,0.0,0.0));

		Sphere B;
		B.setSurface("s 0.0 0.0 0.0 2");
		std::vector<Vec3D> pntOut;
		A.intersect(pntOut,B);
		TS_ASSERT_EQUALS(pntOut.size(),2);		
		TS_ASSERT_EQUALS(pntOut[0],Vec3D(-2.0,0.0,0.0));
		TS_ASSERT_EQUALS(pntOut[1],Vec3D(2.0,0.0,0.0));
	}
};

#endif