#ifndef MANTID_API_RESIZERECTANGULARDETECTORHELPERTEST_H_
#define MANTID_API_RESIZERECTANGULARDETECTORHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/ResizeRectangularDetectorHelper.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid;
using namespace API;
using namespace Geometry;
using namespace Kernel;

class ResizeRectangularDetectorHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ResizeRectangularDetectorHelperTest *createSuite() {
    return new ResizeRectangularDetectorHelperTest();
  }
  static void destroySuite(ResizeRectangularDetectorHelperTest *suite) {
    delete suite;
  }

  void test_applyRectangularDetectorScaleToDetectorInfo() {
    WorkspaceTester ws;
    int pixels = 3;
    double pitch = 0.1;
    double distanceFromSample = 4.2;
    ws.initialize(pixels * pixels, 1, 1);
    ws.setInstrument(ComponentCreationHelper::createTestInstrumentRectangular(
        1, pixels, pitch, distanceFromSample));
    auto &detInfo = ws.mutableDetectorInfo();
    const auto bank = ws.getInstrument()->getComponentByName("bank1");
    const auto oldPos = bank->getPos();
    const auto oldRot = bank->getRotation();
    const V3D newPos(1, 2, 3);
    const V3D axis(3, 2, 1);
    const Quat newRot(13.4, axis);
    // Shift/rotate into non-trivial position.
    detInfo.setPosition(*bank, newPos);
    detInfo.setRotation(*bank, newRot);

    double scaleX = 7.3;
    double scaleY = 1.3;
    applyRectangularDetectorScaleToDetectorInfo(
        detInfo, dynamic_cast<const RectangularDetector &>(*bank), scaleX,
        scaleY);

    // Shift/rotate back for easy test of updated pixel positions
    detInfo.setRotation(*bank, oldRot);
    detInfo.setPosition(*bank, oldPos);
    TS_ASSERT_DELTA(detInfo.position(0)[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(detInfo.position(0)[1], 0.0, 1e-14);
    TS_ASSERT_DELTA(detInfo.position(0)[2], distanceFromSample, 1e-14);
    TS_ASSERT_DELTA(detInfo.position(1)[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(detInfo.position(1)[1], pitch * scaleY, 1e-14);
    TS_ASSERT_DELTA(detInfo.position(1)[2], distanceFromSample, 1e-14);
    TS_ASSERT_DELTA(detInfo.position(2)[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(detInfo.position(2)[1], (2.0 * pitch) * scaleY, 1e-14);
    TS_ASSERT_DELTA(detInfo.position(2)[2], distanceFromSample, 1e-14);
    TS_ASSERT_DELTA(detInfo.position(3)[0], pitch * scaleX, 1e-14);
    TS_ASSERT_DELTA(detInfo.position(3)[1], 0.0, 1e-14);
    TS_ASSERT_DELTA(detInfo.position(3)[2], distanceFromSample, 1e-14);
    TS_ASSERT_DELTA(detInfo.position(4)[0], pitch * scaleX, 1e-14);
    TS_ASSERT_DELTA(detInfo.position(4)[1], pitch * scaleY, 1e-14);
    TS_ASSERT_DELTA(detInfo.position(4)[2], distanceFromSample, 1e-14);
  }
};

#endif /* MANTID_API_RESIZERECTANGULARDETECTORHELPERTEST_H_ */
