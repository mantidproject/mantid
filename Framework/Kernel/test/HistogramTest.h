#ifndef MANTID_KERNEL_HISTOGRAMTEST_H_
#define MANTID_KERNEL_HISTOGRAMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Histogram/Histogram.h"

using namespace Mantid;
using namespace Kernel;

class HistogramTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramTest *createSuite() { return new HistogramTest(); }
  static void destroySuite(HistogramTest *suite) { delete suite; }

  void test_construction_fail() {
    TS_ASSERT_THROWS(Histogram hist(Histogram::XMode::Uninitialized),
                     std::logic_error);
  }

  void test_construction_Points() {
    TS_ASSERT_THROWS_NOTHING(Histogram hist(Histogram::XMode::Points));
  }

  void test_construction_BinEdges() {
    TS_ASSERT_THROWS_NOTHING(Histogram hist(Histogram::XMode::BinEdges));
  }

  void test_xMode() {
    Histogram hist1(Histogram::XMode::Points);
    TS_ASSERT_EQUALS(hist1.xMode(), Histogram::XMode::Points);
    Histogram hist2(Histogram::XMode::BinEdges);
    TS_ASSERT_EQUALS(hist2.xMode(), Histogram::XMode::BinEdges);
  }

  void test_getHistogramXMode() {
    TS_ASSERT_EQUALS(getHistogramXMode(0, 0), Histogram::XMode::Points);
    TS_ASSERT_EQUALS(getHistogramXMode(1, 1), Histogram::XMode::Points);
    TS_ASSERT_EQUALS(getHistogramXMode(1, 0), Histogram::XMode::BinEdges);
    TS_ASSERT_EQUALS(getHistogramXMode(2, 1), Histogram::XMode::BinEdges);
    TS_ASSERT_EQUALS(getHistogramXMode(2, 0), Histogram::XMode::Uninitialized);
    TS_ASSERT_EQUALS(getHistogramXMode(3, 1), Histogram::XMode::Uninitialized);
    TS_ASSERT_EQUALS(getHistogramXMode(0, 1), Histogram::XMode::Uninitialized);
  }
};

#endif /* MANTID_KERNEL_HISTOGRAMTEST_H_ */
