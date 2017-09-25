#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMXTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/Offsetable.h"
#include "MantidHistogramData/Scalable.h"

using namespace Mantid;
using namespace HistogramData;

class HistogramXTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramXTest *createSuite() { return new HistogramXTest(); }
  static void destroySuite(HistogramXTest *suite) { delete suite; }

  // The implementation of the testee is based on mixins and a few
  // compiler-generated constructors. Tests for the features provided by mixins
  // can be found in the respective mixin tests. Here we simply test that we
  // have the expected mixins.

  void test_has_correct_mixins() {
    HistogramX x;
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG(dynamic_cast<detail::FixedLengthVector<HistogramX> &>(x)));
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG(dynamic_cast<detail::Offsetable<HistogramX> &>(x)));
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG(dynamic_cast<detail::Scalable<HistogramX> &>(x)));
  }
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMXTEST_H_ */
