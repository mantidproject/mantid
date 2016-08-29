#ifndef MANTID_INDEXING_MAKERANGETEST_H_
#define MANTID_INDEXING_MAKERANGETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/MakeRange.h"

using Mantid::Indexing::makeRange;

class MakeRangeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MakeRangeTest *createSuite() { return new MakeRangeTest(); }
  static void destroySuite(MakeRangeTest *suite) { delete suite; }

  void test_int() {
    TS_ASSERT_EQUALS(makeRange(1, 3), (std::vector<int>{1, 2, 3}));
  }
};

#endif /* MANTID_INDEXING_MAKERANGETEST_H_ */
