#ifndef MANTID_HISTOGRAMDATA_COUNTSTANDARDDEVIATIONSTEST_H_
#define MANTID_HISTOGRAMDATA_COUNTSTANDARDDEVIATIONSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/CountStandardDeviations.h"

using Mantid::HistogramData::CountStandardDeviations;

class CountStandardDeviationsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CountStandardDeviationsTest *createSuite() {
    return new CountStandardDeviationsTest();
  }
  static void destroySuite(CountStandardDeviationsTest *suite) { delete suite; }

  void test_construct_default() {
    const CountStandardDeviations counts{};
    TS_ASSERT(!counts);
  }
};

#endif /* MANTID_HISTOGRAMDATA_COUNTSTANDARDDEVIATIONSTEST_H_ */
