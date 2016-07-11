#ifndef MANTID_INDEXING_SPECTRUMINDEXSETTEST_H_
#define MANTID_INDEXING_SPECTRUMINDEXSETTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/SpectrumIndexSet.h"

using namespace Mantid;
using namespace Indexing;

class SpectrumIndexSetTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectrumIndexSetTest *createSuite() {
    return new SpectrumIndexSetTest();
  }
  static void destroySuite(SpectrumIndexSetTest *suite) { delete suite; }

  void test_has_correct_mixins() {
    SpectrumIndexSet data(0);
// AppleClang gives warning if the result is unused.
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#endif
    TS_ASSERT_THROWS_NOTHING(
        (dynamic_cast<detail::IndexSet<SpectrumIndexSet> &>(data)));
#if __clang__
#pragma clang diagnostic pop
#endif
  }
};

#endif /* MANTID_INDEXING_SPECTRUMINDEXSETTEST_H_ */
