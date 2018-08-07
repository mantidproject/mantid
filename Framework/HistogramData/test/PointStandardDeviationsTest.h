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
    TS_ASSERT_THROWS_NOTHING(UNUSED_ARG(
        (dynamic_cast<detail::StandardDeviationVectorOf<
             PointStandardDeviations, HistogramDx, PointVariances> &>(data))));
  }

  void test_construct_default() {
    const PointStandardDeviations points{};
    TS_ASSERT(!points);
  }
};

#endif /* MANTID_HISTOGRAMDATA_POINTSTANDARDDEVIATIONSTEST_H_ */
