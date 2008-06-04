#ifndef MANTID_TESTMATERIAL__
#define MANTID_TESTMATERIAL__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>
#include <sstream>

#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"

#include "MantidGeometry/Material.h"


using namespace Mantid;
using namespace Geometry;

class testMaterial: public CxxTest::TestSuite
{
public:
	void testConstructor(){//no way to check the Absorbstion crosssection
		Material A;
		TS_ASSERT_EQUALS(A.getAtomDensity(),0.0);
		TS_ASSERT_EQUALS(A.getName(),"");
		TS_ASSERT_EQUALS(A.getScat(),0.0);
		TS_ASSERT_EQUALS(A.getCoh(),0.0);
		TS_ASSERT_EQUALS(A.getInc(),0.0);
	}

	void testConstructorParams1(){
		Material A("Rb", 1.0, 20.0,2.0,1.5);
		TS_ASSERT_EQUALS(A.getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(A.getName(),"Rb");
		TS_ASSERT_EQUALS(A.getScat(),22.0);
		TS_ASSERT_EQUALS(A.getCoh(),20.0);
		TS_ASSERT_EQUALS(A.getInc(),2.0);
	}

	void testConstructorParams2(){
		Material A( 1.0, 20.0,2.0,1.5);
		TS_ASSERT_EQUALS(A.getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(A.getName(),"");
		TS_ASSERT_EQUALS(A.getScat(),22.0);
		TS_ASSERT_EQUALS(A.getCoh(),20.0);
		TS_ASSERT_EQUALS(A.getInc(),2.0);
	}

	void testConstructorParamMaterial(){
		Material A("Rb", 1.0, 20.0,2.0,1.5);
		TS_ASSERT_EQUALS(A.getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(A.getName(),"Rb");
		TS_ASSERT_EQUALS(A.getScat(),22.0);
		TS_ASSERT_EQUALS(A.getCoh(),20.0);
		TS_ASSERT_EQUALS(A.getInc(),2.0);
		Material B(A);
		TS_ASSERT_EQUALS(B.getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(B.getName(),"Rb");
		TS_ASSERT_EQUALS(B.getScat(),22.0);
		TS_ASSERT_EQUALS(B.getCoh(),20.0);
		TS_ASSERT_EQUALS(B.getInc(),2.0);
	}

	void testClone(){
		Material A("Rb", 1.0, 20.0,2.0,1.5);
		TS_ASSERT_EQUALS(A.getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(A.getName(),"Rb");
		TS_ASSERT_EQUALS(A.getScat(),22.0);
		TS_ASSERT_EQUALS(A.getCoh(),20.0);
		TS_ASSERT_EQUALS(A.getInc(),2.0);
		Material *B;
		B=A.clone();
		TS_ASSERT_EQUALS(B->getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(B->getName(),"Rb");
		TS_ASSERT_EQUALS(B->getScat(),22.0);
		TS_ASSERT_EQUALS(B->getCoh(),20.0);
		TS_ASSERT_EQUALS(B->getInc(),2.0)
	}

	void testAssignment(){
		Material A("Rb", 1.0, 20.0,2.0,1.5);
		TS_ASSERT_EQUALS(A.getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(A.getName(),"Rb");
		TS_ASSERT_EQUALS(A.getScat(),22.0);
		TS_ASSERT_EQUALS(A.getCoh(),20.0);
		TS_ASSERT_EQUALS(A.getInc(),2.0);
		Material B;
		TS_ASSERT_EQUALS(B.getAtomDensity(),0.0);
		TS_ASSERT_EQUALS(B.getName(),"");
		TS_ASSERT_EQUALS(B.getScat(),0.0);
		TS_ASSERT_EQUALS(B.getCoh(),0.0);
		TS_ASSERT_EQUALS(B.getInc(),0.0);
		B=A;
		TS_ASSERT_EQUALS(B.getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(B.getName(),"Rb");
		TS_ASSERT_EQUALS(B.getScat(),22.0);
		TS_ASSERT_EQUALS(B.getCoh(),20.0);
		TS_ASSERT_EQUALS(B.getInc(),2.0);
	}

	void testSetName(){
		Material A("Rb", 1.0, 20.0,2.0,1.5);
		TS_ASSERT_EQUALS(A.getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(A.getName(),"Rb");
		TS_ASSERT_EQUALS(A.getScat(),22.0);
		TS_ASSERT_EQUALS(A.getCoh(),20.0);
		TS_ASSERT_EQUALS(A.getInc(),2.0);
		A.setName("Cu");
		TS_ASSERT_EQUALS(A.getName(),"Cu");
		A.setName("");
		TS_ASSERT_EQUALS(A.getName(),"");
		//A.setName(NULL);
		//TS_ASSERT_EQUALS(A.getName(),NULL);
	}

	void testSetDensity(){
		Material A("Rb", 1.0, 20.0,2.0,1.5);
		TS_ASSERT_EQUALS(A.getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(A.getName(),"Rb");
		TS_ASSERT_EQUALS(A.getScat(),22.0);
		TS_ASSERT_EQUALS(A.getCoh(),20.0);
		TS_ASSERT_EQUALS(A.getInc(),2.0);
		A.setDensity(5.0);
		TS_ASSERT_EQUALS(A.getAtomDensity(),5.0);
		A.setDensity(-1.0); //Can atom density be less than 0?????
		TS_ASSERT_EQUALS(A.getAtomDensity(),-1.0);
		A.setDensity(0.0);
		TS_ASSERT_EQUALS(A.getAtomDensity(),0.0);	
		TS_ASSERT_EQUALS(A.getScat(),22.0);
		TS_ASSERT_EQUALS(A.getCoh(),20.0);
		TS_ASSERT_EQUALS(A.getInc(),2.0);
	}

	void testSetScat(){
		Material A("Rb", 1.0, 20.0,2.0,1.5);
		TS_ASSERT_EQUALS(A.getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(A.getName(),"Rb");
		TS_ASSERT_EQUALS(A.getScat(),22.0);
		TS_ASSERT_EQUALS(A.getCoh(),20.0);
		TS_ASSERT_EQUALS(A.getInc(),2.0);
		A.setScat(2.0,0.2,0.1);
		TS_ASSERT_EQUALS(A.getScat(),2.2);
		TS_ASSERT_EQUALS(A.getCoh(),2.0);
		TS_ASSERT_EQUALS(A.getInc(),0.2);
	}

	void testGetScatFrac(){
		Material A("Rb", 1.0, 20.0,2.0,1.5);
		TS_ASSERT_EQUALS(A.getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(A.getName(),"Rb");
		TS_ASSERT_EQUALS(A.getScat(),22.0);
		TS_ASSERT_EQUALS(A.getCoh(),20.0);
		TS_ASSERT_EQUALS(A.getInc(),2.0);
		TS_ASSERT_DELTA(A.getScatFrac(1),0.9634645,0.000001);
		TS_ASSERT_DELTA(A.getScatFrac(1.798),0.9361702,0.0000001);
		TS_ASSERT_DELTA(A.getScatFrac(0),1.0,0.0000001);
		A.setDensity(-1.0);
		TS_ASSERT_EQUALS(A.getScatFrac(1),1.0);
	}

	void testGetAtten(){
		Material A("Rb", 1.0, 20.0,2.0,1.5);
		TS_ASSERT_EQUALS(A.getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(A.getName(),"Rb");
		TS_ASSERT_EQUALS(A.getScat(),22.0);
		TS_ASSERT_EQUALS(A.getCoh(),20.0);
		TS_ASSERT_EQUALS(A.getInc(),2.0);
		TS_ASSERT_DELTA(A.getAtten(1),20.8342602,0.000001);
		TS_ASSERT_DELTA(A.getAtten(1.798),21.5,0.0000001);
		TS_ASSERT_DELTA(A.getAtten(0),20.0,0.0000001);
		A.setDensity(0);
		TS_ASSERT_EQUALS(A.getAtten(1),0);
	}

	void testGetAttenAbs(){
		Material A("Rb", 1.0, 20.0,2.0,1.5);
		TS_ASSERT_EQUALS(A.getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(A.getName(),"Rb");
		TS_ASSERT_EQUALS(A.getScat(),22.0);
		TS_ASSERT_EQUALS(A.getCoh(),20.0);
		TS_ASSERT_EQUALS(A.getInc(),2.0);
		TS_ASSERT_DELTA(A.getAttenAbs(1),0.8342602,0.000001);
		TS_ASSERT_DELTA(A.getAttenAbs(1.798),1.5,0.0000001);
		TS_ASSERT_DELTA(A.getAttenAbs(0),0.0,0.0000001);
		A.setDensity(0);
		TS_ASSERT_EQUALS(A.getAttenAbs(1),0);
	}

	void testCalcAtten(){
		Material A("Rb", 1.0, 20.0,2.0,1.5);
		TS_ASSERT_EQUALS(A.getAtomDensity(),1.0);
		TS_ASSERT_EQUALS(A.getName(),"Rb");
		TS_ASSERT_EQUALS(A.getScat(),22.0);
		TS_ASSERT_EQUALS(A.getCoh(),20.0);
		TS_ASSERT_EQUALS(A.getInc(),2.0);
		TS_ASSERT_DELTA(A.calcAtten(1,1.2),0.0,0.000001);
		TS_ASSERT_DELTA(A.calcAtten(1.798,1.2),0.0,0.001);
		TS_ASSERT_DELTA(A.calcAtten(0,1.2),0,0.0000001);
	}
};
#endif
