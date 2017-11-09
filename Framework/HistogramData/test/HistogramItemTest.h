#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMITEMTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMITEMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramItem.h"

using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::HistogramItem;
using Mantid::HistogramData::Points;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Frequencies;
using Mantid::HistogramData::BinEdges;

class HistogramItemTest : public CxxTest::TestSuite {
public:
  friend class HistogramItem;
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramItemTest *createSuite() { return new HistogramItemTest(); }
  static void destroySuite(HistogramItemTest *suite) { delete suite; }

  static constexpr double tolerance = 1e-6;

  void test_construction() {
  }

};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMITEMTEST_H_ */
