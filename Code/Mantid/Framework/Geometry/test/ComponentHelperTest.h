#ifndef MANTID_GEOMETRY_COMPONENTHELPERTEST_H_
#define MANTID_GEOMETRY_COMPONENTHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <boost/make_shared.hpp>

class ComponentHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComponentHelperTest *createSuite() {
    return new ComponentHelperTest();
  }
  static void destroySuite(ComponentHelperTest *suite) { delete suite; }

  void test_moveComponent_With_Absolute_Position() {
    using namespace Mantid::Geometry;
    auto inst = createTestInstrument();
    auto det = inst->getDetector(1);
    auto pmap = inst->getParameterMap();

    Mantid::Kernel::V3D absPos;
    absPos.spherical(2.5, 45, 10);

    TS_ASSERT_THROWS_NOTHING(ComponentHelper::moveComponent(
        *det, *pmap, absPos, ComponentHelper::Absolute));

    auto newPos = det->getPos();
    TS_ASSERT_DELTA(newPos.X(), absPos.X(), 1e-12)
    TS_ASSERT_DELTA(newPos.Y(), absPos.Y(), 1e-12)
    TS_ASSERT_DELTA(newPos.Z(), absPos.Z(), 1e-12);
  }

  void test_moveComponent_With_Relative_Position() {
    using namespace Mantid::Geometry;
    auto inst = createTestInstrument();
    auto det = inst->getDetector(1);
    auto pmap = inst->getParameterMap();

    auto origPos = det->getPos();
    Mantid::Kernel::V3D shift(1.5, -2.5, 3.6);
    TS_ASSERT_THROWS_NOTHING(ComponentHelper::moveComponent(
        *det, *pmap, shift, ComponentHelper::Relative));

    auto expectedPos = origPos + shift;
    auto newPos = det->getPos();
    TS_ASSERT_DELTA(newPos.X(), expectedPos.X(), 1e-12)
    TS_ASSERT_DELTA(newPos.Y(), expectedPos.Y(), 1e-12)
    TS_ASSERT_DELTA(newPos.Z(), expectedPos.Z(), 1e-12);
  }

  void test_rotateComponent_With_Absolute_Rotation() {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    auto inst = createTestInstrument();
    auto det = inst->getDetector(1);
    auto pmap = inst->getParameterMap();

    const double angle = 52.0;
    const V3D axis(0.0, 1.0, 1.0);
    Quat rotation(angle, axis);

    TS_ASSERT_THROWS_NOTHING(ComponentHelper::rotateComponent(
        *det, *pmap, rotation, ComponentHelper::Absolute));

    det = inst->getDetector(1);
    auto newRot = det->getRotation();
    TS_ASSERT_DELTA(newRot.real(), rotation.real(), 1e-12);
    TS_ASSERT_DELTA(newRot.imagI(), rotation.imagI(), 1e-12);
    TS_ASSERT_DELTA(newRot.imagJ(), rotation.imagJ(), 1e-12);
    TS_ASSERT_DELTA(newRot.imagK(), rotation.imagK(), 1e-12);
  }

  void test_rotateComponent_With_Relative_Rotation() {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    // The test instrument starts with zero rotation so rotate first of the
    // relative one will be the same as absolute
    auto inst = createTestInstrument();
    auto pmap = inst->getParameterMap();

    const double angle = 45.0;
    const V3D axis(0.0, 0.0, 1.0);
    Quat rotation(angle, axis);

    TS_ASSERT_THROWS_NOTHING(ComponentHelper::rotateComponent(
        *inst, *pmap, rotation,
        ComponentHelper::Absolute)); // Absolute for Instrument

    auto det = inst->getDetector(1);
    TS_ASSERT_THROWS_NOTHING(ComponentHelper::rotateComponent(
        *det, *pmap, rotation,
        ComponentHelper::Relative)); // Relative for Detector

    auto newRot = det->getRotation();
    auto expectedRot = rotation * rotation;

    TS_ASSERT_DELTA(newRot.real(), expectedRot.real(), 1e-12);
    TS_ASSERT_DELTA(newRot.imagI(), expectedRot.imagI(), 1e-12);
    TS_ASSERT_DELTA(newRot.imagJ(), expectedRot.imagJ(), 1e-12);
    TS_ASSERT_DELTA(newRot.imagK(), expectedRot.imagK(), 1e-12);
  }

private:
  Mantid::Geometry::Instrument_sptr createTestInstrument() {
    using namespace Mantid::Geometry;
    auto baseInst =
        ComponentCreationHelper::createTestInstrumentCylindrical(1); // 1 bank
    auto pmap = boost::make_shared<ParameterMap>();
    return boost::make_shared<Instrument>(baseInst, pmap);
  }
};

#endif /* MANTID_GEOMETRY_COMPONENTHELPERTEST_H_ */
