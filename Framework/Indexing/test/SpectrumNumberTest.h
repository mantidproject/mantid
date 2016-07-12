#ifndef MANTID_INDEXING_SPECTRUMNUMBERTEST_H_
#define MANTID_INDEXING_SPECTRUMNUMBERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidIndexing/SpectrumNumber.h"

using namespace Mantid;
using namespace Indexing;

class SpectrumNumberTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectrumNumberTest *createSuite() { return new SpectrumNumberTest(); }
  static void destroySuite(SpectrumNumberTest *suite) { delete suite; }

  void test_has_correct_mixins() {
    SpectrumNumber data(0);
// AppleClang gives warning if the result is unused.
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#endif
    TS_ASSERT_THROWS_NOTHING(
        (dynamic_cast<detail::IndexType<SpectrumNumber, int32_t> &>(data)));
#if __clang__
#pragma clang diagnostic pop
#endif
  }
};

#endif /* MANTID_INDEXING_SPECTRUMNUMBERTEST_H_ */
