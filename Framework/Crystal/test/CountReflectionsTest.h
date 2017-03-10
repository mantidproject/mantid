#ifndef MANTID_CRYSTAL_COUNTPEAKSTEST_H_
#define MANTID_CRYSTAL_COUNTPEAKSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/CountReflections.h"

using Mantid::Crystal::CountReflections;

class CountReflectionsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CountReflectionsTest *createSuite() { return new CountReflectionsTest(); }
  static void destroySuite(CountReflectionsTest *suite) { delete suite; }

  void test_nothing() { TS_ASSERT(false); }
};

#endif /* MANTID_CRYSTAL_COUNTPEAKSTEST_H_ */
