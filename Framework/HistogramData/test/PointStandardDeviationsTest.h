#ifndef MANTID_HISTOGRAMDATA_POINTSTANDARDDEVIATIONSTEST_H_
#define MANTID_HISTOGRAMDATA_POINTSTANDARDDEVIATIONSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/PointStandardDeviations.h"

using namespace Mantid;
using namespace HistogramData;

class PointStandardDeviationsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PointStandardDeviationsTest *createSuite() {
    return new PointStandardDeviationsTest();
  }
  static void destroySuite(PointStandardDeviationsTest *suite) { delete suite; }

  void test_has_correct_mixins() {
    PointStandardDeviations data;
// AppleClang gives warning if the result is unused.
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#endif
    TS_ASSERT_THROWS_NOTHING((dynamic_cast<detail::StandardDeviationVectorOf<
        PointStandardDeviations, HistogramDx, PointVariances> &>(data)));
#if __clang__
#pragma clang diagnostic pop
#endif
  }

  void test_construct_default() {
    const PointStandardDeviations points{};
    TS_ASSERT(!points);
  }
};

#endif /* MANTID_HISTOGRAMDATA_POINTSTANDARDDEVIATIONSTEST_H_ */
