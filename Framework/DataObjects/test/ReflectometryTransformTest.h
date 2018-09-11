#ifndef MANTID_DATAOBJECTS_REFLECTOMETRYTRANSFORTEST_H_
#define MANTID_DATAOBJECTS_REFLECTOMETRYTRANSFORTEST_H_

#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/ReflectometryTransform.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

class ReflectometryTransformTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryTransformTest *createSuite() {
    return new ReflectometryTransformTest();
  }
  static void destroySuite(ReflectometryTransformTest *suite) { delete suite; }

  void test_cache_calculation_when_y_is_up() {

    // Creates a detector with dimensions x=0.02, y=0.04, z=0.06
    auto reflWS =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();

    // Get the existing instrument
    Instrument_sptr inst =
        boost::const_pointer_cast<Instrument>(reflWS->getInstrument());
    // Replace the reference frame
    inst->setReferenceFrame(boost::make_shared<ReferenceFrame>(
        Y /*up*/, X /*along beam*/, Left, "0,0,0"));
    // Reset the instrument on the ws
    reflWS->setInstrument(inst);

    const auto &spectrumInfo = reflWS->spectrumInfo();
    const auto l2 = spectrumInfo.l2(0);

    DetectorAngularCache cache = initAngularCaches(reflWS.get());

    TS_ASSERT_DELTA(0.04, cache.detectorHeights[0], 1e-6);

    auto calculatedThetaW =
        2.0 * std::fabs(std::atan((cache.detectorHeights[0] / 2) / l2)) *
        180.0 / M_PI;

    TSM_ASSERT_DELTA(
        "Calculated theta width should agree with detector height calculation",
        cache.thetaWidths[0], calculatedThetaW, 1e-6)
  }

  void test_cache_calculation_when_x_is_up() {

    // Creates a detector with dimensions x=0.02, y=0.04, z=0.06
    auto reflWS =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();

    // Get the existing instrument
    Instrument_sptr inst =
        boost::const_pointer_cast<Instrument>(reflWS->getInstrument());
    // Replace the reference frame
    inst->setReferenceFrame(boost::make_shared<ReferenceFrame>(
        X /*up*/, Y /*along beam*/, Left, "0,0,0"));
    // Reset the instrument on the ws
    reflWS->setInstrument(inst);

    DetectorAngularCache cache = initAngularCaches(reflWS.get());

    TS_ASSERT_DELTA(0.02, cache.detectorHeights[0], 1e6);
  }

  void test_cache_calculation_when_z_is_up() {

    // Creates a detector with dimensions x=0.02, y=0.04, z=0.06
    auto reflWS =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();

    // Get the existing instrument
    Instrument_sptr inst =
        boost::const_pointer_cast<Instrument>(reflWS->getInstrument());
    // Replace the reference frame
    inst->setReferenceFrame(boost::make_shared<ReferenceFrame>(
        Z /*up*/, X /*along beam*/, Left, "0,0,0"));
    // Reset the instrument on the ws
    reflWS->setInstrument(inst);

    DetectorAngularCache cache = initAngularCaches(reflWS.get());

    TS_ASSERT_DELTA(0.06, cache.detectorHeights[0], 1e6);
  }
};

#endif /* MANTID_DATAOBJECTS_REFLECTOMETRYTRANSFORTEST_H_ */
