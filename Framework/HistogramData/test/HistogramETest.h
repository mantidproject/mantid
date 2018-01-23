#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMETEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Addable.h"
#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/HistogramE.h"

using namespace Mantid;
using namespace HistogramData;

class HistogramETest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramETest *createSuite() { return new HistogramETest(); }
  static void destroySuite(HistogramETest *suite) { delete suite; }

  // The implementation of the testee is based on mixins and a few
  // compiler-generated constructors. Tests for the features provided by mixins
  // can be found in the respective mixin tests. Here we simply test that we
  // have the expected mixins.

  void test_has_correct_mixins() {
    HistogramE e;
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG(dynamic_cast<detail::FixedLengthVector<HistogramE> &>(e)));
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG(dynamic_cast<detail::Addable<HistogramE> &>(e)));
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG(dynamic_cast<detail::Scalable<HistogramE> &>(e)));
  }
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMETEST_H_ */
