#ifndef MANTID_GEOMETRY_HKLFILTERWAVELENGTHTEST_H_
#define MANTID_GEOMETRY_HKLFILTERWAVELENGTHTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Crystal/HKLFilterWavelength.h"

using Mantid::Geometry::HKLFilterWavelength;

class HKLFilterWavelengthTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HKLFilterWavelengthTest *createSuite() { return new HKLFilterWavelengthTest(); }
  static void destroySuite( HKLFilterWavelengthTest *suite ) { delete suite; }


  void test_Something()
  {
    TS_FAIL( "You forgot to write a test!");
  }


};


#endif /* MANTID_GEOMETRY_HKLFILTERWAVELENGTHTEST_H_ */