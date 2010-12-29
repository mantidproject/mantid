#ifndef MANTID_RULESTEST__
#define MANTID_RULESTEST__
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

class testRule: public CxxTest::TestSuite{
public:
	void testMakeCNFCopy(){
	}

	void testMakeFullDNF(){
	}

	void testMakeCNF(){
		Rule *tree=createAMixedTree();
		TS_ASSERT_EQUALS(tree->display(),"(10 11) : (12 10)");
		//TS_ASSERT_EQUALS(Rule::makeCNFcopy(tree),1);
		//TS_ASSERT_EQUALS(tree->display(),"(10 11) : (12 10)");
		delete tree;
	}

	void testRemoveComplementary(){
		Rule *tree=createAUnionTree();
		TS_ASSERT_EQUALS(tree->display(),"10 : 10 : 12 : 11");
		TS_ASSERT_EQUALS(tree->removeComplementary(tree),1);
//		TS_ASSERT_EQUALS(tree->display(),"10 : 12 : 11"); //Problem: The comments don't match to the functionaility it is supposed to do
		delete tree;
	}

	void testRemoveItem(){ //Problem: in removing the item of the tree that has more than one instances,possibly double deletion
		Rule *tree=createAUnionTree();
		TS_ASSERT_EQUALS(tree->removeItem(tree,11),1);
//		TS_ASSERT_EQUALS(tree->removeItem(tree,10),2);
		TS_ASSERT_EQUALS(tree->removeItem(tree,11),0);
		TS_ASSERT_EQUALS(tree->removeItem(tree,12),1);
		delete tree;
	}

	void testCommonType(){
		Rule *uTree=createAUnionTree();
		TS_ASSERT_EQUALS(uTree->commonType(),-1);
		Rule *iTree=createAIntersectionTree();
		TS_ASSERT_EQUALS(iTree->commonType(),1);
		Rule *mTree=createAMixedTree();
		TS_ASSERT_EQUALS(mTree->commonType(),0);
		delete uTree;
		delete iTree;
		delete mTree;
	}

	void testSubstituteSurf(){
		Rule *uTree=createAUnionTree();
		TS_ASSERT_EQUALS(uTree->substituteSurf(11,13,new Cone()),1);
		TS_ASSERT_EQUALS(uTree->display(),"10 : 10 : 12 : 13");
		TS_ASSERT_EQUALS(uTree->substituteSurf(10,14,new Sphere()),2); //its suppose to return 2
		TS_ASSERT_EQUALS(uTree->display(),"14 : 14 : 12 : 13");
		delete uTree;
	}

	void testEliminate(){
	}
private:
	Rule* createAUnionTree(){ //A:B:C:A
		//A Node
		SurfPoint *A,*B,*C;
		A=new SurfPoint();
		A->setKey(new Plane());
		A->setKeyN(10);
		B=new SurfPoint();
		B->setKey(new Sphere());
		B->setKeyN(11);
		C=new SurfPoint();
		C->setKey(new Cylinder());
		C->setKeyN(12);
		
		Union *Left;
		Left=new Union(A,A->clone());

		Union *Right;
		Right=new Union(C,B);

		Union *Root;
		Root=new Union(Left,Right);
		return Root;
	}

	Rule* createAIntersectionTree(){ //A B C A
		//A Node
		SurfPoint *A,*B,*C;
		A=new SurfPoint();
		A->setKey(new Plane());
		A->setKeyN(10);
		B=new SurfPoint();
		B->setKey(new Sphere());
		B->setKeyN(11);
		C=new SurfPoint();
		C->setKey(new Cylinder());
		C->setKeyN(12);
		Intersection *Root;
		Root=new Intersection();
		
		Intersection *Left;
		Left=new Intersection();
		Left->setLeaves(A,B);

		Intersection *Right;
		Right=new Intersection();
		Right->setLeaves(C,A->clone());

		Root->setLeaves(Left,Right);
		return Root;
	}

	Rule* createAMixedTree(){// A B : C A
		SurfPoint *A,*B,*C;
		A=new SurfPoint();
		A->setKey(new Plane());
		A->setKeyN(10);
		B=new SurfPoint();
		B->setKey(new Sphere());
		B->setKeyN(11);
		C=new SurfPoint();
		C->setKey(new Cylinder());
		C->setKeyN(12);
		Union *Root;
		Root=new Union();
		
		Intersection *Left;
		Left=new Intersection();
		Left->setLeaves(A,B);

		Intersection *Right;
		Right=new Intersection();
		Right->setLeaves(C,A->clone());

		Root->setLeaves(Left,Right);
		return Root;
	}
};
class testIntersection: public CxxTest::TestSuite{
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
class testUnion:public CxxTest::TestSuite{
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
//---------------------------------------------------End of Union-------------------------------------------------
class testSurfPoint: public CxxTest::TestSuite{
public:
	void testDefaultConstructor(){
		SurfPoint A;
		TS_ASSERT_EQUALS(A.display(),"0");
	}

	void testSetKey(){
		SurfPoint A;
		TS_ASSERT_EQUALS(A.display(),"0");
		Plane *P1 = new Plane;
		A.setKey(P1);
		TS_ASSERT_EQUALS(A.getKey(),P1);
	}

	void testSetKeyN(){
		SurfPoint A;
		TS_ASSERT_EQUALS(A.getKeyN(),0);
		A.setKeyN(10);
		TS_ASSERT_EQUALS(A.getKeyN(),10);
		TS_ASSERT_EQUALS(A.display(),"10");
	}

	void testFullCreatedObject(){
		SurfPoint A;
    Plane *P1 = new Plane;
		A.setKey(P1);
		A.setKeyN(10);
		TS_ASSERT_EQUALS(A.getKey(),P1);
		TS_ASSERT_EQUALS(A.getKeyN(),10);
		TS_ASSERT_EQUALS(A.display(),"10");
	}

	void testSimplyfy(){ //A simple leafnode of SurfPoint cannot be simplefield so it should always return 0
		SurfPoint A;
		TS_ASSERT_EQUALS(A.simplify(),0);
    Plane *P1 = new Plane;
		A.setKey(P1);
		A.setKeyN(10);
		TS_ASSERT_EQUALS(A.simplify(),0);
	}

	void testLeaf(){ //SurfPoint will always be end of tree always returns 0
		SurfPoint A;
		TS_ASSERT_EQUALS(A.simplify(),0);
		TS_ASSERT_EQUALS(A.leaf(0),(Rule*)0);
    Plane *P1 = new Plane;
		A.setKey(P1);
		A.setKeyN(10);
		TS_ASSERT_EQUALS(A.simplify(),0);
		TS_ASSERT_EQUALS(A.leaf(10),(Rule*)0);
	}

	void testSetLeaves(){ //This should not enabled as surfpoint cannot have leaves
						  //TODO:disable setleaves function
		SurfPoint A;
    Plane *P1 = new Plane;
		A.setKey(P1);
		A.setKeyN(10);
		TS_ASSERT_EQUALS(A.getKey(),P1);
		TS_ASSERT_EQUALS(A.getKeyN(),10);
		TS_ASSERT_EQUALS(A.display(),"10");
		SurfPoint B;
		Sphere *S1 = new Sphere;
		B.setKey(S1);
		B.setKeyN(11);
		TS_ASSERT_EQUALS(B.getKey(),S1);
		TS_ASSERT_EQUALS(B.getKeyN(),11);
		TS_ASSERT_EQUALS(B.display(),"11");
		A.setLeaves(&B,(Rule*)0);
		TS_ASSERT(dynamic_cast<Sphere*>(A.getKey()));
		TS_ASSERT_EQUALS(A.getKeyN(),11);
		TS_ASSERT_EQUALS(A.display(),"11");
	}

	void testSetLeaf(){ //TODO: disable setleaf function as surfpoint cannot have leaves
		SurfPoint A;
    Plane *P1 = new Plane;
		A.setKey(P1);
		A.setKeyN(10);
		TS_ASSERT_EQUALS(A.getKey(),P1);
		TS_ASSERT_EQUALS(A.getKeyN(),10);
		TS_ASSERT_EQUALS(A.display(),"10");
		SurfPoint B;
		Sphere *S1 = new Sphere;
		B.setKey(S1);
		B.setKeyN(11);
		TS_ASSERT_EQUALS(B.getKey(),S1);
		TS_ASSERT_EQUALS(B.getKeyN(),11);
		TS_ASSERT_EQUALS(B.display(),"11");
		A.setLeaf(&B,0);
    TS_ASSERT(dynamic_cast<Sphere*>(A.getKey()));
		TS_ASSERT_EQUALS(A.getKeyN(),11);
		TS_ASSERT_EQUALS(A.display(),"11");
	}

	void testfindLeaf(){//TODO: disable to find leaf as this is final
		SurfPoint A;
    Plane *P1 = new Plane;
		A.setKey(P1);
		A.setKeyN(10);
		TS_ASSERT_EQUALS(A.getKey(),P1);
		TS_ASSERT_EQUALS(A.getKeyN(),10);
		TS_ASSERT_EQUALS(A.display(),"10");
		SurfPoint B;
		Sphere *S1 = new Sphere;
		B.setKey(S1);
		B.setKeyN(11);
		TS_ASSERT_EQUALS(B.getKey(),S1);
		TS_ASSERT_EQUALS(B.getKeyN(),11);
		TS_ASSERT_EQUALS(B.display(),"11");
		TS_ASSERT_EQUALS(A.findLeaf(&B),-1);
		B.setKey(P1->clone());
		B.setKeyN(10);
		TS_ASSERT_EQUALS(A.findLeaf(&B),-1);
		TS_ASSERT_EQUALS(A.findLeaf(&A),0);
	}

	void testFindKey(){
		SurfPoint A;
		Plane *P1 = new Plane;
		A.setKey(P1);
		A.setKeyN(10);
		TS_ASSERT_EQUALS(A.getKey(),P1);
		TS_ASSERT_EQUALS(A.getKeyN(),10);
		TS_ASSERT_EQUALS(A.display(),"10");
		TS_ASSERT_EQUALS(A.findKey(10),&A);
		TS_ASSERT_EQUALS(A.findKey(11),(Rule*)0);
	}

	void testGetSign(){
		SurfPoint A;
    Plane *P1 = new Plane;
		A.setKey(P1);
		A.setKeyN(10);
		TS_ASSERT_EQUALS(A.getKey(),P1);
		TS_ASSERT_EQUALS(A.getKeyN(),10);
		TS_ASSERT_EQUALS(A.display(),"10");
		TS_ASSERT_EQUALS(A.getSign(),1);
		A.setKeyN(-10);
		TS_ASSERT_EQUALS(A.getKeyN(),10);
		TS_ASSERT_EQUALS(A.getSign(),-1);
	}

	void testSelfConstructor(){
		SurfPoint A;
    Plane *P1 = new Plane;
		A.setKey(P1);
		A.setKeyN(10);
		TS_ASSERT_EQUALS(A.getKey(),P1);
		TS_ASSERT_EQUALS(A.getKeyN(),10);
		TS_ASSERT_EQUALS(A.display(),"10");
		TS_ASSERT_EQUALS(A.getSign(),1);
		SurfPoint B(A);
		TS_ASSERT(dynamic_cast<Plane*>(B.getKey()));
		TS_ASSERT_EQUALS(B.getKeyN(),10);
		TS_ASSERT_EQUALS(B.display(),"10");
		TS_ASSERT_EQUALS(B.getSign(),1);
	}

	void testClone(){
		SurfPoint A;
    Plane *P1 = new Plane;
		A.setKey(P1);
		A.setKeyN(10);
		TS_ASSERT_EQUALS(A.getKey(),P1);
		TS_ASSERT_EQUALS(A.getKeyN(),10);
		TS_ASSERT_EQUALS(A.display(),"10");
		TS_ASSERT_EQUALS(A.getSign(),1);
		SurfPoint *B;
		B=A.clone();
		TS_ASSERT(dynamic_cast<Plane*>(B->getKey()));
		TS_ASSERT_EQUALS(B->getKeyN(),10);
		TS_ASSERT_EQUALS(B->display(),"10");
		TS_ASSERT_EQUALS(B->getSign(),1);
		delete B;
	}

	void testAssignment(){
		SurfPoint A;
    Plane *P1 = new Plane;
		A.setKey(P1);
		A.setKeyN(10);
		TS_ASSERT_EQUALS(A.getKey(),P1);
		TS_ASSERT_EQUALS(A.getKeyN(),10);
		TS_ASSERT_EQUALS(A.display(),"10");
		TS_ASSERT_EQUALS(A.getSign(),1);
		SurfPoint B;
		B=A;
    TS_ASSERT(dynamic_cast<Plane*>(B.getKey()));
		TS_ASSERT_EQUALS(B.getKeyN(),10);
		TS_ASSERT_EQUALS(B.display(),"10");
		TS_ASSERT_EQUALS(B.getSign(),1);
	}

	void testIsValid(){
		SurfPoint A;
    Plane *P1 = new Plane;
		P1->setSurface("px 5");
		A.setKey(P1);
		A.setKeyN(10);
		TS_ASSERT_EQUALS(A.getKey(),P1);
		TS_ASSERT_EQUALS(A.getKeyN(),10);
		TS_ASSERT_EQUALS(A.display(),"10");
		TS_ASSERT_EQUALS(A.getSign(),1);
		TS_ASSERT_EQUALS(A.isValid(V3D(4.9,0.0,0.0)),false);
		TS_ASSERT_EQUALS(A.isValid(V3D(5,0.0,0.0)),true);
		TS_ASSERT_EQUALS(A.isValid(V3D(5.1,0.0,0.0)),true);
	}

	void testIsValidMap(){
		SurfPoint A;
    Plane *P1 = new Plane;
		P1->setSurface("px 5");
		A.setKey(P1);
		A.setKeyN(10);
		TS_ASSERT_EQUALS(A.getKey(),P1);
		TS_ASSERT_EQUALS(A.getKeyN(),10);
		TS_ASSERT_EQUALS(A.display(),"10");
		TS_ASSERT_EQUALS(A.getSign(),1);
		std::map<int,int> input;
		input[5]=1;
		input[10]=1;
		input[15]=0;
		input[20]=-1;
		TS_ASSERT_EQUALS(A.isValid(input),true);
		A.setKeyN(15);
		TS_ASSERT_EQUALS(A.isValid(input),false);
		A.setKeyN(20);
		TS_ASSERT_EQUALS(A.isValid(input),true);
	}
};
//---------------------------------End of SurfPoint-------------------------------------

class testCompObj: public CxxTest::TestSuite{
public:
	void testConstructor(){
		CompObj A;
		TS_ASSERT_EQUALS(A.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(A.leaf(1),(Rule*)0);
		TS_ASSERT_EQUALS(A.display(),"#0");
		TS_ASSERT_EQUALS(A.getObjN(),0);
		TS_ASSERT_EQUALS(A.getObj(),(Object*)0);
		TS_ASSERT_EQUALS(A.isComplementary(),1);
	}

	void testSetObject(){
		Object cpCylinder=createCappedCylinder();
		CompObj A;
		A.setObj(&cpCylinder);
		A.setObjN(10);
		TS_ASSERT_EQUALS(A.display(),"#10");
		TS_ASSERT_EQUALS(A.getObjN(),10);
		TS_ASSERT_EQUALS(A.getObj(),&cpCylinder);
	}

	void testCompObjConstructor(){
		Object cpCylinder=createCappedCylinder();
		CompObj A;
		A.setObj(&cpCylinder);
		A.setObjN(10);
		CompObj B(A);
		TS_ASSERT_EQUALS(B.display(),"#10");
		TS_ASSERT_EQUALS(B.getObjN(),10);
		TS_ASSERT_EQUALS(B.getObj(),&cpCylinder);
	}

	void testClone(){
		Object cpCylinder=createCappedCylinder();
		CompObj A;
		A.setObj(&cpCylinder);
		A.setObjN(10);
		CompObj *B;
		B=A.clone();
		TS_ASSERT_EQUALS(B->display(),"#10");
		TS_ASSERT_EQUALS(B->getObjN(),10);
		TS_ASSERT_EQUALS(B->getObj(),&cpCylinder);
		delete B;
	}

	void testAssignment(){
		Object cpCylinder=createCappedCylinder();
		CompObj A;
		A.setObj(&cpCylinder);
		A.setObjN(10);
		CompObj B;
		B=A;
		TS_ASSERT_EQUALS(B.display(),"#10");
		TS_ASSERT_EQUALS(B.getObjN(),10);
		TS_ASSERT_EQUALS(B.getObj(),&cpCylinder);
	}

	void testSetLeaves(){
		Object cpCylinder=createCappedCylinder();
		CompObj A;
		A.setObj(&cpCylinder);
		A.setObjN(10);
		CompObj B;
		B.setLeaves(&A,(Rule*)0);
		TS_ASSERT_EQUALS(B.display(),"#10");
		TS_ASSERT_EQUALS(B.getObjN(),10);
		TS_ASSERT_EQUALS(B.getObj(),&cpCylinder);
	}

	void testSetLeaf(){
		Object cpCylinder=createCappedCylinder();
		CompObj A;
		A.setObj(&cpCylinder);
		A.setObjN(10);
		CompObj B;
		B.setLeaf(&A,0);
		TS_ASSERT_EQUALS(B.display(),"#10");
		TS_ASSERT_EQUALS(B.getObjN(),10);
		TS_ASSERT_EQUALS(B.getObj(),&cpCylinder);
	}

	void testFindLeaf(){
		Object cpCylinder=createCappedCylinder();
		CompObj A;
		A.setObj(&cpCylinder);
		A.setObjN(10);
		CompObj B;
		TS_ASSERT_EQUALS(A.findLeaf(&A),0);
		TS_ASSERT_EQUALS(A.findLeaf(&B),-1);
		B=A;
		TS_ASSERT_EQUALS(A.findLeaf(&B),-1);
	}

	void testFindKey(){
		Object cpCylinder=createCappedCylinder();
		CompObj A;
		A.setObj(&cpCylinder);
		A.setObjN(10);
		CompObj B;
		TS_ASSERT_EQUALS(A.findKey(10),(Rule*)0); //Always returns 0
		TS_ASSERT_EQUALS(A.findKey(11),(Rule*)0);
	}

	void testIsValid(){
		Object cpCylinder=createCappedCylinder();
		CompObj A;
		A.setObj(&cpCylinder);
		A.setObjN(10);
		TS_ASSERT_EQUALS(A.isValid(V3D(0.0,0.0,0.0)),false); //center is inside the cylinder so it will return complement
		TS_ASSERT_EQUALS(A.isValid(V3D(1.3,0.0,0.0)),true); //outside cap cylinder
		TS_ASSERT_EQUALS(A.isValid(V3D(1.2,0.0,0.0)),false); //on the cap end of cylinder
		TS_ASSERT_EQUALS(A.isValid(V3D(1.1,0.0,0.0)),false); //inside the cap end of cylinder
		TS_ASSERT_EQUALS(A.isValid(V3D(-3.3,0.0,0.0)),true); //outside other end of cap cylinder
		TS_ASSERT_EQUALS(A.isValid(V3D(-3.2,0.0,0.0)),false); //on end of cylinder
		TS_ASSERT_EQUALS(A.isValid(V3D(-3.1,0.0,0.0)),false); //inside the cylinder
		TS_ASSERT_EQUALS(A.isValid(V3D(0.0,3.1,0.0)),true); //outside cylinder
		TS_ASSERT_EQUALS(A.isValid(V3D(0.0,3.0,0.0)),false); //on the cylinder
		TS_ASSERT_EQUALS(A.isValid(V3D(0.0,2.9,0.0)),false); //inside cylinder
		TS_ASSERT_EQUALS(A.isValid(V3D(0.0,0.0,3.1)),true); //outside cylinder
		TS_ASSERT_EQUALS(A.isValid(V3D(0.0,0.0,3.0)),false); //on the cylinder
		TS_ASSERT_EQUALS(A.isValid(V3D(0.0,0.0,2.9)),false); //inside cylinder		
	}

	void testIsValidMap(){
		Object cpCylinder=createCappedCylinder();
		CompObj A;
		A.setObj(&cpCylinder);
		A.setObjN(10);

		std::map<int,int> input;
		input[31]=1;
		input[32]=1;
		input[33]=1;
		TS_ASSERT_EQUALS(A.isValid(input),true);
		input[31]=0;
		TS_ASSERT_EQUALS(A.isValid(input),true);
		input[32]=0;
		TS_ASSERT_EQUALS(A.isValid(input),false);
		input[33]=0;
		TS_ASSERT_EQUALS(A.isValid(input),true);
		input[32]=1;
		TS_ASSERT_EQUALS(A.isValid(input),true);
		input[33]=1;
		TS_ASSERT_EQUALS(A.isValid(input),true);
	}

	void testSimplyfy(){
		Object cpCylinder=createCappedCylinder();
		CompObj A;
		A.setObj(&cpCylinder);
		A.setObjN(10);
		TS_ASSERT_EQUALS(A.simplify(),0); //Always return 0 bcos a end node cannot be simplified
	}
private:
  Object createCappedCylinder()
  {
    std::string C31="cx 3.0";         // cylinder x-axis radius 3
    std::string C32="px 1.2";
    std::string C33="px -3.2";

    // First create some surfaces
    std::map<int,Surface*> CylSurMap;
    CylSurMap[31]=new Cylinder();
    CylSurMap[32]=new Plane();
    CylSurMap[33]=new Plane();

    CylSurMap[31]->setSurface(C31);
    CylSurMap[32]->setSurface(C32);
    CylSurMap[33]->setSurface(C33);
    CylSurMap[31]->setName(31);
    CylSurMap[32]->setName(32);
    CylSurMap[33]->setName(33);

    // Capped cylinder (id 21) 
    // using surface ids: 31 (cylinder) 32 (plane (top) ) and 33 (plane (base))
    std::string ObjCapCylinder="-31 -32 33";

    Object retVal; 
    retVal.setObject(21,ObjCapCylinder);
    retVal.populate(CylSurMap);

    return retVal;
  }

};
//---------------------------------End of CompObj----------------------------------------
class testCompGrp: public CxxTest::TestSuite{
public:
	void testConstructor(){
		CompGrp A;
		TS_ASSERT_EQUALS(A.display(),"");
		TS_ASSERT_EQUALS(A.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(A.isComplementary(),1);
	}

	void testTwoRuleConstructor(){
		Intersection Parent;
		Rule *uSC=createUnionSphereAndCylinder();
		CompGrp A(&Parent,uSC);
		TS_ASSERT_EQUALS(A.getParent(),&Parent);
		TS_ASSERT_EQUALS(A.leaf(0),uSC);
		TS_ASSERT_EQUALS(A.isComplementary(),1);
		TS_ASSERT_EQUALS(A.display(),"#( -10 : -11 )");
	}

	void testCompGrpConstructor(){
		CompGrp A;
		Rule *uSC=createUnionSphereAndCylinder();
		A.setLeaf(uSC,0);
		TS_ASSERT_EQUALS(A.leaf(0),uSC);
		CompGrp B(A);
		TS_ASSERT_EQUALS(B.leaf(0)->display(),uSC->display());
	}

	void testClone(){
		CompGrp A;
		Rule *uSC=createUnionSphereAndCylinder();
		A.setLeaf(uSC,0);
		TS_ASSERT_EQUALS(A.leaf(0),uSC);
		CompGrp *B;
		B=A.clone();
		TS_ASSERT_EQUALS(B->leaf(0)->display(),uSC->display());
		delete B;
	}

	void testAssignment(){
		CompGrp A;
		Rule *uSC=createUnionSphereAndCylinder();
		A.setLeaf(uSC,0);
		TS_ASSERT_EQUALS(A.leaf(0),uSC);
		CompGrp B;
		B=A;
		TS_ASSERT_EQUALS(B.leaf(0)->display(),uSC->display());
	}

	void testSetLeaves(){
		CompGrp A;
		Rule *uSC=createUnionSphereAndCylinder();
		A.setLeaves(uSC,(Rule*)0);
		TS_ASSERT_EQUALS(A.leaf(0),uSC);
		TS_ASSERT_EQUALS(A.display(),"#( -10 : -11 )");
	}

	void testFindLeaf(){
		CompGrp A,B;
		Rule *uSC=createUnionSphereAndCylinder();
		A.setLeaf(uSC,0);
		TS_ASSERT_EQUALS(A.leaf(0),uSC);
		TS_ASSERT_EQUALS(A.findLeaf(uSC),0);
		TS_ASSERT_EQUALS(A.findLeaf(&B),-1);
	}

	void testFindKey(){
		CompGrp A;
		Rule *uSC=createUnionSphereAndCylinder();
		A.setLeaf(uSC,0);
		TS_ASSERT_EQUALS(A.leaf(0),uSC);
		TS_ASSERT_EQUALS(A.findKey(0),(Rule*)0); //Always returns 0
	}

	void testIsValid(){
		CompGrp A;
		Rule *uSC=createUnionSphereAndCylinder();
		A.setLeaf(uSC,0);
		TS_ASSERT_EQUALS(A.leaf(0),uSC);
		TS_ASSERT_EQUALS(A.isValid(V3D(0.0,0.0,0.0)),false); //inside the sphere and cylinder 
		TS_ASSERT_EQUALS(A.isValid(V3D(4.1,0.0,0.0)),true); //outside sphere
		TS_ASSERT_EQUALS(A.isValid(V3D(4.0,0.0,0.0)),false); //on sphere
		TS_ASSERT_EQUALS(A.isValid(V3D(3.9,0.0,0.0)),false); //inside sphere
		TS_ASSERT_EQUALS(A.isValid(V3D(1.1,4.0,0.0)),true); //outside cylinder
		TS_ASSERT_EQUALS(A.isValid(V3D(1.0,4.0,0.0)),false); //on cylinder
		TS_ASSERT_EQUALS(A.isValid(V3D(0.9,4.0,0.0)),false); //inside cylinder
	}

	void testIsValidMap(){
		CompGrp A;
		Rule *uSC=createUnionSphereAndCylinder();
		A.setLeaf(uSC,0);
		TS_ASSERT_EQUALS(A.leaf(0),uSC);
		std::map<int,int> input;
		input[10]=1;
		input[11]=1;
		TS_ASSERT_EQUALS(A.isValid(input),true);
		input[10]=0;
		TS_ASSERT_EQUALS(A.isValid(input),false);
		input[11]=0;
		TS_ASSERT_EQUALS(A.isValid(input),false);
		input[10]=1;
		TS_ASSERT_EQUALS(A.isValid(input),false);
	}

	void testSimplyfy(){
		CompGrp A;
		Rule *uSC=createUnionSphereAndCylinder();
		A.setLeaf(uSC,0);
		TS_ASSERT_EQUALS(A.leaf(0),uSC);
		TS_ASSERT_EQUALS(A.simplify(),0); //Always return 0 bcos a single node cannot be simplified
	}

private:
	Rule* createUnionSphereAndCylinder(){
		SurfPoint *sR1,*sR2;
		sR1=new SurfPoint();
		sR2=new SurfPoint();
		Sphere *sP = new Sphere();
		sP->setSurface("s 2.0 0.0 0.0 2"); 
		sR1->setKey(sP);//Sphere
		sR1->setKeyN(-10);
		Cylinder *cP = new Cylinder();
		cP->setSurface("cy 1.0");
		sR2->setKey(cP);//cappedcylinder
		sR2->setKeyN(-11);
		Union *uR1;
		uR1=new Union(sR1,sR2);
		return uR1;
	}
};
//---------------------------------End of CompGrp----------------------------------------
class testBoolValue: public CxxTest::TestSuite{
public:
	void testConstructor(){
		BoolValue A;
		TS_ASSERT_EQUALS(A.display()," Unknown ");
		TS_ASSERT_EQUALS(A.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(A.leaf(1),(Rule*)0);
		A.setStatus(0);
		TS_ASSERT_EQUALS(A.display()," False ");
		A.setStatus(1);
		TS_ASSERT_EQUALS(A.display()," True ");
	}

	void testBoolValueConstructor(){
		BoolValue A;
		TS_ASSERT_EQUALS(A.display()," Unknown ");
		TS_ASSERT_EQUALS(A.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(A.leaf(1),(Rule*)0);
		A.setStatus(0);
		TS_ASSERT_EQUALS(A.display()," False ");
		BoolValue B(A);
		TS_ASSERT_EQUALS(B.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(B.leaf(1),(Rule*)0);
		TS_ASSERT_EQUALS(B.display()," False ");
	}

	void testClone(){
		BoolValue A;
		TS_ASSERT_EQUALS(A.display()," Unknown ");
		TS_ASSERT_EQUALS(A.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(A.leaf(1),(Rule*)0);
		A.setStatus(0);
		TS_ASSERT_EQUALS(A.display()," False ");
		BoolValue *B;
		B=A.clone();
		TS_ASSERT_EQUALS(B->leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(B->leaf(1),(Rule*)0);
		TS_ASSERT_EQUALS(B->display()," False ");
		delete B;
	}

	void testAssignment(){
		BoolValue A;
		TS_ASSERT_EQUALS(A.display()," Unknown ");
		TS_ASSERT_EQUALS(A.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(A.leaf(1),(Rule*)0);
		A.setStatus(0);
		TS_ASSERT_EQUALS(A.display()," False ");
		BoolValue B;
		TS_ASSERT_EQUALS(B.display()," Unknown ");
		B=A;
		TS_ASSERT_EQUALS(B.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(B.leaf(1),(Rule*)0);
		TS_ASSERT_EQUALS(B.display()," False ");
	}

	void testLeafOperations(){
		BoolValue A;
		TS_ASSERT_EQUALS(A.display()," Unknown ");
		TS_ASSERT_EQUALS(A.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(A.leaf(1),(Rule*)0);
		A.setStatus(0);
		TS_ASSERT_EQUALS(A.display()," False ");
		BoolValue *B=new BoolValue();
		TS_ASSERT_EQUALS(B->display()," Unknown ");
		B->setStatus(1);
		A.setLeaves(B,(Rule*)0);
		TS_ASSERT_EQUALS(A.display()," True ");
		BoolValue *C=new BoolValue();
		TS_ASSERT_EQUALS(C->display()," Unknown ");
		C->setStatus(0);
		A.setLeaf(C,1);
		TS_ASSERT_EQUALS(A.display()," False ");
		delete B;
		delete C;
	}

	void testFindOperations(){
		BoolValue A;
		TS_ASSERT_EQUALS(A.display()," Unknown ");
		TS_ASSERT_EQUALS(A.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(A.leaf(1),(Rule*)0);
		A.setStatus(0);
		TS_ASSERT_EQUALS(A.display()," False ");
		BoolValue *B=new BoolValue();
		TS_ASSERT_EQUALS(B->display()," Unknown ");
		B->setStatus(1);
		A.setLeaves(B,(Rule*)0);
		TS_ASSERT_EQUALS(A.display()," True ");
		TS_ASSERT_EQUALS(A.findLeaf(&A),0);
		TS_ASSERT_EQUALS(A.findLeaf(B),-1);
		TS_ASSERT_EQUALS(A.findKey(0),(Rule*)0);
		delete B;
	}

	void testIsValid(){
		BoolValue A;
		TS_ASSERT_EQUALS(A.display()," Unknown ");
		TS_ASSERT_EQUALS(A.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(A.leaf(1),(Rule*)0);
		A.setStatus(0);
		TS_ASSERT_EQUALS(A.display()," False ");
		TS_ASSERT_EQUALS(A.isValid(V3D(0,0,0)),false);
		A.setStatus(-1);
		TS_ASSERT_EQUALS(A.isValid(V3D(0,0,0)),false);
		A.setStatus(1);
		TS_ASSERT_EQUALS(A.isValid(V3D(0,0,0)),true);

		std::map<int,int> input;
		input[0]=0;
		input[5]=1;
		input[10]=1;
		input[15]=0;
		input[20]=-1;
		TS_ASSERT_EQUALS(A.isValid(input),true);
		A.setStatus(0);
		TS_ASSERT_EQUALS(A.isValid(input),false);
	}
	void testSimplyfy(){
		BoolValue A;
		TS_ASSERT_EQUALS(A.display()," Unknown ");
		TS_ASSERT_EQUALS(A.leaf(0),(Rule*)0);
		TS_ASSERT_EQUALS(A.leaf(1),(Rule*)0);
		A.setStatus(0);
		TS_ASSERT_EQUALS(A.display()," False ");
		TS_ASSERT_EQUALS(A.simplify(),0); //Always return 0 bcos a end node cannot be simplified
	}
};
#endif
