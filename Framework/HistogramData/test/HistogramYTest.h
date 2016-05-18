#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMYTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/HistogramY.h"

using Mantid::HistogramData::HistogramY;

class HistogramYTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramYTest *createSuite() { return new HistogramYTest(); }
  static void destroySuite(HistogramYTest *suite) { delete suite; }

  void test_fake() {}
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMYTEST_H_ */
