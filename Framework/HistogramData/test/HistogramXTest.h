#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMXTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/HistogramX.h"

using Mantid::HistogramData::HistogramX;

class HistogramXTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramXTest *createSuite() { return new HistogramXTest(); }
  static void destroySuite(HistogramXTest *suite) { delete suite; }

  void test_fake() {}
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMXTEST_H_ */
