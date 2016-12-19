#ifndef MANTID_INDEXING_DETECTORIDTEST_H_
#define MANTID_INDEXING_DETECTORIDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/DetectorID.h"

using namespace Mantid;
using namespace Indexing;

class DetectorIDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorIDTest *createSuite() { return new DetectorIDTest(); }
  static void destroySuite(DetectorIDTest *suite) { delete suite; }

  void test_has_correct_mixins() {
    DetectorID data(0);
// AppleClang gives warning if the result is unused.
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#endif
    TS_ASSERT_THROWS_NOTHING(
        (dynamic_cast<detail::IndexType<DetectorID, int32_t> &>(data)));
#if __clang__
#pragma clang diagnostic pop
#endif
  }
};

#endif /* MANTID_INDEXING_DETECTORIDTEST_H_ */
