#ifndef MANTID_GEOMETRY_V3RTEST_H_
#define MANTID_GEOMETRY_V3RTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/V3R.h"

using namespace Mantid::Geometry;

class V3RTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static V3RTest *createSuite() { return new V3RTest(); }
  static void destroySuite( V3RTest *suite ) { delete suite; }


  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_GEOMETRY_V3RTEST_H_ */
