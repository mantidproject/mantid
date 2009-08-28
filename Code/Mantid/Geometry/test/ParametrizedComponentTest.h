#ifndef MANTID_TESTPARCOMPONENT__
#define MANTID_TESTPARCOMPONENT__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <iostream>
#include <string>
#include "MantidGeometry/Instrument/ParametrizedComponent.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Geometry;

class testParComponent : public CxxTest::TestSuite
{
public:
  void testEmptyConstructor()
  {
    Component q;

    ParameterMap pmap;
    ParametrizedComponent pq(&q,pmap);

    TS_ASSERT_EQUALS(pq.getName(),"");
    TS_ASSERT(!pq.getParent());
    TS_ASSERT_EQUALS(pq.getRelativePos(),V3D(0,0,0));
    TS_ASSERT_EQUALS(pq.getRelativeRot(),Quat(1,0,0,0));
    //as there is no parent GetPos should equal getRelativePos
    TS_ASSERT_EQUALS(pq.getRelativePos(),pq.getPos());
  }

  void testNameLocationOrientationParentValueConstructor()
  {
    Component parent("Parent",V3D(1,1,1));
    //name and parent
    Component q("Child",V3D(5,6,7),Quat(1,1,1,1),&parent);
    ParameterMap pmap;
    ParametrizedComponent pq(&q,pmap);

    TS_ASSERT_EQUALS(pq.getName(),"Child");
    //check the parent
    TS_ASSERT(pq.getParent());
    TS_ASSERT_EQUALS(pq.getParent()->getName(),parent.getName());

    TS_ASSERT_EQUALS(pq.getRelativePos(),V3D(5,6,7));
    TS_ASSERT_EQUALS(pq.getPos(),V3D(6,7,8));
    TS_ASSERT_EQUALS(pq.getRelativeRot(),Quat(1,1,1,1));
  }


};

#endif
