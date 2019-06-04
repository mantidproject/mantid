// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_BEAMLINE_DETECTORINFOTEST_H_
#define MANTID_BEAMLINE_DETECTORINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"


using namespace Mantid;
using Beamline::DetectorInfo;
using PosVec = std::vector<Eigen::Vector3d>;
using RotVec = std::vector<Eigen::Quaterniond,
                           Eigen::aligned_allocator<Eigen::Quaterniond>>;

class DetectorInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorInfoTest *createSuite() { return new DetectorInfoTest(); }
  static void destroySuite(DetectorInfoTest *suite) { delete suite; }

  void test_constructor() {
    std::unique_ptr<DetectorInfo> detInfo;
    TS_ASSERT_THROWS_NOTHING(detInfo = std::make_unique<DetectorInfo>());
    TS_ASSERT_EQUALS(detInfo->size(), 0);
    TS_ASSERT(!detInfo->isScanning());
    TS_ASSERT(!detInfo->hasComponentInfo());

    TS_ASSERT_THROWS_NOTHING(
        detInfo = std::make_unique<DetectorInfo>(PosVec(1), RotVec(1)));
    TS_ASSERT_EQUALS(detInfo->size(), 1);
    TS_ASSERT(!detInfo->isScanning());
    TS_ASSERT(!detInfo->hasComponentInfo());
  }

  void test_constructor_with_monitors() {
    std::unique_ptr<DetectorInfo> info;
    std::vector<size_t> mons{0, 2};
    TS_ASSERT_THROWS_NOTHING(
        info = std::make_unique<DetectorInfo>(PosVec(3), RotVec(3), mons));
    TS_ASSERT_EQUALS(info->size(), 3);
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(PosVec(3), RotVec(3), {}));
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(PosVec(3), RotVec(3), {0}));
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(PosVec(3), RotVec(3), {0, 1, 2}));
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(PosVec(3), RotVec(3), {0, 0, 0}));
    TS_ASSERT_THROWS(DetectorInfo(PosVec(3), RotVec(3), {3}),
                     const std::out_of_range &);
  }

  void test_constructor_length_mismatch() {
    TS_ASSERT_THROWS(DetectorInfo(PosVec(3), RotVec(2)),
                     const std::runtime_error &);
  }

  void test_assign_componentInfo() {
    DetectorInfo detInfo;
    TS_ASSERT(!detInfo.hasComponentInfo());
    Mantid::Beamline::ComponentInfo compInfo;
    detInfo.setComponentInfo(&compInfo);
    TS_ASSERT(detInfo.hasComponentInfo());
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
    DetectorInfo detInfo;
    Mantid::Beamline::ComponentInfo compInfo;
    detInfo.setComponentInfo(&compInfo);
    TS_ASSERT_EQUALS(detInfo.scanCount(), 1);
  }

  void test_scanIntervals() {
    DetectorInfo detInfo;
    Mantid::Beamline::ComponentInfo compInfo;
    detInfo.setComponentInfo(&compInfo);
    TS_ASSERT_EQUALS(detInfo.scanIntervals(),
                     (std::vector<std::pair<int64_t, int64_t>>{{0, 1}}));
  }
};

#endif /* MANTID_BEAMLINE_DETECTORINFOTEST_H_ */
