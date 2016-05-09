#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMDXTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMDXTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/HistogramDx.h"

using Mantid::HistogramData::HistogramDx;

class HistogramDxTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramDxTest *createSuite() { return new HistogramDxTest(); }
  static void destroySuite(HistogramDxTest *suite) { delete suite; }

  void test_fake() {}
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMDXTEST_H_ */
