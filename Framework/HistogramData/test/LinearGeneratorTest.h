#ifndef MANTID_HISTOGRAMDATA_LINEARGENERATORTEST_H_
#define MANTID_HISTOGRAMDATA_LINEARGENERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/LinearGenerator.h"

#include <algorithm>

using Mantid::HistogramData::LinearGenerator;

class LinearGeneratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LinearGeneratorTest *createSuite() {
    return new LinearGeneratorTest();
  }
  static void destroySuite(LinearGeneratorTest *suite) { delete suite; }

  void test_length0() {
    std::vector<double> x(0);
    std::generate_n(x.begin(), 0, LinearGenerator(0.1, 0.2));
    TS_ASSERT_EQUALS(x, std::vector<double>(0));
  }

  void test_length1() {
    std::vector<double> x(1);
    std::generate_n(x.begin(), 1, LinearGenerator(0.1, 0.2));
    TS_ASSERT_EQUALS(x, std::vector<double>{0.1});
  }

  void test_length2() {
    std::vector<double> x(2);
    std::generate_n(x.begin(), 2, LinearGenerator(0.1, 0.2));
    TS_ASSERT_DELTA(x, std::vector<double>({0.1, 0.3}), 1e-14);
  }
};

#endif /* MANTID_HISTOGRAMDATA_LINEARGENERATORTEST_H_ */
