#ifndef MANTID_TESTDETECTOR__
#define MANTID_TESTDETECTOR__

#include <cxxtest/TestSuite.h>
#include "../inc/Detector.h" 
#include "../inc/Component.h" 

using namespace Mantid;
using namespace Geometry;

class testDetector : public CxxTest::TestSuite
{
public:
  void testEmptyConstructor()
    {
      Detector det;
      TS_ASSERT_EQUALS(det.getName(),"");
      TS_ASSERT(!det.getParent());
      TS_ASSERT_EQUALS(det.getID(),0);
    }
  void testNameConstructor()
    {
      Detector det("det1");
      TS_ASSERT_EQUALS(det.getName(),"det1");
      TS_ASSERT(!det.getParent());
      TS_ASSERT_EQUALS(det.getID(),0);
    }	
	void testNameParentConstructor()
	{
		Component parent("Parent");
		Detector det("det1", &parent);
		TS_ASSERT_EQUALS(det.getName(),"det1");
		TS_ASSERT(det.getParent());
		TS_ASSERT_EQUALS(det.getID(),0);
	}
	void testSetId()
	{
		int id1=41;
		int id2=-43;
		Detector det("det1");
		TS_ASSERT_EQUALS(det.getID(),0);
		det.setID(id1);
		TS_ASSERT_EQUALS(det.getID(),id1);
		det.setID(id2);
		TS_ASSERT_EQUALS(det.getID(),id2);
	}

	void testType()
	{
		Detector det;
		TS_ASSERT_EQUALS(det.type(),"DetectorComponent");
	}
	
	void testDead()
	{
	  Detector det;
	  TS_ASSERT( ! det.isDead() )
	  TS_ASSERT_THROWS_NOTHING( det.markDead() )
    TS_ASSERT( det.isDead() )
    // Re-flagging as dead doesn't throw, just prints a warning
    TS_ASSERT_THROWS_NOTHING( det.markDead() )
    TS_ASSERT( det.isDead() )	  
	}
};

#endif
