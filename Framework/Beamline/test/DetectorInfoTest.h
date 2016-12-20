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
};

#endif /* MANTID_BEAMLINE_DETECTORINFOTEST_H_ */
