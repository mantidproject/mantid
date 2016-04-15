#ifndef MANTID_KERNEL_HISTOGRAM_HISTOGRAMDATATEST_H_
#define MANTID_KERNEL_HISTOGRAM_HISTOGRAMDATATEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Histogram/HistogramData.h"
#include "MantidKernel/make_cow.h"

using Mantid::Kernel::HistogramData;
using Mantid::Kernel::make_cow;

struct HistogramDataTester : public HistogramData<HistogramDataTester> {
  using HistogramData<HistogramDataTester>::HistogramData;
};

class HistogramHistogramDataTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramHistogramDataTest *createSuite() {
    return new HistogramHistogramDataTest();
  }
  static void destroySuite(HistogramHistogramDataTest *suite) { delete suite; }

  void test_constructor() {
    TS_ASSERT_THROWS_NOTHING(
        HistogramDataTester data(make_cow<std::vector<double>>(0)));
  }

  void test_constructor_fail() {
    TS_ASSERT_THROWS(HistogramDataTester data(nullptr), std::logic_error);
  }

  void test_operator_bool() {
    HistogramDataTester data(make_cow<std::vector<double>>(0));
    TS_ASSERT(data);
  }

  void test_size() {
    HistogramDataTester data0(make_cow<std::vector<double>>(0));
    TS_ASSERT_EQUALS(data0.size(), 0);
    HistogramDataTester data1(make_cow<std::vector<double>>(1));
    TS_ASSERT_EQUALS(data1.size(), 1);
  }

  void test_const_index_operator() {
    const HistogramDataTester data(
        make_cow<std::vector<double>>(std::vector<double>{0.1, 0.2}));
    const auto copy(data);
    TS_ASSERT_EQUALS(&data[0], &copy[0]);
    TS_ASSERT_EQUALS(data[0], 0.1);
    TS_ASSERT_EQUALS(data[1], 0.2);
  }

  void test_index_operator() {
    HistogramDataTester data(
        make_cow<std::vector<double>>(std::vector<double>{0.1, 0.2}));
    const auto copy(data);
    TS_ASSERT_DIFFERS(&data[0], &copy[0]);
    TS_ASSERT_EQUALS(data[0], 0.1);
    TS_ASSERT_EQUALS(data[1], 0.2);
  }
};

#endif /* MANTID_KERNEL_HISTOGRAM_HISTOGRAMDATATEST_H_ */
