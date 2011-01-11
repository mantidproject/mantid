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

class RulesTest: public CxxTest::TestSuite{
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

#endif
