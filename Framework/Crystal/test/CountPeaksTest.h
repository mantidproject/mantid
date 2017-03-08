#ifndef MANTID_CRYSTAL_COUNTPEAKSTEST_H_
#define MANTID_CRYSTAL_COUNTPEAKSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/CountPeaks.h"

using Mantid::Crystal::CountPeaks;

class CountPeaksTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CountPeaksTest *createSuite() { return new CountPeaksTest(); }
  static void destroySuite(CountPeaksTest *suite) { delete suite; }

  void test_nothing() { TS_ASSERT(false); }
};

#endif /* MANTID_CRYSTAL_COUNTPEAKSTEST_H_ */
