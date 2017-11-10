#ifndef MANTID_ALGORITHMS_RESIZERECTANGULARDETECTORTEST_H_
#define MANTID_ALGORITHMS_RESIZERECTANGULARDETECTORTEST_H_

#include "MantidAlgorithms/ResizeRectangularDetector.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include "MantidGeometry/Instrument/RectangularDetectorPixel.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class ResizeRectangularDetectorTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    ResizeRectangularDetector alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    Mantid::DataObjects::EventWorkspace_sptr ews =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 10);

    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(ews);

    ResizeRectangularDetector alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ComponentName", "bank1"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ScaleX", 2.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ScaleY", 0.5));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    Instrument_const_sptr inst = ws->getInstrument();
    boost::shared_ptr<const RectangularDetector> det =
        boost::dynamic_pointer_cast<const RectangularDetector>(
            inst->getComponentByName("bank1"));

    // Bank 1 got scaled
    V3D pos;
    pos = det->getAtXY(1, 1)->getPos();
    TS_ASSERT(ws->constInstrumentParameters().contains(det.get(), "scalex"));
    TS_ASSERT(ws->constInstrumentParameters().contains(det.get(), "scaley"));
    TS_ASSERT_EQUALS(pos, V3D(0.008 * 2, 0.008 * 0.5, 5.0));
    TS_ASSERT_DELTA(det->xstep(), 0.008 * 2, 1e-6);

    // Check that accessing through spectrumInfo.detector() also works
    const RectangularDetectorPixel *recDetPix;
    const auto &spectrumInfo = ws->spectrumInfo();
    const auto &pixel = spectrumInfo.detector(11);
    recDetPix = dynamic_cast<const RectangularDetectorPixel *>(
        det->getAtXY(1, 1).get());
    TSM_ASSERT("getDetector() returns a RectangularDetectorPixel", recDetPix);
    pos = pixel.getPos();
    TS_ASSERT_EQUALS(pos, V3D(0.008 * 2, 0.008 * 0.5, 5.0));

    // Bank 2 did not get scaled
    det = boost::dynamic_pointer_cast<const RectangularDetector>(
        inst->getComponentByName("bank2"));
    pos = det->getAtXY(1, 1)->getPos();
    TS_ASSERT_EQUALS(pos, V3D(0.008 * 1.0, 0.008 * 1.0, 10.0));
    TS_ASSERT_DELTA(det->xstep(), 0.008 * 1, 1e-6);
  }
};

#endif /* MANTID_ALGORITHMS_RESIZERECTANGULARDETECTORTEST_H_ */
