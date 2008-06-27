#ifndef MANTID_TESTOBJCOMPNENT__
#define MANTID_TESTOBJCOMPNENT__

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/ObjComponent.h"
#include "MantidGeometry/Component.h"

using namespace Mantid::Geometry;

class ObjComponentTest : public CxxTest::TestSuite
{
public:
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
    ObjComponent objComp("objComp");
    TS_ASSERT_EQUALS(objComp.type(),"PhysicalComponent");
  }
};

#endif
