#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMITERATORTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMITERATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramItem.h"
#include "MantidHistogramData/HistogramIterator.h"

using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::HistogramItem;
using Mantid::HistogramData::HistogramIterator;
using Mantid::HistogramData::Points;
using Mantid::HistogramData::Counts;

class HistogramIteratorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramIteratorTest *createSuite() {
    return new HistogramIteratorTest();
  }
  static void destroySuite(HistogramIteratorTest *suite) { delete suite; }

  void test_construction() {
    Histogram hist(Histogram::XMode::BinEdges, Histogram::YMode::Counts);
    TS_ASSERT_THROWS_NOTHING(HistogramIterator iter(hist));
  }
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMITERATORTEST_H_ */
