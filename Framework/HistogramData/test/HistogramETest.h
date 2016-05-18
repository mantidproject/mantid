#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMETEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/HistogramE.h"

using Mantid::HistogramData::HistogramE;

class HistogramETest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramETest *createSuite() { return new HistogramETest(); }
  static void destroySuite(HistogramETest *suite) { delete suite; }

  void test_fake() {}
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMETEST_H_ */
