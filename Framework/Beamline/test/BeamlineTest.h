#ifndef MANTID_BEAMLINE_BEAMLINETEST_H_
#define MANTID_BEAMLINE_BEAMLINETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidBeamline/Beamline.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidBeamline/DetectorInfo.h"

using namespace Mantid::Beamline;

class BeamlineTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BeamlineTest *createSuite() { return new BeamlineTest(); }
  static void destroySuite(BeamlineTest *suite) { delete suite; }

  void test_cross_referencing_applied() {
    std::unique_ptr<ComponentInfo> componentInfo(new ComponentInfo{});
    std::unique_ptr<DetectorInfo> detectorInfo(new DetectorInfo{});

    // Sanity check no initial referencing
    TS_ASSERT(!componentInfo->hasDetectorInfo());
    TS_ASSERT(!detectorInfo->hasComponentInfo());

    Beamline beamline(std::move(componentInfo), std::move(detectorInfo));

    TSM_ASSERT("ComponentInfo should ref DetectorInfo",
               beamline.componentInfo().hasDetectorInfo())
    TSM_ASSERT("DetectorInfo should ref ComponentInfo",
               beamline.detectorInfo().hasComponentInfo())
  }
};

#endif /* MANTID_BEAMLINE_BEAMLINETEST_H_ */
