#ifndef MANTID_RULESCOMPGRPTEST__
#define MANTID_RULESCOMPGRPTEST__
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

class RulesCompGrpTest: public CxxTest::TestSuite{
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


#endif
