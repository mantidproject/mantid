#ifndef MANTID_GEOMETRY_SYMMETRYELEMENTTEST_H_
#define MANTID_GEOMETRY_SYMMETRYELEMENTTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidGeometry/Crystal/SymmetryElement.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidGeometry/Crystal/PointGroupFactory.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
class SymmetryElementTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SymmetryElementTest *createSuite() {
    return new SymmetryElementTest();
  }
  static void destroySuite(SymmetryElementTest *suite) { delete suite; }

  void testConstructionHmSymbol() {
    TS_ASSERT_THROWS_NOTHING(MockSymmetryElement element("1"));
    MockSymmetryElement element("1");

    TS_ASSERT_EQUALS(element.hmSymbol(), "1");
  }

  void testSymmetryElementIdentity() {
    SymmetryElementIdentity identity;

    TS_ASSERT_EQUALS(identity.hmSymbol(), "1");

    SymmetryElement_sptr cloned = identity.clone();
    SymmetryElementIdentity_sptr castedClone =
        boost::dynamic_pointer_cast<SymmetryElementIdentity>(cloned);

    TS_ASSERT(castedClone);
    TS_ASSERT_EQUALS(castedClone->hmSymbol(), "1");
  }

  void testSymmetryElementTranslation() {
    V3R bodyCenteringVector = V3R(1, 1, 1) / 2;
    SymmetryElementTranslation translation(bodyCenteringVector);
    TS_ASSERT_EQUALS(translation.hmSymbol(), "t");
    TS_ASSERT_EQUALS(translation.getTranslation(), bodyCenteringVector);

    SymmetryElement_sptr cloned = translation.clone();
    SymmetryElementTranslation_sptr castedClone =
        boost::dynamic_pointer_cast<SymmetryElementTranslation>(cloned);

    TS_ASSERT(castedClone);
    TS_ASSERT_EQUALS(castedClone->hmSymbol(), "t");
    TS_ASSERT_EQUALS(castedClone->getTranslation(), bodyCenteringVector);
  }

  void testSymmetryElementInversion() {
    V3R oneEighth = V3R(1, 1, 1) / 8;
    SymmetryElementInversion inversion(oneEighth);
    TS_ASSERT_EQUALS(inversion.hmSymbol(), "-1");
    TS_ASSERT_EQUALS(inversion.getInversionPoint(), oneEighth);

    SymmetryElement_sptr cloned = inversion.clone();
    SymmetryElementInversion_sptr castedClone =
        boost::dynamic_pointer_cast<SymmetryElementInversion>(cloned);

    TS_ASSERT(castedClone);
    TS_ASSERT_EQUALS(castedClone->hmSymbol(), "-1");
    TS_ASSERT_EQUALS(castedClone->getInversionPoint(), oneEighth);

    SymmetryElementInversion zeroInversion;
    TS_ASSERT_EQUALS(zeroInversion.getInversionPoint(), V3R(0, 0, 0));
  }

  void testSymmetryElementWithAxis() {
    V3R axis(0, 0, 1);
    V3R translation = V3R(0, 0, 1) / 4;

    TS_ASSERT_THROWS(MockSymmetryElementWithAxis invalidElement(
                         "41", V3R(0, 0, 0), translation),
                     std::invalid_argument);

    TS_ASSERT_THROWS_NOTHING(
        MockSymmetryElementWithAxis axisElement("41", axis, translation));

    MockSymmetryElementWithAxis axisElement("41", axis, translation);
    TS_ASSERT_EQUALS(axisElement.getAxis(), axis);
    TS_ASSERT_EQUALS(axisElement.getTranslation(), translation);
  }

  void testSymmetryElementRotation() {
    std::string symbolPlain("4");
    std::string symbolScrew("41");
    V3R axis(0, 0, 1);
    V3R translation = V3R(0, 0, 1) / 4;
    SymmetryElementRotation::RotationSense rotationSense(
        SymmetryElementRotation::Negative);

    SymmetryElementRotation defaultTranslation(symbolPlain, axis);
    TS_ASSERT_EQUALS(defaultTranslation.hmSymbol(), symbolPlain);
    TS_ASSERT_EQUALS(defaultTranslation.getAxis(), axis);
    TS_ASSERT_EQUALS(defaultTranslation.getTranslation(), V3R(0, 0, 0));
    TS_ASSERT_EQUALS(defaultTranslation.getRotationSense(),
                     SymmetryElementRotation::Positive);

    SymmetryElementRotation defaultSense(symbolScrew, axis, translation);
    TS_ASSERT_EQUALS(defaultSense.hmSymbol(), symbolScrew);
    TS_ASSERT_EQUALS(defaultSense.getAxis(), axis);
    TS_ASSERT_EQUALS(defaultSense.getTranslation(), translation);
    TS_ASSERT_EQUALS(defaultSense.getRotationSense(),
                     SymmetryElementRotation::Positive);

    SymmetryElementRotation rotationElement(symbolScrew, axis, translation,
                                            rotationSense);
    TS_ASSERT_EQUALS(rotationElement.hmSymbol(), symbolScrew);
    TS_ASSERT_EQUALS(rotationElement.getAxis(), axis);
    TS_ASSERT_EQUALS(rotationElement.getTranslation(), translation);
    TS_ASSERT_EQUALS(rotationElement.getRotationSense(), rotationSense);

    SymmetryElement_sptr cloned = rotationElement.clone();
    SymmetryElementRotation_sptr castedClone =
        boost::dynamic_pointer_cast<SymmetryElementRotation>(cloned);
    TS_ASSERT(castedClone);

    TS_ASSERT_EQUALS(castedClone->hmSymbol(), symbolScrew);
    TS_ASSERT_EQUALS(castedClone->getAxis(), axis);
    TS_ASSERT_EQUALS(castedClone->getTranslation(), translation);
    TS_ASSERT_EQUALS(castedClone->getRotationSense(), rotationSense);
  }

  void testSymmetryElementMirror() {
    std::string symbolPlain("m");
    std::string symbolGlide("c");
    V3R axis(0, 0, 1);
    V3R translation = V3R(0, 0, 1) / 2;

    SymmetryElementMirror defaultTranslation(symbolPlain, axis);
    TS_ASSERT_EQUALS(defaultTranslation.hmSymbol(), symbolPlain);
    TS_ASSERT_EQUALS(defaultTranslation.getAxis(), axis);
    TS_ASSERT_EQUALS(defaultTranslation.getTranslation(), V3R(0, 0, 0));

    SymmetryElementMirror mirrorElement(symbolGlide, axis, translation);
    TS_ASSERT_EQUALS(mirrorElement.hmSymbol(), symbolGlide);
    TS_ASSERT_EQUALS(mirrorElement.getAxis(), axis);
    TS_ASSERT_EQUALS(mirrorElement.getTranslation(), translation);

    SymmetryElement_sptr cloned = mirrorElement.clone();
    SymmetryElementMirror_sptr castedClone =
        boost::dynamic_pointer_cast<SymmetryElementMirror>(cloned);
    TS_ASSERT(castedClone);

    TS_ASSERT_EQUALS(castedClone->hmSymbol(), symbolGlide);
    TS_ASSERT_EQUALS(castedClone->getAxis(), axis);
    TS_ASSERT_EQUALS(castedClone->getTranslation(), translation);
  }

private:
  class MockSymmetryElement : public SymmetryElement {
    friend class SymmetryElementTest;

  public:
    MockSymmetryElement(const std::string &hmSymbol)
        : SymmetryElement(hmSymbol) {}
    MOCK_CONST_METHOD0(clone, SymmetryElement_sptr());
  };

  class MockSymmetryElementWithAxis : public SymmetryElementWithAxis {
    friend class SymmetryElementTest;

  public:
    MockSymmetryElementWithAxis(const std::string &symbol, const V3R &axis,
                                const V3R &translation)
        : SymmetryElementWithAxis(symbol, axis, translation) {}
    MOCK_CONST_METHOD0(clone, SymmetryElement_sptr());
  };
};

#endif /* MANTID_GEOMETRY_SYMMETRYELEMENTTEST_H_ */
