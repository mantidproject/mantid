#ifndef MANTID_RULESINTERSECTIONTEST__
#define MANTID_RULESINTERSECTIONTEST__
#include <cxxtest/TestSuite.h>
#include <cmath>
#include <vector>
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"
#include <cfloat>
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Cone.h"

using namespace Mantid;
using namespace Geometry;

class RulesIntersectionTest: public CxxTest::TestSuite{
public:
	void testDefaultConstructor(){
		Intersection A;
		TS_ASSERT_EQUALS(A.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(A.leaf(1),(Rule*)0);

		
		//Intersection A;
		//SurfPoint S1,S2;
		//Plane P1;
		//Sphere Sp1;
		//P1.setSurface("px 5"); //yz plane with x=5
		//Sp1.setSurface("s 5.0 0.0 0.0 5");//a sphere with center (5,0,0) and radius 5. this will touch origin
		//S1.setKey(&P1);
		//S1.setKeyN(10);
		//S2.setKey(&Sp1);
		//S2.setKeyN(11);
		//A.setLeaves(S1,S2);
	}

	void testTwoRuleConstructor(){ //Creating a half sphere
		SurfPoint *S1,*S2;
    Plane *P1 = new Plane;
    Sphere *Sp1 = new Sphere;
		P1->setSurface("px 5"); //yz plane with x=5
		Sp1->setSurface("s 5.0 0.0 0.0 5");//a sphere with center (5,0,0) and radius 5. this will touch origin
		S1=new SurfPoint();
		S2=new SurfPoint();
		S1->setKey(P1);
		S1->setKeyN(10);
		S2->setKey(Sp1);
		S2->setKeyN(-11);
		Intersection A(S1,S2);
		TS_ASSERT_EQUALS(A.leaf(0),S2);
		TS_ASSERT_EQUALS(A.leaf(1),S1);
		TS_ASSERT_EQUALS(A.display(),"-11 10");
	}

	void testThreeRuleConstructor(){
		SurfPoint *S1,*S2;
    Plane *P1 = new Plane;
    Sphere *Sp1 = new Sphere;
		P1->setSurface("px 5"); //yz plane with x=5
		Sp1->setSurface("s 5.0 0.0 0.0 5");//a sphere with center (5,0,0) and radius 5. this will touch origin
		S1=new SurfPoint();
		S2=new SurfPoint();
		S1->setKey(P1);
		S1->setKeyN(10);
		S2->setKey(Sp1);
		S2->setKeyN(11);
		Intersection Parent;
		Intersection A(&Parent,S1,S2);
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		TS_ASSERT_EQUALS(A.display(),"10 11");
		TS_ASSERT_EQUALS(A.getParent(),&Parent);
	}

	void testClone(){
		SurfPoint *S1,*S2;
    Plane *P1 = new Plane;
    Sphere *Sp1 = new Sphere;
		P1->setSurface("px 5"); //yz plane with x=5
		Sp1->setSurface("s 5.0 0.0 0.0 5");//a sphere with center (5,0,0) and radius 5. this will touch origin
		S1=new SurfPoint();
		S2=new SurfPoint();
		S1->setKey(P1);
		S1->setKeyN(10);
		S2->setKey(Sp1);
		S2->setKeyN(11);
		Intersection A;
		A.setLeaves(S1,S2);
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		TS_ASSERT_EQUALS(A.display(),"10 11");
		Intersection *B;
		B=A.clone();
		TS_ASSERT_EQUALS(B->leaf(0)->display(),S1->display());
		TS_ASSERT_EQUALS(B->leaf(1)->display(),S2->display());
		TS_ASSERT_EQUALS(B->display(),"10 11");
		delete B;
	}

	void testIntersectionConstructor(){
		SurfPoint *S1,*S2;
    Plane *P1 = new Plane;
    Sphere *Sp1 = new Sphere;
		P1->setSurface("px 5"); //yz plane with x=5
		Sp1->setSurface("s 5.0 0.0 0.0 5");//a sphere with center (5,0,0) and radius 5. this will touch origin
		S1=new SurfPoint();
		S2=new SurfPoint();
		S1->setKey(P1);
		S1->setKeyN(10);
		S2->setKey(Sp1);
		S2->setKeyN(11);
		Intersection A;
		A.setLeaves(S1,S2);
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		TS_ASSERT_EQUALS(A.display(),"10 11");
		Intersection B(A);
		TS_ASSERT_EQUALS(B.leaf(0)->display(),S1->display());
		TS_ASSERT_EQUALS(B.leaf(1)->display(),S2->display());
		TS_ASSERT_EQUALS(B.display(),"10 11");
	}

	void testAssignment(){
		SurfPoint *S1,*S2;
    Plane *P1 = new Plane;
    Sphere *Sp1 = new Sphere;
		P1->setSurface("px 5"); //yz plane with x=5
		Sp1->setSurface("s 5.0 0.0 0.0 5");//a sphere with center (5,0,0) and radius 5. this will touch origin
		S1=new SurfPoint();
		S2=new SurfPoint();
		S1->setKey(P1);
		S1->setKeyN(10);
		S2->setKey(Sp1);
		S2->setKeyN(11);
		Intersection A;
		A.setLeaves(S1,S2);
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		TS_ASSERT_EQUALS(A.display(),"10 11");
		Intersection B;
		B=A;
		TS_ASSERT_EQUALS(B.leaf(0)->display(),S1->display());
		TS_ASSERT_EQUALS(B.leaf(1)->display(),S2->display());
		TS_ASSERT_EQUALS(B.display(),"10 11");
	}

	void testFindLeaf(){
		SurfPoint *S1,*S2,S3;
    Plane *P1 = new Plane;
    Sphere *Sp1 = new Sphere;
		P1->setSurface("px 5"); //yz plane with x=5
		Sp1->setSurface("s 5.0 0.0 0.0 5");//a sphere with center (5,0,0) and radius 5. this will touch origin
		S1=new SurfPoint();
		S2=new SurfPoint();
		S1->setKey(P1);
		S1->setKeyN(10);
		S2->setKey(Sp1);
		S2->setKeyN(11);
		Intersection A;
		A.setLeaves(S1,S2);
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		TS_ASSERT_EQUALS(A.display(),"10 11");
		TS_ASSERT_EQUALS(A.findLeaf(S1),0);
		TS_ASSERT_EQUALS(A.findLeaf(S2),1);
		TS_ASSERT_EQUALS(A.findLeaf(&S3),-1);
	}

	void testFindKey(){
		SurfPoint *S1,*S2,S3;
    Plane *P1 = new Plane;
    Sphere *Sp1 = new Sphere;
		P1->setSurface("px 5"); //yz plane with x=5
		Sp1->setSurface("s 5.0 0.0 0.0 5");//a sphere with center (5,0,0) and radius 5. this will touch origin
		S1=new SurfPoint();
		S2=new SurfPoint();
		S1->setKey(P1);
		S1->setKeyN(10);
		S2->setKey(Sp1);
		S2->setKeyN(11);
		Intersection A;
		A.setLeaves(S1,S2);
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		TS_ASSERT_EQUALS(A.display(),"10 11");
		TS_ASSERT_EQUALS(A.findKey(10),S1);
		TS_ASSERT_EQUALS(A.findKey(11),S2);
		TS_ASSERT_EQUALS(A.findKey(12),(Rule*)0);
	}

	void testIsComplementary(){
		SurfPoint *S1,*S2;
    Plane *P1 = new Plane;
    Sphere *Sp1 = new Sphere;
		P1->setSurface("px 5"); //yz plane with x=5
		Sp1->setSurface("s 5.0 0.0 0.0 5");//a sphere with center (5,0,0) and radius 5. this will touch origin
		S1=new SurfPoint();
		S2=new SurfPoint();
		S1->setKey(P1);
		S1->setKeyN(10);
		S2->setKey(Sp1);
		S2->setKeyN(11);
		Intersection A;
		A.setLeaves(S1,S2);
		TS_ASSERT_EQUALS(A.display(),"10 11");
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		TS_ASSERT_EQUALS(A.isComplementary(),0);
		CompObj *B=new CompObj();
		CompObj *C=new CompObj();
		A.setLeaf(B,1);
		TS_ASSERT_EQUALS(A.isComplementary(),-1);
		A.setLeaf(C,0);
		TS_ASSERT_EQUALS(A.isComplementary(),1);
	}

	void testIsValid(){
		SurfPoint *S1,*S2,S3;
    Plane *P1 = new Plane;
    Sphere *Sp1 = new Sphere;
		P1->setSurface("px 5"); //yz plane with x=5
		Sp1->setSurface("s 5.0 0.0 0.0 5");//a sphere with center (5,0,0) and radius 5. this will touch origin
		S1=new SurfPoint();
		S2=new SurfPoint();
		S1->setKey(P1);
		S1->setKeyN(10);
		S2->setKey(Sp1);
		S2->setKeyN(-11);
		Intersection A;
		A.setLeaves(S1,S2);
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		TS_ASSERT_EQUALS(A.display(),"10 -11");
		TS_ASSERT_EQUALS(A.isValid(V3D(5.0,0.0,0.0)),true); //on surface
		TS_ASSERT_EQUALS(A.isValid(V3D(5.1,0.0,0.0)),true); //inside surface
		TS_ASSERT_EQUALS(A.isValid(V3D(4.9,0.0,0.0)),false);//just outside surface
		TS_ASSERT_EQUALS(A.isValid(V3D(10,0.0,0.0)),true);
		TS_ASSERT_EQUALS(A.isValid(V3D(10.1,0.0,0.0)),false);//on other side of the plane
	}

	void testBoundingBox(){
		SurfPoint *S1,*S2,S3;
    Plane *P1 = new Plane;
    Sphere *Sp1 = new Sphere;
		P1->setSurface("px 5"); //yz plane with x=5
		Sp1->setSurface("s 5.0 0.0 0.0 5");//a sphere with center (5,0,0) and radius 5. this will touch origin
		S1=new SurfPoint();
		S2=new SurfPoint();
		S1->setKey(P1);
		S1->setKeyN(10);
		S2->setKey(Sp1);
		S2->setKeyN(-11);
		Intersection A;
		A.setLeaves(S1,S2);
		double xmax,ymax,zmax,xmin,ymin,zmin;
		xmax=ymax=zmax=DBL_MAX;
		xmin=ymin=zmin=-DBL_MAX;
		A.getBoundingBox(xmax,ymax,zmax,xmin,ymin,zmin);
		TS_ASSERT_DELTA(xmax,10.0,0.001);
		TS_ASSERT_DELTA(xmin,0.0,0.001);
		TS_ASSERT_DELTA(ymax,5.0,0.001);
		TS_ASSERT_DELTA(ymin,-5.0,0.001);
		TS_ASSERT_DELTA(zmax,5.0,0.0001);
		TS_ASSERT_DELTA(zmin,-5.0,0.0001);
	}
};
//-----------------------------------------------End of Intersection---------------------------------------

#endif
