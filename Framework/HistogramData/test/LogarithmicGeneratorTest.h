#ifndef MANTID_HISTOGRAMDATA_LOGARITHMICGENERATORTEST_H_
#define MANTID_HISTOGRAMDATA_LOGARITHMICGENERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/LogarithmicGenerator.h"

#include <algorithm>

using Mantid::HistogramData::LogarithmicGenerator;

class LogarithmicGeneratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LogarithmicGeneratorTest *createSuite() {
    return new LogarithmicGeneratorTest();
  }
  static void destroySuite(LogarithmicGeneratorTest *suite) { delete suite; }

  void test_length0() {
    std::vector<double> x(0);
    std::generate_n(x.begin(), 0, LogarithmicGenerator(0.1, 2.0));
    TS_ASSERT_EQUALS(x, std::vector<double>(0));
  }

  void test_length1() {
    std::vector<double> x(1);
    std::generate_n(x.begin(), 1, LogarithmicGenerator(0.1, 2.0));
    TS_ASSERT_EQUALS(x, std::vector<double>{0.1});
  }

  void test_length2() {
    std::vector<double> x(2);
    std::generate_n(x.begin(), 2, LogarithmicGenerator(0.1, 2.0));
    TS_ASSERT_DELTA(x, std::vector<double>({0.1, 0.3}), 1e-14);
  }

  void test_length4() {
    std::vector<double> x(4);
    std::generate_n(x.begin(), 4, LogarithmicGenerator(0.1, 1.0));
    TS_ASSERT_DELTA(x, std::vector<double>({0.1, 0.2, 0.4, 0.8}), 1e-14);
    std::generate_n(x.begin(), 4, LogarithmicGenerator(0.1, 2.0));
    TS_ASSERT_DELTA(x, std::vector<double>({0.1, 0.3, 0.9, 2.7}), 1e-14);
  }
};

#endif /* MANTID_HISTOGRAMDATA_LOGARITHMICGENERATORTEST_H_ */
