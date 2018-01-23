#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMDXTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMDXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/HistogramDx.h"

using namespace Mantid;
using namespace HistogramData;

class HistogramDxTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramDxTest *createSuite() { return new HistogramDxTest(); }
  static void destroySuite(HistogramDxTest *suite) { delete suite; }

  // The implementation of the testee is based on mixins and a few
  // compiler-generated constructors. Tests for the features provided by mixins
  // can be found in the respective mixin tests. Here we simply test that we
  // have the expected mixins.

  void test_has_correct_mixins() {
    HistogramDx dx;
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG(dynamic_cast<detail::FixedLengthVector<HistogramDx> &>(dx)));
  }
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMDXTEST_H_ */
