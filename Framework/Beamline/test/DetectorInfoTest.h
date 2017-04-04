#ifndef MANTID_BEAMLINE_DETECTORINFOTEST_H_
#define MANTID_BEAMLINE_DETECTORINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/DetectorInfo.h"
#include "MantidKernel/make_unique.h"

using namespace Mantid;
using Beamline::DetectorInfo;
using PosVec = std::vector<Eigen::Vector3d>;
using RotVec = std::vector<Eigen::Quaterniond>;

class DetectorInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorInfoTest *createSuite() { return new DetectorInfoTest(); }
  static void destroySuite(DetectorInfoTest *suite) { delete suite; }

  void test_constructor() {
    std::unique_ptr<DetectorInfo> detInfo;
    TS_ASSERT_THROWS_NOTHING(detInfo = Kernel::make_unique<DetectorInfo>());
    TS_ASSERT_EQUALS(detInfo->size(), 0);
    TS_ASSERT(!detInfo->isScanning());
    TS_ASSERT_THROWS_NOTHING(
        detInfo = Kernel::make_unique<DetectorInfo>(PosVec(1), RotVec(1)));
    TS_ASSERT_EQUALS(detInfo->size(), 1);
    TS_ASSERT(!detInfo->isScanning());
  }

  void test_constructor_with_monitors() {
    std::unique_ptr<DetectorInfo> info;
    std::vector<size_t> mons{0, 2};
    TS_ASSERT_THROWS_NOTHING(
        info = Kernel::make_unique<DetectorInfo>(PosVec(3), RotVec(3), mons));
    TS_ASSERT_EQUALS(info->size(), 3);
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(PosVec(3), RotVec(3), {}));
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(PosVec(3), RotVec(3), {0}));
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(PosVec(3), RotVec(3), {0, 1, 2}));
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(PosVec(3), RotVec(3), {0, 0, 0}));
    TS_ASSERT_THROWS(DetectorInfo(PosVec(3), RotVec(3), {3}),
                     std::out_of_range);
  }

  void test_constructor_length_mismatch() {
    TS_ASSERT_THROWS(DetectorInfo(PosVec(3), RotVec(2)), std::runtime_error);
  }

  void test_comparison_length() {
    const DetectorInfo length0{};
    const DetectorInfo length1(PosVec(1), RotVec(1));
    TS_ASSERT(length0.isEquivalent(length0));
    TS_ASSERT(length1.isEquivalent(length1));
    TS_ASSERT(!length0.isEquivalent(length1));
  }

  void test_comparison_isMonitor() {
    const DetectorInfo a(PosVec(1), RotVec(1));
    const DetectorInfo b(PosVec(1), RotVec(1), {0});
    TS_ASSERT(!a.isEquivalent(b));
  }

  void test_comparison_isMasked() {
    DetectorInfo a(PosVec(1), RotVec(1));
    const auto b(a);
    a.setMasked(0, true);
    TS_ASSERT(!a.isEquivalent(b));
    a.setMasked(0, false);
    TS_ASSERT(a.isEquivalent(b));
  }

  void test_comparison_position() {
    DetectorInfo a(PosVec(2, {0, 0, 0}), RotVec(2));
    DetectorInfo b(a);
    a.setPosition(1, {1, 2, 3});
    TS_ASSERT(!a.isEquivalent(b));
    b.setPosition(1, a.position(1));
    TS_ASSERT(a.isEquivalent(b));
  }

  void test_comparison_zero_position() {
    DetectorInfo a(PosVec(1), RotVec(1));
    DetectorInfo b(a);
    a.setPosition(0, {0, 0, 0});
    b.setPosition(0, {0, 0, 1e-10});
    TS_ASSERT(a.isEquivalent(b));
  }

  void test_comparison_minimum_position() {
    DetectorInfo a(PosVec(1), RotVec(1));
    DetectorInfo b(a);
    a.setPosition(0, {1000, 0, 0});
    b.setPosition(0, {1000, 0, 1e-9});
    TS_ASSERT(!a.isEquivalent(b));
    b.setPosition(0, {1000, 0, 1e-10});
    TS_ASSERT(a.isEquivalent(b));
  }

  void test_comparison_rotation() {
    DetectorInfo a(
        PosVec(2),
        RotVec(2, Eigen::Quaterniond(Eigen::AngleAxisd(
                      30.0, Eigen::Vector3d{1, 2, 3}.normalized()))));
    DetectorInfo b(a);
    a.setRotation(1, {1, 2, 3, 4});
    TS_ASSERT(!a.isEquivalent(b));
    b.setRotation(1, a.rotation(1));
    TS_ASSERT(a.isEquivalent(b));
  }

  void test_comparison_minimum_rotation() {
    DetectorInfo a(PosVec(1), RotVec(1, Eigen::Quaterniond::Identity()));
    DetectorInfo b(a);

    // Change of 1 um at distance 1000 m is caught.
    Eigen::Quaterniond qmin;
    qmin.setFromTwoVectors(Eigen::Vector3d({1000, 0, 0}),
                           Eigen::Vector3d({1000, 1e-9, 0}));
    a.setRotation(0, qmin);
    TS_ASSERT(!a.isEquivalent(b));

    // Change of 0.1 um at distance 1000 m is allowed.
    Eigen::Quaterniond qepsilon;
    qepsilon.setFromTwoVectors(Eigen::Vector3d({1000, 0, 0}),
                               Eigen::Vector3d({1000, 1e-10, 0}));
    a.setRotation(0, qepsilon);
    TS_ASSERT(a.isEquivalent(b));
  }

  void test_copy() {
    const DetectorInfo source(PosVec(7), RotVec(7));
    const auto copy(source);
    TS_ASSERT_EQUALS(copy.size(), 7);
  }

  void test_move() {
    DetectorInfo source(PosVec(7), RotVec(7));
    const auto moved(std::move(source));
    TS_ASSERT_EQUALS(moved.size(), 7);
    TS_ASSERT_EQUALS(source.size(), 0);
  }

  void test_assign() {
    const DetectorInfo source(PosVec(7), RotVec(7));
    DetectorInfo assignee(PosVec(1), RotVec(1));
    assignee = source;
    TS_ASSERT_EQUALS(assignee.size(), 7);
  }

  void test_move_assign() {
    DetectorInfo source(PosVec(7), RotVec(7));
    DetectorInfo assignee(PosVec(1), RotVec(1));
    assignee = std::move(source);
    TS_ASSERT_EQUALS(assignee.size(), 7);
    TS_ASSERT_EQUALS(source.size(), 0);
  }

  void test_no_monitors() {
    DetectorInfo info(PosVec(3), RotVec(3));
    TS_ASSERT(!info.isMonitor(0));
    TS_ASSERT(!info.isMonitor(1));
    TS_ASSERT(!info.isMonitor(2));
  }

  void test_monitors() {
    std::vector<size_t> monitors{0, 2};
    DetectorInfo info(PosVec(3), RotVec(3), monitors);
    TS_ASSERT(info.isMonitor(0));
    TS_ASSERT(!info.isMonitor(1));
    TS_ASSERT(info.isMonitor(2));
  }

  void test_duplicate_monitors_ignored() {
    std::vector<size_t> monitors{0, 0, 2, 2};
    DetectorInfo info(PosVec(3), RotVec(3), monitors);
    TS_ASSERT(info.isMonitor(0));
    TS_ASSERT(!info.isMonitor(1));
    TS_ASSERT(info.isMonitor(2));
  }

  void test_masking() {
    DetectorInfo info(PosVec(3), RotVec(3));
    TS_ASSERT(!info.isMasked(0));
    TS_ASSERT(!info.isMasked(1));
    TS_ASSERT(!info.isMasked(2));
    info.setMasked(1, true);
    TS_ASSERT(!info.isMasked(0));
    TS_ASSERT(info.isMasked(1));
    TS_ASSERT(!info.isMasked(2));
    info.setMasked(1, false);
    TS_ASSERT(!info.isMasked(0));
    TS_ASSERT(!info.isMasked(1));
    TS_ASSERT(!info.isMasked(2));
  }

  void test_masking_copy() {
    DetectorInfo source(PosVec(1), RotVec(1));
    source.setMasked(0, true);
    DetectorInfo copy(source);
    TS_ASSERT(copy.isMasked(0));
    source.setMasked(0, false);
    TS_ASSERT(!source.isMasked(0));
    TS_ASSERT(copy.isMasked(0));
  }

  void test_constructors_set_positions_correctly() {
    Eigen::Vector3d pos0{1, 2, 3};
    Eigen::Vector3d pos1{2, 3, 4};
    PosVec positions{pos0, pos1};
    const DetectorInfo info(positions, RotVec(2));
    TS_ASSERT_EQUALS(info.position(0), pos0);
    TS_ASSERT_EQUALS(info.position(1), pos1);
    const DetectorInfo info_with_monitors(positions, RotVec(2), {1});
    TS_ASSERT_EQUALS(info_with_monitors.position(0), pos0);
    TS_ASSERT_EQUALS(info_with_monitors.position(1), pos1);
  }

  void test_constructors_set_rotations_correctly() {
    Eigen::Quaterniond rot0{1, 2, 3, 4};
    Eigen::Quaterniond rot1{2, 3, 4, 5};
    RotVec rotations{rot0, rot1};
    const DetectorInfo info(PosVec(2), rotations);
    TS_ASSERT_EQUALS(info.rotation(0).coeffs(), rot0.coeffs());
    TS_ASSERT_EQUALS(info.rotation(1).coeffs(), rot1.coeffs());
    const DetectorInfo info_with_monitors(PosVec(2), rotations);
    TS_ASSERT_EQUALS(info_with_monitors.rotation(0).coeffs(), rot0.coeffs());
    TS_ASSERT_EQUALS(info_with_monitors.rotation(1).coeffs(), rot1.coeffs());
  }

  void test_position_rotation_copy() {
    DetectorInfo source(PosVec(7), RotVec(7));
    source.setPosition(0, {1, 2, 3});
    source.setRotation(0, Eigen::Quaterniond::Identity());
    const auto copy(source);
    source.setPosition(0, {3, 2, 1});
    source.setRotation(0, Eigen::Quaterniond(Eigen::AngleAxisd(
                              30.0, Eigen::Vector3d{1, 2, 3})));
    TS_ASSERT_EQUALS(copy.size(), 7);
    TS_ASSERT_EQUALS(copy.position(0), Eigen::Vector3d(1, 2, 3));
    TS_ASSERT_EQUALS(copy.rotation(0).coeffs(),
                     Eigen::Quaterniond::Identity().coeffs());
  }

  void test_setPosition() {
    DetectorInfo info(PosVec(1), RotVec(1));
    Eigen::Vector3d pos{1, 2, 3};
    info.setPosition(0, pos);
    TS_ASSERT_EQUALS(info.position(0), pos);
  }

  void test_setRotattion() {
    DetectorInfo info(PosVec(1), RotVec(1));
    Eigen::Quaterniond rot{1, 2, 3, 4};
    info.setRotation(0, rot);
    TS_ASSERT_EQUALS(info.rotation(0).coeffs(), rot.normalized().coeffs());
  }

  void test_scanCount() {
    DetectorInfo info(PosVec(1), RotVec(1));
    TS_ASSERT_EQUALS(info.scanCount(0), 1);
  }

  void test_scanInterval() {
    DetectorInfo info(PosVec(1), RotVec(1));
    TS_ASSERT_EQUALS(info.scanInterval({0, 0}),
                     (std::pair<int64_t, int64_t>(0, 0)));
  }

  void test_setScanInterval() {
    DetectorInfo info(PosVec(1), RotVec(1));
    info.setScanInterval(0, {1, 2});
    TS_ASSERT_EQUALS(info.scanInterval({0, 0}),
                     (std::pair<int64_t, int64_t>(1, 2)));
  }

  void test_setScanInterval_failures() {
    DetectorInfo info(PosVec(1), RotVec(1));
    TS_ASSERT_THROWS_EQUALS(
        info.setScanInterval(0, {1, 1}), const std::runtime_error &e,
        std::string(e.what()),
        "DetectorInfo: cannot set scan interval with start >= end");
    TS_ASSERT_THROWS_EQUALS(
        info.setScanInterval(0, {2, 1}), const std::runtime_error &e,
        std::string(e.what()),
        "DetectorInfo: cannot set scan interval with start >= end");
  }

  void test_merge_fail_size() {
    DetectorInfo a(PosVec(1), RotVec(1));
    DetectorInfo b(PosVec(2), RotVec(2));
    a.setScanInterval(0, {0, 1});
    b.setScanInterval(0, {0, 1});
    b.setScanInterval(1, {0, 1});
    TS_ASSERT_THROWS_EQUALS(a.merge(b), const std::runtime_error &e,
                            std::string(e.what()),
                            "Cannot merge DetectorInfo: size mismatch");
  }

  void test_merge_fail_no_intervals() {
    DetectorInfo a(PosVec(1), RotVec(1));
    DetectorInfo b(PosVec(1), RotVec(1));
    DetectorInfo c(PosVec(1), RotVec(1));
    TS_ASSERT_THROWS_EQUALS(
        a.merge(b), const std::runtime_error &e, std::string(e.what()),
        "Cannot merge DetectorInfo: scan intervals not defined");
    c.setScanInterval(0, {0, 1});
    TS_ASSERT_THROWS_EQUALS(
        a.merge(c), const std::runtime_error &e, std::string(e.what()),
        "Cannot merge DetectorInfo: scan intervals not defined");
    a.setScanInterval(0, {0, 1});
    TS_ASSERT_THROWS_EQUALS(
        a.merge(b), const std::runtime_error &e, std::string(e.what()),
        "Cannot merge DetectorInfo: scan intervals not defined");
  }

  void test_merge_fail_monitor_mismatch() {
    DetectorInfo a(PosVec(2), RotVec(2));
    DetectorInfo b(PosVec(2), RotVec(2), {1});
    a.setScanInterval(0, {0, 1});
    a.setScanInterval(1, {0, 1});
    b.setScanInterval(0, {0, 1});
    b.setScanInterval(1, {0, 1});
    TS_ASSERT_THROWS_EQUALS(
        a.merge(b), const std::runtime_error &e, std::string(e.what()),
        "Cannot merge DetectorInfo: monitor flags mismatch");
  }

  void test_merge_identical_interval_failures() {
    DetectorInfo a(PosVec(1), RotVec(1));
    a.setScanInterval(0, {0, 1});
    Eigen::Vector3d pos1(1, 0, 0);
    Eigen::Vector3d pos2(2, 0, 0);
    Eigen::Quaterniond rot1(
        Eigen::AngleAxisd(30.0, Eigen::Vector3d{1, 2, 3}.normalized()));
    Eigen::Quaterniond rot2(
        Eigen::AngleAxisd(31.0, Eigen::Vector3d{1, 2, 3}.normalized()));
    a.setMasked(0, true);
    a.setPosition(0, pos1);
    a.setRotation(0, rot1);
    auto b(a);
    TS_ASSERT_THROWS_NOTHING(b.merge(a));

    b = a;
    b.setMasked(0, false);
    TS_ASSERT_THROWS_EQUALS(b.merge(a), const std::runtime_error &e,
                            std::string(e.what()), "Cannot merge DetectorInfo: "
                                                   "matching scan interval but "
                                                   "mask flags differ");
    b.setMasked(0, true);
    TS_ASSERT_THROWS_NOTHING(b.merge(a));

    b = a;
    b.setPosition(0, pos2);
    TS_ASSERT_THROWS_EQUALS(b.merge(a), const std::runtime_error &e,
                            std::string(e.what()), "Cannot merge DetectorInfo: "
                                                   "matching scan interval but "
                                                   "positions differ");
    b.setPosition(0, pos1);
    TS_ASSERT_THROWS_NOTHING(b.merge(a));

    b = a;
    b.setRotation(0, rot2);
    TS_ASSERT_THROWS_EQUALS(b.merge(a), const std::runtime_error &e,
                            std::string(e.what()), "Cannot merge DetectorInfo: "
                                                   "matching scan interval but "
                                                   "rotations differ");
    b.setRotation(0, rot1);
    TS_ASSERT_THROWS_NOTHING(b.merge(a));
  }

  void test_merge_identical_interval() {
    DetectorInfo a(PosVec(1), RotVec(1));
    a.setScanInterval(0, {0, 1});
    const auto b(a);
    TS_ASSERT_THROWS_NOTHING(a.merge(b));
    TS_ASSERT(a.isEquivalent(b));
  }

  void test_merge_identical_interval_with_monitor() {
    DetectorInfo a(PosVec(2), RotVec(2), {1});
    a.setScanInterval(0, {0, 1});
    a.setScanInterval(1, {0, 1});
    const auto b(a);
    TS_ASSERT_THROWS_NOTHING(a.merge(b));
    TS_ASSERT(a.isEquivalent(b));
  }

  void test_merge_fail_partial_overlap() {
    DetectorInfo a(PosVec(2), RotVec(2));
    a.setScanInterval(0, {0, 10});
    a.setScanInterval(1, {0, 10});
    auto b(a);
    TS_ASSERT_THROWS_NOTHING(b.merge(a));
    b = a;
    b.setScanInterval(1, {-1, 5});
    TS_ASSERT_THROWS_EQUALS(
        b.merge(a), const std::runtime_error &e, std::string(e.what()),
        "Cannot merge DetectorInfo: scan intervals overlap but not identical");
    b.setScanInterval(1, {1, 5});
    TS_ASSERT_THROWS_EQUALS(
        b.merge(a), const std::runtime_error &e, std::string(e.what()),
        "Cannot merge DetectorInfo: scan intervals overlap but not identical");
    b.setScanInterval(1, {1, 11});
    TS_ASSERT_THROWS_EQUALS(
        b.merge(a), const std::runtime_error &e, std::string(e.what()),
        "Cannot merge DetectorInfo: scan intervals overlap but not identical");
  }

  void test_merge() {
    DetectorInfo a(PosVec(2), RotVec(2), {1});
    // Monitor at index 1, set up for identical interval
    std::pair<int64_t, int64_t> monitorInterval(0, 2);
    a.setScanInterval(1, monitorInterval);
    auto b(a);
    Eigen::Vector3d pos1(1, 0, 0);
    Eigen::Vector3d pos2(2, 0, 0);
    a.setPosition(0, pos1);
    b.setPosition(0, pos2);
    std::pair<int64_t, int64_t> interval1(0, 1);
    std::pair<int64_t, int64_t> interval2(1, 2);
    a.setScanInterval(0, interval1);
    b.setScanInterval(0, interval2);
    TS_ASSERT_THROWS_NOTHING(a.merge(b));
    TS_ASSERT(a.isScanning());
    TS_ASSERT(!a.isEquivalent(b));
    TS_ASSERT_EQUALS(a.size(), 2);
    TS_ASSERT_EQUALS(a.scanCount(0), 2);
    // Note that the order is not guaranteed, currently these are just int the
    // order in which the are merged.
    TS_ASSERT_EQUALS(a.scanInterval({0, 0}), interval1);
    TS_ASSERT_EQUALS(a.scanInterval({0, 1}), interval2);
    TS_ASSERT_EQUALS(a.position({0, 0}), pos1);
    TS_ASSERT_EQUALS(a.position({0, 1}), pos2);
    // Monitor is not scanning
    TS_ASSERT_EQUALS(a.scanCount(1), 1);
  }

  void test_merge_idempotent() {
    // Test that A + B + B = A + B
    DetectorInfo a(PosVec(2), RotVec(2), {1});
    // Monitor at index 1, set up for identical interval
    std::pair<int64_t, int64_t> monitorInterval(0, 2);
    a.setScanInterval(1, monitorInterval);
    a.setPosition(1, {0, 0, 0});
    auto b(a);
    Eigen::Vector3d pos1(1, 0, 0);
    Eigen::Vector3d pos2(2, 0, 0);
    a.setPosition(0, pos1);
    b.setPosition(0, pos2);
    std::pair<int64_t, int64_t> interval1(0, 1);
    std::pair<int64_t, int64_t> interval2(1, 2);
    a.setScanInterval(0, interval1);
    b.setScanInterval(0, interval2);

    TS_ASSERT_THROWS_NOTHING(a.merge(b));
    auto a0(a);
    TS_ASSERT_THROWS_NOTHING(a.merge(b));
    TS_ASSERT(a.isEquivalent(a0));
  }

  void test_merge_multiple() {
    DetectorInfo a(PosVec(2), RotVec(2), {1});
    // Monitor at index 1, set up for identical interval
    std::pair<int64_t, int64_t> monitorInterval(0, 3);
    a.setScanInterval(1, monitorInterval);
    auto b(a);
    auto c(a);
    Eigen::Vector3d pos1(1, 0, 0);
    Eigen::Vector3d pos2(2, 0, 0);
    Eigen::Vector3d pos3(3, 0, 0);
    a.setPosition(0, pos1);
    b.setPosition(0, pos2);
    c.setPosition(0, pos3);
    std::pair<int64_t, int64_t> interval1(0, 1);
    std::pair<int64_t, int64_t> interval2(1, 2);
    std::pair<int64_t, int64_t> interval3(2, 3);
    a.setScanInterval(0, interval1);
    b.setScanInterval(0, interval2);
    c.setScanInterval(0, interval3);
    TS_ASSERT_THROWS_NOTHING(a.merge(b));
    TS_ASSERT_THROWS_NOTHING(a.merge(c));
    TS_ASSERT(a.isScanning());
    TS_ASSERT(!a.isEquivalent(b));
    TS_ASSERT(!a.isEquivalent(c));
    TS_ASSERT_EQUALS(a.size(), 2);
    TS_ASSERT_EQUALS(a.scanCount(0), 3);
    TS_ASSERT_EQUALS(a.scanInterval({0, 0}), interval1);
    TS_ASSERT_EQUALS(a.scanInterval({0, 1}), interval2);
    TS_ASSERT_EQUALS(a.scanInterval({0, 2}), interval3);
    TS_ASSERT_EQUALS(a.position({0, 0}), pos1);
    TS_ASSERT_EQUALS(a.position({0, 1}), pos2);
    TS_ASSERT_EQUALS(a.position({0, 2}), pos3);
    // Monitor is not scanning
    TS_ASSERT_EQUALS(a.scanCount(1), 1);
  }

  void test_merge_multiple_associative() {
    // Test that (A + B) + C == A + (B + C)
    // This is implied by the ordering guaranteed by merge().
    DetectorInfo a1(PosVec(1), RotVec(1));
    a1.setRotation(0, Eigen::Quaterniond::Identity());
    auto b(a1);
    auto c(a1);
    Eigen::Vector3d pos1(1, 0, 0);
    Eigen::Vector3d pos2(2, 0, 0);
    Eigen::Vector3d pos3(3, 0, 0);
    a1.setPosition(0, pos1);
    b.setPosition(0, pos2);
    c.setPosition(0, pos3);
    std::pair<int64_t, int64_t> interval1(0, 1);
    std::pair<int64_t, int64_t> interval2(1, 2);
    std::pair<int64_t, int64_t> interval3(2, 3);
    a1.setScanInterval(0, interval1);
    b.setScanInterval(0, interval2);
    c.setScanInterval(0, interval3);
    auto a2(a1);
    TS_ASSERT_THROWS_NOTHING(a1.merge(b));
    TS_ASSERT_THROWS_NOTHING(a1.merge(c));
    TS_ASSERT_THROWS_NOTHING(b.merge(c));
    TS_ASSERT_THROWS_NOTHING(a2.merge(b));
    TS_ASSERT(a1.isEquivalent(a2));
  }
};

#endif /* MANTID_BEAMLINE_DETECTORINFOTEST_H_ */
