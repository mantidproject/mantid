#ifndef MANTID_TESTOBJCOMPNENT__
#define MANTID_TESTOBJCOMPNENT__

#include <cxxtest/TestSuite.h>
#include "../inc/ObjComponent.h" 
#include "../inc/Component.h" 

using namespace Mantid;
using namespace Geometry;

class testObjComponent : public CxxTest::TestSuite
{
public:
  void testEmptyConstructor()
    {
      ObjComponent objComp;
      TS_ASSERT_EQUALS(objComp.getName(),"");
      TS_ASSERT(!objComp.getParent());
    }
  void testNameConstructor()
    {
      ObjComponent objComp("objComp1");
      TS_ASSERT_EQUALS(objComp.getName(),"objComp1");
      TS_ASSERT(!objComp.getParent());
    }	
  void testNameParentConstructor()
    {
      Component parent("Parent");
      ObjComponent objComp("objComp1", &parent);
      TS_ASSERT_EQUALS(objComp.getName(),"objComp1");
      TS_ASSERT(objComp.getParent());
    }
  
  void testType()
    {
      ObjComponent objComp;
      TS_ASSERT_EQUALS(objComp.type(),"PhysicalComponent");
    }
};

#endif
