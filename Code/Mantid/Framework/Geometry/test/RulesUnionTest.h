#ifndef MANTID_RULESUNIONTEST__
#define MANTID_RULESUNIONTEST__
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

class RulesUnionTest:public CxxTest::TestSuite{
public:
	void testDefaultConstructor(){
		Union A;
		TS_ASSERT_EQUALS(A.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(A.leaf(1),(Rule*)0);
	}

	void testTwoRuleConstructor(){
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
		Union A(S1,S2);
		TS_ASSERT_EQUALS(A.display(),"10 : 11");
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
	}

	void testThreeRuleConstructor(){
		Union Parent;
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
		Union A(&Parent,S1,S2);
		TS_ASSERT_EQUALS(A.display(),"10 : 11");
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		TS_ASSERT_EQUALS(A.getParent(),&Parent);
	}

	void testUnionConstructor(){
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
		Union A(S1,S2);
		TS_ASSERT_EQUALS(A.display(),"10 : 11");
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		Union B(A);
		TS_ASSERT_EQUALS(B.display(),"10 : 11");
		TS_ASSERT_EQUALS(B.leaf(0)->display(),S1->display());
		TS_ASSERT_EQUALS(B.leaf(1)->display(),S2->display());
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
		Union A(S1,S2);
		TS_ASSERT_EQUALS(A.display(),"10 : 11");
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		Union *B;
		B=A.clone();
		TS_ASSERT_EQUALS(B->display(),"10 : 11");
		TS_ASSERT_EQUALS(B->leaf(0)->display(),S1->display());
		TS_ASSERT_EQUALS(B->leaf(1)->display(),S2->display());
		delete B;
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
		Union A(S1,S2);
		TS_ASSERT_EQUALS(A.display(),"10 : 11");
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		Union B;
		B=A;
		TS_ASSERT_EQUALS(B.display(),"10 : 11");
		TS_ASSERT_EQUALS(B.leaf(0)->display(),S1->display());
		TS_ASSERT_EQUALS(B.leaf(1)->display(),S2->display());
	}

	void testSetLeaves(){
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
		Union A;
		A.setLeaves(S1,S2);
		TS_ASSERT_EQUALS(A.display(),"10 : 11");
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
	}

	void testSetLeaf(){
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
		Union A;
		A.setLeaf(S2,1);
		TS_ASSERT_EQUALS(A.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		A.setLeaf(S1,0);
		TS_ASSERT_EQUALS(A.display(),"10 : 11");
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
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
		Union A;
		A.setLeaves(S1,S2);
		TS_ASSERT_EQUALS(A.display(),"10 : 11");
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		TS_ASSERT_EQUALS(A.findLeaf(S1),0);
		TS_ASSERT_EQUALS(A.findLeaf(S2),1);
		TS_ASSERT_EQUALS(A.findLeaf(&S3),-1);
	}

	void testFindKey(){
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
		Union A;
		A.setLeaves(S1,S2);
		TS_ASSERT_EQUALS(A.display(),"10 : 11");
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		TS_ASSERT_EQUALS(A.findKey(10),S1);
		TS_ASSERT_EQUALS(A.findKey(11),S2);
		TS_ASSERT_EQUALS(A.findKey(15),(Rule*)0);
	}

	void testIsComplementary(){ //Problem: it only detects whether first leaf or second leaf but not both
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
		Union A(S1,S2);
		TS_ASSERT_EQUALS(A.display(),"10 : 11");
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
		Union A(S1,S2);
		TS_ASSERT_EQUALS(A.display(),"10 : -11");
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		TS_ASSERT_EQUALS(A.isValid(V3D(0.0,0.0,0.0)),true); //on surface
		TS_ASSERT_EQUALS(A.isValid(V3D(5.0,0.0,0.0)),true); //inside surface
		TS_ASSERT_EQUALS(A.isValid(V3D(-0.1,0.0,0.0)),false);//just outside surface
		TS_ASSERT_EQUALS(A.isValid(V3D(10.1,1.0,1.0)),true);//on other side of the plane
	}

	void testIsValidMap(){
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
		Union A(S1,S2);
		TS_ASSERT_EQUALS(A.display(),"10 : -11");
		TS_ASSERT_EQUALS(A.leaf(0),S1);
		TS_ASSERT_EQUALS(A.leaf(1),S2);
		std::map<int,int> input;
		input[5]=1;
		input[10]=1;
		input[11]=1;
		input[15]=0;
		input[20]=-1;
		TS_ASSERT_EQUALS(A.isValid(input),true);
		input[10]=0;
		input[11]=0;
		TS_ASSERT_EQUALS(A.isValid(input),true);
		input[11]=1;
		TS_ASSERT_EQUALS(A.isValid(input),false);
	}
};

#endif
