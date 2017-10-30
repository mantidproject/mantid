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
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG((dynamic_cast<detail::IndexSet<SpectrumIndexSet> &>(data))));
  }
};

#endif /* MANTID_INDEXING_SPECTRUMINDEXSETTEST_H_ */
