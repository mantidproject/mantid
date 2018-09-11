#ifndef MANTID_HISTOGRAMDATA_QUADRATICGENERATORTEST_H_
#define MANTID_HISTOGRAMDATA_QUADRATICGENERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/QuadraticGenerator.h"

#include <algorithm>

using Mantid::HistogramData::QuadraticGenerator;

class QuadraticGeneratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QuadraticGeneratorTest *createSuite() {
    return new QuadraticGeneratorTest();
  }
  static void destroySuite(QuadraticGeneratorTest *suite) { delete suite; }

  void test_length0() {
    std::vector<double> x(0);
    std::generate_n(x.begin(), 0, QuadraticGenerator(0.1, 0.2, 0.3));
    TS_ASSERT_EQUALS(x, std::vector<double>(0));
  }

  void test_length1() {
    std::vector<double> x(1);
    std::generate_n(x.begin(), 1, QuadraticGenerator(0.1, 0.2, 0.3));
    TS_ASSERT_EQUALS(x, std::vector<double>{0.1});
  }

  void test_length2() {
    std::vector<double> x(2);
    std::generate_n(x.begin(), 2, QuadraticGenerator(3, 2, 1));
    TS_ASSERT_DELTA(x, std::vector<double>({3, 6}), 1e-14);
  }
};

#endif /* MANTID_HISTOGRAMDATA_QUADRATICGENERATORTEST_H_ */
