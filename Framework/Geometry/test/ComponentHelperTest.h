#ifndef MANTID_GEOMETRY_COMPONENTHELPERTEST_H_
#define MANTID_GEOMETRY_COMPONENTHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

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

  void test_absolute_planar_rotation_takes_into_account_grandparent() {
    const int banks = 1;
    const int pixels = 1;
    auto base =
        ComponentCreationHelper::createTestInstrumentRectangular(banks, pixels);
    auto pmap = boost::make_shared<ParameterMap>();
    Instrument instrument(base, pmap);

    const auto bank = instrument.getComponentByName("bank1");
    const auto pixel = instrument.getDetector(1);

    Quat rootRot(100.0, V3D(1, 2, 3));
    Quat bankRot(110.0, V3D(1, 2, 3));
    Quat pixelRot(111.0, V3D(1, 2, 3));

    const auto type = ComponentHelper::TransformType::Absolute;
    ComponentHelper::rotateComponent(instrument, *pmap, rootRot, type);
    TS_ASSERT_EQUALS(instrument.getRotation(), rootRot);
    TS_ASSERT_EQUALS(bank->getRotation(), rootRot);
    TS_ASSERT_EQUALS(pixel->getRotation(), rootRot);
    ComponentHelper::rotateComponent(*bank, *pmap, bankRot, type);
    TS_ASSERT_EQUALS(instrument.getRotation(), rootRot);
    TS_ASSERT_EQUALS(bank->getRotation(), bankRot);
    TS_ASSERT_EQUALS(pixel->getRotation(), bankRot);
    ComponentHelper::rotateComponent(*pixel, *pmap, pixelRot, type);
    TS_ASSERT_EQUALS(instrument.getRotation(), rootRot);
    TS_ASSERT_EQUALS(bank->getRotation(), bankRot);
    TS_ASSERT_EQUALS(pixel->getRotation(), pixelRot);
  }

  void test_absolute_rotation_takes_into_account_parent_correctly() {
    const int banks = 1;
    const int pixels = 1;
    auto base =
        ComponentCreationHelper::createTestInstrumentRectangular(banks, pixels);
    auto pmap = boost::make_shared<ParameterMap>();
    Instrument instrument(base, pmap);

    const auto bank = instrument.getComponentByName("bank1");
    const auto pixel = instrument.getDetector(1);

    Quat bankRot(10.0, V3D(1, 0, 0));
    Quat pixelRot(20.0, V3D(0, 1, 0));

    const auto type = ComponentHelper::TransformType::Absolute;
    ComponentHelper::rotateComponent(*bank, *pmap, bankRot, type);
    TS_ASSERT_EQUALS(bank->getRotation(), bankRot);
    TS_ASSERT_EQUALS(pixel->getRotation(), bankRot);
    ComponentHelper::rotateComponent(*pixel, *pmap, pixelRot, type);
    TS_ASSERT_EQUALS(bank->getRotation(), bankRot);
    TS_ASSERT_EQUALS(pixel->getRotation(), pixelRot);
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

class ComponentHelperTestPerformance : public CxxTest::TestSuite {

private:
  Mantid::Geometry::Instrument_sptr m_sansInstrument;
  Mantid::Geometry::IComponent_const_sptr m_sansFrontTrolley;
  Mantid::Geometry::IComponent_const_sptr m_sansBank;
  boost::shared_ptr<Mantid::Geometry::ParameterMap> m_paramMap;
  Mantid::Kernel::Quat m_zRotation;
  Mantid::Kernel::V3D m_pos;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComponentHelperTestPerformance *createSuite() {
    return new ComponentHelperTestPerformance();
  }
  static void destroySuite(ComponentHelperTestPerformance *suite) {
    delete suite;
  }

  ComponentHelperTestPerformance() {

    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    Mantid::Kernel::V3D sourcePos(0, 0, 0);
    Mantid::Kernel::V3D samplePos(0, 0, 1);
    Mantid::Kernel::V3D trolley1Pos(0, 0, 3);
    Mantid::Kernel::V3D trolley2Pos(0, 0, 6);

    m_paramMap = boost::make_shared<Mantid::Geometry::ParameterMap>();

    auto baseInstrument = ComponentCreationHelper::sansInstrument(
        sourcePos, samplePos, trolley1Pos, trolley2Pos);

    m_sansInstrument =
        boost::make_shared<Instrument>(baseInstrument, m_paramMap);

    // See component creation helper for instrument definition
    m_sansFrontTrolley = m_sansInstrument->getComponentByName("Trolley1");
    m_sansBank = m_sansInstrument->getComponentByName("Bank1");

    m_zRotation =
        Mantid::Kernel::Quat(180, V3D(0, 0, 1)); // rotate 180 degrees around z

    m_pos = Mantid::Kernel::V3D(1, 1, 1);
  }

  void do_rotate_whole_instrument_x1000(
      const Mantid::Geometry::ComponentHelper::TransformType transformType) {

    using namespace Mantid::Geometry;

    for (size_t i = 0; i < 1000; ++i) {
      ComponentHelper::rotateComponent(*m_sansInstrument, *m_paramMap,
                                       m_zRotation, transformType);
    }
  }

  void do_rotate_trolley_x1000(
      const Mantid::Geometry::ComponentHelper::TransformType transformType) {

    using namespace Mantid::Geometry;

    for (size_t i = 0; i < 1000; ++i) {

      ComponentHelper::rotateComponent(*m_sansFrontTrolley, *m_paramMap,
                                       m_zRotation, transformType);
    }
  }

  void do_rotate_bank_x1000(
      const Mantid::Geometry::ComponentHelper::TransformType transformType) {

    using namespace Mantid::Geometry;

    for (size_t i = 0; i < 1000; ++i) {
      ComponentHelper::rotateComponent(*m_sansBank, *m_paramMap, m_zRotation,
                                       transformType);
    }
  }

  void do_translate_whole_instrument_x1000(
      const Mantid::Geometry::ComponentHelper::TransformType transformType) {

    using namespace Mantid::Geometry;

    for (size_t i = 0; i < 1000; ++i) {
      ComponentHelper::moveComponent(*m_sansInstrument, *m_paramMap, m_pos,
                                     transformType);
    }
  }

  void do_translate_trolley_x1000(
      const Mantid::Geometry::ComponentHelper::TransformType transformType) {

    using namespace Mantid::Geometry;

    for (size_t i = 0; i < 1000; ++i) {
      ComponentHelper::moveComponent(*m_sansFrontTrolley, *m_paramMap, m_pos,
                                     transformType);
    }
  }

  void do_translate_bank_x1000(
      const Mantid::Geometry::ComponentHelper::TransformType transformType) {

    using namespace Mantid::Geometry;

    for (size_t i = 0; i < 1000; ++i) {
      ComponentHelper::moveComponent(*m_sansBank, *m_paramMap, m_pos,
                                     transformType);
    }
  }

  void test_rotate_whole_instrument_absolute_x1000() {

    do_rotate_whole_instrument_x1000(
        Mantid::Geometry::ComponentHelper::Absolute);
  }

  void test_rotate_trolley_absolute_x1000() {

    do_rotate_trolley_x1000(Mantid::Geometry::ComponentHelper::Absolute);
  }

  void test_rotate_bank_absolute_x1000() {

    do_rotate_bank_x1000(Mantid::Geometry::ComponentHelper::Relative);
  }

  void test_rotate_whole_instrument_relative_x1000() {

    do_rotate_whole_instrument_x1000(
        Mantid::Geometry::ComponentHelper::Relative);
  }

  void test_rotate_trolley_relative_x1000() {

    do_rotate_trolley_x1000(Mantid::Geometry::ComponentHelper::Relative);
  }

  void test_rotate_bank_relative_x1000() {

    do_rotate_bank_x1000(Mantid::Geometry::ComponentHelper::Relative);
  }

  void test_translate_whole_instrument_absolute_x1000() {

    do_translate_whole_instrument_x1000(
        Mantid::Geometry::ComponentHelper::Absolute);
  }

  void test_translate_trolley_absolute_x1000() {

    do_translate_trolley_x1000(Mantid::Geometry::ComponentHelper::Absolute);
  }

  void test_translate_bank_absolute_x1000() {

    do_translate_bank_x1000(Mantid::Geometry::ComponentHelper::Relative);
  }

  void test_translate_whole_instrument_relative_x1000() {

    do_translate_whole_instrument_x1000(
        Mantid::Geometry::ComponentHelper::Relative);
  }

  void test_translate_trolley_relative_x1000() {

    do_translate_trolley_x1000(Mantid::Geometry::ComponentHelper::Relative);
  }

  void test_translate_bank_relative_x1000() {

    do_translate_bank_x1000(Mantid::Geometry::ComponentHelper::Relative);
  }

  /*
   * This is a very typical scenario. No unpaired writes without reads.
   */
  void test_rotate_bank_and_read_positions() {

    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    ComponentHelper::rotateComponent(
        *m_sansBank, *m_paramMap, m_zRotation,
        Mantid::Geometry::ComponentHelper::Relative);

    V3D pos;
    for (int i = 1;
         i <= static_cast<int>(m_sansInstrument->getNumberDetectors()); ++i) {
      pos += m_sansInstrument->getDetector(i)->getPos();
    }
  }

  /*
   * This is a very typical scenario. No unpaired writes without reads.
   */
  void test_move_bank_and_read_positions() {

    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    ComponentHelper::moveComponent(*m_sansBank, *m_paramMap, m_pos,
                                   Mantid::Geometry::ComponentHelper::Relative);

    V3D pos;
    for (int i = 1;
         i <= static_cast<int>(m_sansInstrument->getNumberDetectors()); ++i) {
      pos += m_sansInstrument->getDetector(i)->getPos();
    }
  }
};

#endif /* MANTID_GEOMETRY_COMPONENTHELPERTEST_H_ */
