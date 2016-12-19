#ifndef MANTID_INDEXING_DETECTORINDEXSETTEST_H_
#define MANTID_INDEXING_DETECTORINDEXSETTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/DetectorIndexSet.h"

using namespace Mantid;
using namespace Indexing;

class DetectorIndexSetTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorIndexSetTest *createSuite() {
    return new DetectorIndexSetTest();
  }
  static void destroySuite(DetectorIndexSetTest *suite) { delete suite; }

  void test_has_correct_mixins() {
    DetectorIndexSet data(0);
// AppleClang gives warning if the result is unused.
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#endif
    TS_ASSERT_THROWS_NOTHING(
        (dynamic_cast<detail::IndexSet<DetectorIndexSet> &>(data)));
#if __clang__
#pragma clang diagnostic pop
#endif
  }
};

#endif /* MANTID_INDEXING_DETECTORINDEXSETTEST_H_ */
