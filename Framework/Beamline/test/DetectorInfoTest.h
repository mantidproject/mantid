#ifndef MANTID_BEAMLINE_DETECTORINFOTEST_H_
#define MANTID_BEAMLINE_DETECTORINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/DetectorInfo.h"
#include "MantidKernel/make_unique.h"

using namespace Mantid;
using Beamline::DetectorInfo;

class DetectorInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorInfoTest *createSuite() { return new DetectorInfoTest(); }
  static void destroySuite(DetectorInfoTest *suite) { delete suite; }

  void test_constructor() {
    std::unique_ptr<DetectorInfo> detInfo;
    TS_ASSERT_THROWS_NOTHING(detInfo = Kernel::make_unique<DetectorInfo>(0));
    TS_ASSERT_EQUALS(detInfo->size(), 0);
    TS_ASSERT_THROWS_NOTHING(detInfo = Kernel::make_unique<DetectorInfo>(1));
    TS_ASSERT_EQUALS(detInfo->size(), 1);
  }

  void test_constructor_with_monitors() {
    std::unique_ptr<DetectorInfo> info;
    std::vector<size_t> mons{0, 2};
    TS_ASSERT_THROWS_NOTHING(info = Kernel::make_unique<DetectorInfo>(3, mons));
    TS_ASSERT_EQUALS(info->size(), 3);
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(3, {}));
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(3, {0}));
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(3, {0, 1, 2}));
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(3, {0, 0, 0}));
    TS_ASSERT_THROWS(DetectorInfo(3, {3}), std::out_of_range);
  }

  void test_copy() {
    const DetectorInfo source(7);
    const auto copy(source);
    TS_ASSERT_EQUALS(copy.size(), 7);
  }

  void test_move() {
    DetectorInfo source(7);
    const auto moved(std::move(source));
    TS_ASSERT_EQUALS(moved.size(), 7);
    // TODO once DetectorInfo has moveable fields, check that they are cleared.
  }

  void test_assign() {
    const DetectorInfo source(7);
    DetectorInfo assignee(1);
    assignee = source;
    TS_ASSERT_EQUALS(assignee.size(), 7);
  }

  void test_move_assign() {
    DetectorInfo source(7);
    DetectorInfo assignee(1);
    assignee = std::move(source);
    TS_ASSERT_EQUALS(assignee.size(), 7);
    // TODO once DetectorInfo has moveable fields, check that they are cleared.
  }

  void test_no_monitors() {
    DetectorInfo info(3);
    TS_ASSERT(!info.isMonitor(0));
    TS_ASSERT(!info.isMonitor(1));
    TS_ASSERT(!info.isMonitor(2));
  }

  void test_monitors() {
    std::vector<size_t> monitors{0, 2};
    DetectorInfo info(3, monitors);
    TS_ASSERT(info.isMonitor(0));
    TS_ASSERT(!info.isMonitor(1));
    TS_ASSERT(info.isMonitor(2));
  }

  void test_duplicate_monitors_ignored() {
    std::vector<size_t> monitors{0, 0, 2, 2};
    DetectorInfo info(3, monitors);
    TS_ASSERT(info.isMonitor(0));
    TS_ASSERT(!info.isMonitor(1));
    TS_ASSERT(info.isMonitor(2));
  }

  void test_masking() {
    DetectorInfo info(3);
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
    DetectorInfo source(1);
    source.setMasked(0, true);
    DetectorInfo copy(source);
    TS_ASSERT(copy.isMasked(0));
    source.setMasked(0, false);
    TS_ASSERT(!source.isMasked(0));
    TS_ASSERT(copy.isMasked(0));
  }
};

#endif /* MANTID_BEAMLINE_DETECTORINFOTEST_H_ */
