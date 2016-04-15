#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Histogram.h"

using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::getHistogramXMode;
using Mantid::HistogramData::Points;
using Mantid::HistogramData::BinEdges;

class HistogramTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramTest *createSuite() { return new HistogramTest(); }
  static void destroySuite(HistogramTest *suite) { delete suite; }

  void test_construction_fail() {
    TS_ASSERT_THROWS(Histogram hist(Histogram::XMode::Uninitialized),
                     std::logic_error);
  }

  void test_construction_Points() {
    TS_ASSERT_THROWS_NOTHING(Histogram hist(Histogram::XMode::Points));
  }

  void test_construction_BinEdges() {
    TS_ASSERT_THROWS_NOTHING(Histogram hist(Histogram::XMode::BinEdges));
  }

  void test_copy_constructor() {
    Histogram src(Histogram::XMode::Points);
    src.setPoints(Points{0.1, 0.2, 0.4});
    Histogram dest(src);
    TS_ASSERT(src.points());
    auto points = dest.points();
    TS_ASSERT(points);
    TS_ASSERT_EQUALS(points.size(), 3);
    TS_ASSERT_EQUALS(points[0], 0.1);
    TS_ASSERT_EQUALS(points[1], 0.2);
    TS_ASSERT_EQUALS(points[2], 0.4);
  }

  void test_move_constructor() {
    Histogram src(Histogram::XMode::Points);
    src.setPoints(Points{0.1, 0.2, 0.4});
    Histogram dest(std::move(src));
    TS_ASSERT(!src.points());
    TS_ASSERT(dest.points());
  }

  void test_copy_assignment() {
    Histogram src(Histogram::XMode::Points);
    src.setPoints(Points{0.1, 0.2, 0.4});
    Histogram dest(Histogram::XMode::Points);
    dest = src;
    TS_ASSERT(src.points());
    auto points = dest.points();
    TS_ASSERT(points);
    TS_ASSERT_EQUALS(points.size(), 3);
    TS_ASSERT_EQUALS(points[0], 0.1);
    TS_ASSERT_EQUALS(points[1], 0.2);
    TS_ASSERT_EQUALS(points[2], 0.4);
  }

  void test_move_assignment() {
    Histogram src(Histogram::XMode::Points);
    src.setPoints(Points{0.1, 0.2, 0.4});
    Histogram dest(Histogram::XMode::Points);
    dest = std::move(src);
    TS_ASSERT(!src.points());
    TS_ASSERT(dest.points());
  }

  void test_xMode() {
    Histogram hist1(Histogram::XMode::Points);
    TS_ASSERT_EQUALS(hist1.xMode(), Histogram::XMode::Points);
    Histogram hist2(Histogram::XMode::BinEdges);
    TS_ASSERT_EQUALS(hist2.xMode(), Histogram::XMode::BinEdges);
  }

  void test_getHistogramXMode() {
    TS_ASSERT_EQUALS(getHistogramXMode(0, 0), Histogram::XMode::Points);
    TS_ASSERT_EQUALS(getHistogramXMode(1, 1), Histogram::XMode::Points);
    TS_ASSERT_EQUALS(getHistogramXMode(1, 0), Histogram::XMode::BinEdges);
    TS_ASSERT_EQUALS(getHistogramXMode(2, 1), Histogram::XMode::BinEdges);
    TS_ASSERT_EQUALS(getHistogramXMode(2, 0), Histogram::XMode::Uninitialized);
    TS_ASSERT_EQUALS(getHistogramXMode(3, 1), Histogram::XMode::Uninitialized);
    TS_ASSERT_EQUALS(getHistogramXMode(0, 1), Histogram::XMode::Uninitialized);
  }

  void test_setPoints_fromPoints() {
    Histogram hist(Histogram::XMode::Points);
    hist.setPoints(Points{0.1, 0.2, 0.4});
    TS_ASSERT_EQUALS(hist.xMode(), Histogram::XMode::Points);
    auto points = hist.points();
    TS_ASSERT_EQUALS(points[0], 0.1);
    TS_ASSERT_EQUALS(points[1], 0.2);
    TS_ASSERT_EQUALS(points[2], 0.4);
  }

  void test_setPoints_fromBinEdges() {
    Histogram hist(Histogram::XMode::Points);
    hist.setPoints(BinEdges{0.1, 0.2, 0.4});
    TS_ASSERT_EQUALS(hist.xMode(), Histogram::XMode::Points);
    auto points = hist.points();
    TS_ASSERT_DELTA(points[0], 0.15, 1e-14);
    TS_ASSERT_DELTA(points[1], 0.3, 1e-14);
  }

  void test_setPoints_fromVector() {
    Histogram hist(Histogram::XMode::Points);
    hist.setPoints(std::vector<double>{0.1, 0.2, 0.4});
    TS_ASSERT_EQUALS(hist.xMode(), Histogram::XMode::Points);
    auto points = hist.points();
    TS_ASSERT_EQUALS(points[0], 0.1);
    TS_ASSERT_EQUALS(points[1], 0.2);
    TS_ASSERT_EQUALS(points[2], 0.4);
  }

  void test_setPoints_moveFromPoints() {
    Histogram hist(Histogram::XMode::Points);
    Points src{0.1, 0.2, 0.4};
    hist.setPoints(std::move(src));
    TS_ASSERT(!src);
    TS_ASSERT_EQUALS(hist.xMode(), Histogram::XMode::Points);
    auto points = hist.points();
    TS_ASSERT_EQUALS(points[0], 0.1);
    TS_ASSERT_EQUALS(points[1], 0.2);
    TS_ASSERT_EQUALS(points[2], 0.4);
  }

  void test_setPoints_moveFromBinEdges() {
    Histogram hist(Histogram::XMode::Points);
    BinEdges src{0.1, 0.2, 0.4};
    hist.setPoints(std::move(src));
    // Creating Points from BinEdges: currently there is no move constructor
    // implemented, so src should be untouched.
    TS_ASSERT(src);
    TS_ASSERT_EQUALS(hist.xMode(), Histogram::XMode::Points);
    auto points = hist.points();
    TS_ASSERT_DELTA(points[0], 0.15, 1e-14);
    TS_ASSERT_DELTA(points[1], 0.3, 1e-14);
  }

  void test_setPoints_changes_xMode() {
    Histogram hist(Histogram::XMode::BinEdges);
    hist.setPoints(Points{0.1});
    TS_ASSERT_EQUALS(hist.xMode(), Histogram::XMode::Points);
  }
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMTEST_H_ */
