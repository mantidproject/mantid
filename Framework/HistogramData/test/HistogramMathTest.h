#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMMATHTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMMATHTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramMath.h"

using namespace Mantid::HistogramData;

class HistogramMathTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramMathTest *createSuite() { return new HistogramMathTest(); }
  static void destroySuite(HistogramMathTest *suite) { delete suite; }

  void test_times_equals() {
    Histogram hist(BinEdges{1, 2, 3}, Counts{4, 9});
    hist *= 3.0;
    TS_ASSERT_EQUALS(hist.x()[0], 1.0);
    TS_ASSERT_EQUALS(hist.x()[1], 2.0);
    TS_ASSERT_EQUALS(hist.x()[2], 3.0);
    TS_ASSERT_EQUALS(hist.y()[0], 12.0);
    TS_ASSERT_EQUALS(hist.y()[1], 27.0);
    TS_ASSERT_EQUALS(hist.e()[0], 6.0);
    TS_ASSERT_EQUALS(hist.e()[1], 9.0);
  }

  void test_divide_equals() {
    Histogram hist(BinEdges{1, 2, 3}, Counts{4, 9});
    hist /= 0.5;
    TS_ASSERT_EQUALS(hist.x()[0], 1.0);
    TS_ASSERT_EQUALS(hist.x()[1], 2.0);
    TS_ASSERT_EQUALS(hist.x()[2], 3.0);
    TS_ASSERT_EQUALS(hist.y()[0], 8.0);
    TS_ASSERT_EQUALS(hist.y()[1], 18.0);
    TS_ASSERT_EQUALS(hist.e()[0], 4.0);
    TS_ASSERT_EQUALS(hist.e()[1], 6.0);
  }

  void test_times() {
    const Histogram hist(BinEdges{1, 2, 3}, Counts{4, 9});
    auto result = hist * 3.0;
    TS_ASSERT_EQUALS(result.x()[0], 1.0);
    TS_ASSERT_EQUALS(result.x()[1], 2.0);
    TS_ASSERT_EQUALS(result.x()[2], 3.0);
    TS_ASSERT_EQUALS(result.y()[0], 12.0);
    TS_ASSERT_EQUALS(result.y()[1], 27.0);
    TS_ASSERT_EQUALS(result.e()[0], 6.0);
    TS_ASSERT_EQUALS(result.e()[1], 9.0);
  }

  void test_times_reverse_order() {
    const Histogram hist(BinEdges{1, 2, 3}, Counts{4, 9});
    auto result = 3.0 * hist;
    TS_ASSERT_EQUALS(result.x()[0], 1.0);
    TS_ASSERT_EQUALS(result.x()[1], 2.0);
    TS_ASSERT_EQUALS(result.x()[2], 3.0);
    TS_ASSERT_EQUALS(result.y()[0], 12.0);
    TS_ASSERT_EQUALS(result.y()[1], 27.0);
    TS_ASSERT_EQUALS(result.e()[0], 6.0);
    TS_ASSERT_EQUALS(result.e()[1], 9.0);
  }

  void test_divide() {
    const Histogram hist(BinEdges{1, 2, 3}, Counts{4, 9});
    auto result = hist / 0.5;
    TS_ASSERT_EQUALS(result.x()[0], 1.0);
    TS_ASSERT_EQUALS(result.x()[1], 2.0);
    TS_ASSERT_EQUALS(result.x()[2], 3.0);
    TS_ASSERT_EQUALS(result.y()[0], 8.0);
    TS_ASSERT_EQUALS(result.y()[1], 18.0);
    TS_ASSERT_EQUALS(result.e()[0], 4.0);
    TS_ASSERT_EQUALS(result.e()[1], 6.0);
  }
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMMATHTEST_H_ */
