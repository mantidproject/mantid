#ifndef MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTORTEST_H_
#define MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidKernel/make_cow.h"

using Mantid::HistogramData::detail::FixedLengthVector;
using Mantid::Kernel::make_cow;

struct FixedLengthVectorTester
    : public FixedLengthVector<FixedLengthVectorTester> {
  using FixedLengthVector<FixedLengthVectorTester>::FixedLengthVector;
};

class FixedLengthVectorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FixedLengthVectorTest *createSuite() {
    return new FixedLengthVectorTest();
  }
  static void destroySuite(FixedLengthVectorTest *suite) { delete suite; }

  void test_constructor() {
    TS_ASSERT_THROWS_NOTHING(
        FixedLengthVectorTester data(make_cow<std::vector<double>>(0)));
  }

  void test_constructor_fail() {
    TS_ASSERT_THROWS(FixedLengthVectorTester data(nullptr), std::logic_error);
  }

  void test_operator_bool() {
    FixedLengthVectorTester data(make_cow<std::vector<double>>(0));
    TS_ASSERT(data);
  }

  void test_size() {
    FixedLengthVectorTester data0(make_cow<std::vector<double>>(0));
    TS_ASSERT_EQUALS(data0.size(), 0);
    FixedLengthVectorTester data1(make_cow<std::vector<double>>(1));
    TS_ASSERT_EQUALS(data1.size(), 1);
  }

  void test_const_index_operator() {
    const FixedLengthVectorTester data(
        make_cow<std::vector<double>>(std::vector<double>{0.1, 0.2}));
    const auto copy(data);
    TS_ASSERT_EQUALS(&data[0], &copy[0]);
    TS_ASSERT_EQUALS(data[0], 0.1);
    TS_ASSERT_EQUALS(data[1], 0.2);
  }

  void test_index_operator() {
    FixedLengthVectorTester data(
        make_cow<std::vector<double>>(std::vector<double>{0.1, 0.2}));
    const auto copy(data);
    TS_ASSERT_DIFFERS(&data[0], &copy[0]);
    TS_ASSERT_EQUALS(data[0], 0.1);
    TS_ASSERT_EQUALS(data[1], 0.2);
  }
};

#endif /* MANTID_HISTOGRAMDATA_FIXEDLENGTHVECTORTEST_H_ */
