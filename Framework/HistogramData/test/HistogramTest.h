#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMTEST_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Histogram.h"

using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::getHistogramXMode;
using Mantid::HistogramData::Points;
using Mantid::HistogramData::BinEdges;

class HistogramTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramTest *createSuite() { return new HistogramTest(); }
  static void destroySuite(HistogramTest *suite) { delete suite; }

  void test_construction_Points() {
    TS_ASSERT_THROWS_NOTHING(Histogram hist(Histogram::XMode::Points));
  }

  void test_construction_BinEdges() {
    TS_ASSERT_THROWS_NOTHING(Histogram hist(Histogram::XMode::BinEdges));
  }

  void test_construct_from_Points() {
    TS_ASSERT_THROWS_NOTHING(Histogram src(Points(0)));
    TS_ASSERT_THROWS_NOTHING(Histogram src(Points{0.1, 0.2, 0.4}));
  }

  void test_construct_from_BinEdges() {
    TS_ASSERT_THROWS_NOTHING(Histogram src(BinEdges(0)));
    TS_ASSERT_THROWS_NOTHING(Histogram src(BinEdges{0.1, 0.2, 0.4}));
  }

  void test_construct_from_invalid_BinEdges() {
    BinEdges binEdges(1);
    TS_ASSERT_THROWS(Histogram{binEdges}, std::logic_error);
  }

  void test_copy_constructor() {
    Histogram src(Points{0.1, 0.2, 0.4});
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
    Histogram src(Points{0.1, 0.2, 0.4});
    Histogram dest(std::move(src));
    TS_ASSERT(!src.points());
    TS_ASSERT(dest.points());
  }

  void test_copy_assignment() {
    Histogram src(Points{0.1, 0.2, 0.4});
    Histogram dest(Histogram::XMode::BinEdges);
    TS_ASSERT_EQUALS(dest.xMode(), Histogram::XMode::BinEdges);
    dest = src;
    TS_ASSERT(src.points());
    TS_ASSERT_EQUALS(dest.xMode(), Histogram::XMode::Points);
    auto points = dest.points();
    TS_ASSERT(points);
    TS_ASSERT_EQUALS(points.size(), 3);
    TS_ASSERT_EQUALS(points[0], 0.1);
    TS_ASSERT_EQUALS(points[1], 0.2);
    TS_ASSERT_EQUALS(points[2], 0.4);
  }

  void test_move_assignment() {
    Histogram src(Points{0.1, 0.2, 0.4});
    Histogram dest(Histogram::XMode::BinEdges);
    TS_ASSERT_EQUALS(dest.xMode(), Histogram::XMode::BinEdges);
    dest = std::move(src);
    TS_ASSERT(!src.points());
    TS_ASSERT(dest.points());
    TS_ASSERT_EQUALS(dest.xMode(), Histogram::XMode::Points);
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
    TS_ASSERT_THROWS(getHistogramXMode(2, 0), std::logic_error);
    TS_ASSERT_THROWS(getHistogramXMode(3, 1), std::logic_error);
    TS_ASSERT_THROWS(getHistogramXMode(0, 1), std::logic_error);
  }

  void test_assignment() {
    const Histogram src(Points(1));
    Histogram dest(Points(1));
    TS_ASSERT_THROWS_NOTHING(dest = src);
    TS_ASSERT_EQUALS(&dest.x()[0], &src.x()[0]);
  }

  void test_assignment_mutating() {
    const Histogram src(Points(1));
    Histogram dest(BinEdges(2));
    TS_ASSERT_THROWS_NOTHING(dest = src);
    TS_ASSERT_EQUALS(&dest.x()[0], &src.x()[0]);
  }

  void test_assignment_size_change() {
    Histogram src1(Points(1));
    Histogram dest1(Points(2));
    TS_ASSERT_THROWS_NOTHING(dest1 = src1);
    Histogram src2(Points(1));
    Histogram dest2(BinEdges(3));
    TS_ASSERT_THROWS_NOTHING(dest2 = src2);
    Histogram src3(BinEdges(2));
    Histogram dest3(Points(2));
    TS_ASSERT_THROWS_NOTHING(dest3 = src3);
    Histogram src4(BinEdges(2));
    Histogram dest4(BinEdges(3));
    TS_ASSERT_THROWS_NOTHING(dest4 = src4);
  }

  void test_points_from_edges() {
    BinEdges binEdges{0.1, 0.2, 0.4};
    const Histogram hist(binEdges);
    const auto points = hist.points();
    TS_ASSERT_DIFFERS(&points[0], &hist.x()[0]);
    TS_ASSERT_EQUALS(points.size(), 2);
    TS_ASSERT_DELTA(points[0], 0.15, 1e-14);
    TS_ASSERT_DELTA(points[1], 0.3, 1e-14);
  }

  void test_points_from_points() {
    const Histogram hist(Points{0.1, 0.2, 0.4});
    const auto points = hist.points();
    TS_ASSERT_EQUALS(&points[0], &hist.x()[0]);
  }

  void test_setPoints_from_vector() {
    Histogram h1(Points(2));
    TS_ASSERT_THROWS_NOTHING(h1.setPoints(std::vector<double>{0.1, 0.2}));
    TS_ASSERT_EQUALS(h1.x().size(), 2);
    TS_ASSERT_EQUALS(h1.x()[0], 0.1);
    TS_ASSERT_EQUALS(h1.x()[1], 0.2);
    Histogram h2(BinEdges(2));
    TS_ASSERT_THROWS_NOTHING(h2.setPoints(std::vector<double>{0.1}));
    TS_ASSERT_EQUALS(h2.x().size(), 1);
    TS_ASSERT_EQUALS(h2.x()[0], 0.1);
  }

  void test_setPoints_from_Points() {
    Histogram h1(Points(2));
    TS_ASSERT_THROWS_NOTHING(h1.setPoints(Points{0.1, 0.2}));
    TS_ASSERT_EQUALS(h1.x().size(), 2);
    TS_ASSERT_EQUALS(h1.x()[0], 0.1);
    TS_ASSERT_EQUALS(h1.x()[1], 0.2);
    Histogram h2(BinEdges(2));
    TS_ASSERT_THROWS_NOTHING(h2.setPoints(Points{0.1}));
    TS_ASSERT_EQUALS(h2.x().size(), 1);
    TS_ASSERT_EQUALS(h2.x()[0], 0.1);
  }

  void test_setPoints_from_BinEdges() {
    Histogram h1(Points(2));
    TS_ASSERT_THROWS_NOTHING(h1.setPoints(BinEdges{0.1, 0.2, 0.4}));
    TS_ASSERT_EQUALS(h1.x().size(), 2);
    TS_ASSERT_DELTA(h1.x()[0], 0.15, 1e-14);
    TS_ASSERT_DELTA(h1.x()[1], 0.3, 1e-14);
    Histogram h2(BinEdges(2));
    TS_ASSERT_THROWS_NOTHING(h2.setPoints(BinEdges{0.1, 0.2}));
    TS_ASSERT_EQUALS(h2.x().size(), 1);
    TS_ASSERT_DELTA(h2.x()[0], 0.15, 1e-14);
  }

  void test_setPoints_degenerate() {
    Histogram h1(Points(0));
    TS_ASSERT_THROWS_NOTHING(h1.setPoints(std::vector<double>(0)));
    TS_ASSERT_EQUALS(h1.x().size(), 0);
    TS_ASSERT_THROWS_NOTHING(h1.setPoints(Points(0)));
    TS_ASSERT_EQUALS(h1.x().size(), 0);
    TS_ASSERT_THROWS_NOTHING(h1.setPoints(BinEdges(0)));
    TS_ASSERT_EQUALS(h1.x().size(), 0);
    Histogram h2(BinEdges(0));
    TS_ASSERT_THROWS_NOTHING(h2.setPoints(std::vector<double>(0)));
    TS_ASSERT_EQUALS(h2.x().size(), 0);
    TS_ASSERT_THROWS_NOTHING(h2.setPoints(Points(0)));
    TS_ASSERT_EQUALS(h2.x().size(), 0);
    TS_ASSERT_THROWS_NOTHING(h2.setPoints(BinEdges(0)));
    TS_ASSERT_EQUALS(h2.x().size(), 0);
  }

  void test_setPoints_size_mismatch() {
    Histogram h1(Points(2));
    TS_ASSERT_THROWS(h1.setPoints(std::vector<double>(1)), std::logic_error);
    TS_ASSERT_THROWS(h1.setPoints(std::vector<double>(3)), std::logic_error);
    TS_ASSERT_THROWS(h1.setPoints(Points(1)), std::logic_error);
    TS_ASSERT_THROWS(h1.setPoints(Points(3)), std::logic_error);
    TS_ASSERT_THROWS(h1.setPoints(BinEdges(2)), std::logic_error);
    TS_ASSERT_THROWS(h1.setPoints(BinEdges(4)), std::logic_error);
    Histogram h2(BinEdges(2));
    TS_ASSERT_THROWS(h2.setPoints(std::vector<double>(0)), std::logic_error);
    TS_ASSERT_THROWS(h2.setPoints(std::vector<double>(2)), std::logic_error);
    TS_ASSERT_THROWS(h2.setPoints(Points(0)), std::logic_error);
    TS_ASSERT_THROWS(h2.setPoints(Points(2)), std::logic_error);
    TS_ASSERT_THROWS(h2.setPoints(BinEdges(1)), std::logic_error);
    TS_ASSERT_THROWS(h2.setPoints(BinEdges(3)), std::logic_error);
  }

  void test_setPoints_size_mismatch_degenerate() {
    Histogram h1(Points(0));
    TS_ASSERT_THROWS(h1.setPoints(std::vector<double>(1)), std::logic_error);
    TS_ASSERT_THROWS(h1.setPoints(Points(1)), std::logic_error);
    TS_ASSERT_THROWS(h1.setPoints(BinEdges(1)), std::logic_error);
    Histogram h2(BinEdges(0));
    TS_ASSERT_THROWS(h2.setPoints(std::vector<double>(1)), std::logic_error);
    TS_ASSERT_THROWS(h2.setPoints(Points(1)), std::logic_error);
    TS_ASSERT_THROWS(h2.setPoints(BinEdges(1)), std::logic_error);
  }

  void test_setPoints_self_assignment() {
    Histogram h(Points(0));
    auto &x = h.x();
    auto old_address = &x;
    h.setPoints(x);
    TS_ASSERT_EQUALS(&h.x(), old_address);
  }

  void test_setPoints_legacy_self_assignment() {
    Histogram h(Points(0));
    auto &x = h.readX();
    auto old_address = &x;
    h.setPoints(x);
    TS_ASSERT_EQUALS(&h.readX(), old_address);
  }

  void test_setPoints_self_assignment_with_size_mismatch() {
    Histogram h(BinEdges(2));
    auto &x = h.x();
    // This test makes sure that size mismatch takes precedence over the
    // self-assignment check. x is bin edges, setting it as points should fail.
    TS_ASSERT_THROWS(h.setPoints(x), std::logic_error);
  }

  void test_setPoints_changesDxStorageMode() {
    Histogram hist(BinEdges(3));
    auto dx = {1.0, 2.0, 3.0};
    hist.setBinEdgeStandardDeviations(dx);
    TS_ASSERT_EQUALS(hist.dx().size(), 3);
    hist.setPoints(Points(2));
    TS_ASSERT_EQUALS(hist.dx().size(), 2);
    TS_ASSERT_DELTA(hist.dx()[0], 1.5, 1e-14);
    TS_ASSERT_DELTA(hist.dx()[1], 2.5, 1e-14);
  }

  void test_edges_from_edges() {
    const Histogram hist(BinEdges{0.1, 0.2, 0.4});
    const auto edges = hist.binEdges();
    TS_ASSERT_EQUALS(&edges[0], &hist.x()[0]);
    TS_ASSERT_EQUALS(edges.size(), 3);
  }

  void test_edges_from_points() {
    const Histogram hist(Points{0.1, 0.2, 0.4});
    const auto edges = hist.binEdges();
    TS_ASSERT_DIFFERS(&edges[0], &hist.x()[0]);
    TS_ASSERT_EQUALS(edges.size(), 4);
  }

  void test_setBinEdges() {
    Histogram h1(Points(2));
    TS_ASSERT_THROWS_NOTHING(h1.setBinEdges(std::vector<double>(3)));
    TS_ASSERT_THROWS_NOTHING(h1.setBinEdges(Points(2)));
    TS_ASSERT_THROWS_NOTHING(h1.setBinEdges(BinEdges(3)));
    Histogram h2(BinEdges(2));
    TS_ASSERT_THROWS_NOTHING(h2.setBinEdges(std::vector<double>(2)));
    TS_ASSERT_THROWS_NOTHING(h2.setBinEdges(Points(1)));
    TS_ASSERT_THROWS_NOTHING(h2.setBinEdges(BinEdges(2)));
  }

  void test_setBinEdges_from_vector() {
    Histogram h1(Points(2));
    TS_ASSERT_THROWS_NOTHING(
        h1.setBinEdges(std::vector<double>{0.1, 0.2, 0.4}));
    TS_ASSERT_EQUALS(h1.x().size(), 3);
    TS_ASSERT_EQUALS(h1.x()[0], 0.1);
    TS_ASSERT_EQUALS(h1.x()[1], 0.2);
    TS_ASSERT_EQUALS(h1.x()[2], 0.4);
    Histogram h2(BinEdges(2));
    TS_ASSERT_THROWS_NOTHING(h2.setBinEdges(std::vector<double>{0.1, 0.2}));
    TS_ASSERT_EQUALS(h2.x().size(), 2);
    TS_ASSERT_EQUALS(h2.x()[0], 0.1);
    TS_ASSERT_EQUALS(h2.x()[1], 0.2);
  }

  void test_setBinEdges_from_Points() {
    Histogram h1(Points(2));
    TS_ASSERT_THROWS_NOTHING(h1.setBinEdges(Points{0.1, 0.3}));
    TS_ASSERT_EQUALS(h1.x().size(), 3);
    TS_ASSERT_DELTA(h1.x()[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(h1.x()[1], 0.2, 1e-14);
    TS_ASSERT_DELTA(h1.x()[2], 0.4, 1e-14);
    Histogram h2(BinEdges(2));
    TS_ASSERT_THROWS_NOTHING(h2.setBinEdges(Points{1.0}));
    TS_ASSERT_EQUALS(h2.x().size(), 2);
    TS_ASSERT_DELTA(h2.x()[0], 0.5, 1e-14);
    TS_ASSERT_DELTA(h2.x()[1], 1.5, 1e-14);
  }

  void test_setBinEdges_from_BinEdges() {
    Histogram h1(Points(2));
    TS_ASSERT_THROWS_NOTHING(h1.setBinEdges(BinEdges{0.1, 0.2, 0.4}));
    TS_ASSERT_EQUALS(h1.x().size(), 3);
    TS_ASSERT_EQUALS(h1.x()[0], 0.1);
    TS_ASSERT_EQUALS(h1.x()[1], 0.2);
    TS_ASSERT_EQUALS(h1.x()[2], 0.4);
    Histogram h2(BinEdges(2));
    TS_ASSERT_THROWS_NOTHING(h2.setBinEdges(BinEdges{0.1, 0.2}));
    TS_ASSERT_EQUALS(h2.x().size(), 2);
    TS_ASSERT_EQUALS(h2.x()[0], 0.1);
    TS_ASSERT_EQUALS(h2.x()[1], 0.2);
  }

  void test_setBinEdges_degenerate() {
    Histogram h1(Points(0));
    TS_ASSERT_THROWS_NOTHING(h1.setBinEdges(std::vector<double>(0)));
    TS_ASSERT_EQUALS(h1.x().size(), 0);
    TS_ASSERT_THROWS_NOTHING(h1.setBinEdges(Points(0)));
    TS_ASSERT_EQUALS(h1.x().size(), 0);
    TS_ASSERT_THROWS_NOTHING(h1.setBinEdges(BinEdges(0)));
    TS_ASSERT_EQUALS(h1.x().size(), 0);
    Histogram h2(BinEdges(0));
    TS_ASSERT_THROWS_NOTHING(h2.setBinEdges(std::vector<double>(0)));
    TS_ASSERT_EQUALS(h2.x().size(), 0);
    TS_ASSERT_THROWS_NOTHING(h2.setBinEdges(Points(0)));
    TS_ASSERT_EQUALS(h2.x().size(), 0);
    TS_ASSERT_THROWS_NOTHING(h2.setBinEdges(BinEdges(0)));
    TS_ASSERT_EQUALS(h2.x().size(), 0);
  }

  void test_setBinEdges_size_mismatch() {
    Histogram h1(Points(2));
    TS_ASSERT_THROWS(h1.setBinEdges(std::vector<double>(2)), std::logic_error);
    TS_ASSERT_THROWS(h1.setBinEdges(std::vector<double>(4)), std::logic_error);
    TS_ASSERT_THROWS(h1.setBinEdges(Points(1)), std::logic_error);
    TS_ASSERT_THROWS(h1.setBinEdges(Points(3)), std::logic_error);
    TS_ASSERT_THROWS(h1.setBinEdges(BinEdges(2)), std::logic_error);
    TS_ASSERT_THROWS(h1.setBinEdges(BinEdges(4)), std::logic_error);
    Histogram h2(BinEdges(2));
    TS_ASSERT_THROWS(h2.setBinEdges(std::vector<double>(1)), std::logic_error);
    TS_ASSERT_THROWS(h2.setBinEdges(std::vector<double>(3)), std::logic_error);
    TS_ASSERT_THROWS(h2.setBinEdges(Points(0)), std::logic_error);
    TS_ASSERT_THROWS(h2.setBinEdges(Points(2)), std::logic_error);
    TS_ASSERT_THROWS(h2.setBinEdges(BinEdges(1)), std::logic_error);
    TS_ASSERT_THROWS(h2.setBinEdges(BinEdges(3)), std::logic_error);
  }

  void test_setBinEdges_size_mismatch_degenerate() {
    Histogram h1(Points(0));
    TS_ASSERT_THROWS(h1.setBinEdges(std::vector<double>(1)), std::logic_error);
    TS_ASSERT_THROWS(h1.setBinEdges(Points(1)), std::logic_error);
    TS_ASSERT_THROWS(h1.setBinEdges(BinEdges(1)), std::logic_error);
    Histogram h2(BinEdges(0));
    TS_ASSERT_THROWS(h2.setBinEdges(std::vector<double>(1)), std::logic_error);
    TS_ASSERT_THROWS(h2.setBinEdges(Points(1)), std::logic_error);
    TS_ASSERT_THROWS(h2.setBinEdges(BinEdges(1)), std::logic_error);
  }

  void test_setBinEdges_self_assignment() {
    Histogram h(BinEdges(0));
    auto &x = h.x();
    auto old_address = &x;
    h.setBinEdges(x);
    TS_ASSERT_EQUALS(&h.x(), old_address);
  }

  void test_setBinEdges_legacy_self_assignment() {
    Histogram h(BinEdges(0));
    auto &x = h.readX();
    auto old_address = &x;
    h.setBinEdges(x);
    TS_ASSERT_EQUALS(&h.readX(), old_address);
  }

  void test_setBinEdges_self_assignment_with_size_mismatch() {
    Histogram h(Points(2));
    auto &x = h.x();
    // This test makes sure that size mismatch takes precedence over the
    // self-assignment check. x is points, setting it as bin edges should fail.
    TS_ASSERT_THROWS(h.setBinEdges(x), std::logic_error);
  }

  void test_setBinEdges_changesDxStorageMode() {
    Histogram hist(Points(2));
    auto dx = {1.0, 2.0};
    hist.setPointStandardDeviations(dx);
    TS_ASSERT_EQUALS(hist.dx().size(), 2);
    hist.setBinEdges(BinEdges(3));
    TS_ASSERT_EQUALS(hist.dx().size(), 3);
    TS_ASSERT_DELTA(hist.dx()[0], 0.5, 1e-14);
    TS_ASSERT_DELTA(hist.dx()[1], 1.5, 1e-14);
    TS_ASSERT_DELTA(hist.dx()[2], 2.5, 1e-14);
  }

  void test_x() {
    Histogram hist(Points({0.1, 0.2, 0.4}));
    TS_ASSERT_EQUALS(hist.x()[0], 0.1);
    TS_ASSERT_EQUALS(hist.x()[1], 0.2);
    TS_ASSERT_EQUALS(hist.x()[2], 0.4);
  }

  void test_x_references_internal_data() {
    Histogram hist(Points(0));
    auto copy(hist);
    TS_ASSERT_EQUALS(&hist.x(), &copy.x());
  }

  void test_mutableX() {
    Histogram hist(Points({0.1, 0.2, 0.4}));
    TS_ASSERT_EQUALS(hist.mutableX()[0], 0.1);
    TS_ASSERT_EQUALS(hist.mutableX()[1], 0.2);
    TS_ASSERT_EQUALS(hist.mutableX()[2], 0.4);
  }

  void test_mutableX_triggers_copy() {
    Histogram hist(Points(0));
    auto copy(hist);
    TS_ASSERT_DIFFERS(&hist.x(), &copy.mutableX());
  }

  void test_x_references_same_data_as_binEdges() {
    Histogram hist(BinEdges(0));
    TS_ASSERT_EQUALS(&hist.x(), &hist.binEdges().data());
    TS_ASSERT_DIFFERS(&hist.x(), &hist.points().data());
  }

  void test_x_references_same_data_as_points() {
    Histogram hist(Points(0));
    TS_ASSERT_DIFFERS(&hist.x(), &hist.binEdges().data());
    TS_ASSERT_EQUALS(&hist.x(), &hist.points().data());
  }

  void test_sharedX() {
    auto data = Mantid::Kernel::make_cow<HistogramX>(0);
    Histogram hist{BinEdges(data)};
    TS_ASSERT_EQUALS(hist.sharedX(), data);
  }

  void test_setSharedX() {
    auto data1 = Mantid::Kernel::make_cow<HistogramX>(0);
    auto data2 = Mantid::Kernel::make_cow<HistogramX>(0);
    Histogram hist{BinEdges(data1)};
    TS_ASSERT_EQUALS(hist.sharedX(), data1);
    TS_ASSERT_THROWS_NOTHING(hist.setSharedX(data2));
    TS_ASSERT_DIFFERS(hist.sharedX(), data1);
    TS_ASSERT_EQUALS(hist.sharedX(), data2);
  }

  void test_setSharedX_size_mismatch() {
    auto data1 = Mantid::Kernel::make_cow<HistogramX>(0);
    auto data2 = Mantid::Kernel::make_cow<HistogramX>(2);
    Histogram hist{BinEdges(data1)};
    TS_ASSERT_THROWS(hist.setSharedX(data2), std::logic_error);
  }

  void test_setSharedX_catches_misuse() {
    BinEdges edges(2);
    Histogram hist(edges);
    auto points = hist.points();
    TS_ASSERT_THROWS(hist.setSharedX(points.cowData()), std::logic_error);
  }
};

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMTEST_H_ */
