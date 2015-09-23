#ifndef MANTID_GEOMETRY_BASICHKLFILTERSTEST_H_
#define MANTID_GEOMETRY_BASICHKLFILTERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/BasicHKLFilters.h"

class HKLFilterDRangeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HKLFilterDRangeTest *createSuite() { return new HKLFilterDRangeTest(); }
  static void destroySuite( HKLFilterDRangeTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_GEOMETRY_BASICHKLFILTERSTEST_H_ */
