#ifndef MANTID_HISTOGRAMDATA_POINTVARIANCESTEST_H_
#define MANTID_HISTOGRAMDATA_POINTVARIANCESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/PointStandardDeviations.h"
#include "MantidHistogramData/PointVariances.h"

using namespace Mantid;
using namespace HistogramData;

class PointVariancesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PointVariancesTest *createSuite() { return new PointVariancesTest(); }
  static void destroySuite(PointVariancesTest *suite) { delete suite; }

  void test_has_correct_mixins() {
    PointVariances data;
    TS_ASSERT_THROWS_NOTHING(UNUSED_ARG(
        (dynamic_cast<detail::VarianceVectorOf<
             PointVariances, HistogramDx, PointStandardDeviations> &>(data))));
  }

  void test_construct_default() {
    const PointVariances points{};
    TS_ASSERT(!points);
  }

  void test_conversion_identity() {
    const PointVariances variances{1.0, 4.0, 9.0};
    const PointStandardDeviations sigmas(variances);
    const PointVariances result(sigmas);
    TS_ASSERT_EQUALS(result[0], variances[0]);
    TS_ASSERT_EQUALS(result[1], variances[1]);
    TS_ASSERT_EQUALS(result[2], variances[2]);
  }
};

#endif /* MANTID_HISTOGRAMDATA_POINTVARIANCESTEST_H_ */
