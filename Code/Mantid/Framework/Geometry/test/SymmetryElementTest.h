#ifndef MANTID_GEOMETRY_SYMMETRYELEMENTTEST_H_
#define MANTID_GEOMETRY_SYMMETRYELEMENTTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidGeometry/Crystal/SymmetryElement.h"

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

  void testHMSymbolGetSet() {
    MockSymmetryElement element;

    TS_ASSERT_EQUALS(element.hmSymbol(), "");

    TS_ASSERT_THROWS_NOTHING(element.setHMSymbol("SomeSymbol"));
    TS_ASSERT_EQUALS(element.hmSymbol(), "SomeSymbol");
  }

  void testSymmetryElementIdentity() {
    TS_ASSERT_THROWS_NOTHING(SymmetryElementIdentity identity);

    SymmetryOperation identityOperation("x,y,z");

    /* SymmetryElementIdentity can only be initialized with the identity
     * operation x,y,z. All others operations throw std::invalid_argument
     */
    SymmetryElementIdentity identityElement;
    TS_ASSERT_THROWS_NOTHING(identityElement.init(identityOperation));
    TS_ASSERT_EQUALS(identityElement.hmSymbol(), "1");

    SymmetryOperation mirrorZ("x,y,-z");
    TS_ASSERT_THROWS(identityElement.init(mirrorZ), std::invalid_argument);
  }

  void testSymmetryElementInversion() {
    TS_ASSERT_THROWS_NOTHING(SymmetryElementInversion inversion);

    SymmetryOperation inversionOperation("-x,-y,-z");

    /* SymmetryElementInversion can only be initialized with the inversion
     * operation -x,-y,-z. All others operations throw std::invalid_argument
     */
    SymmetryElementInversion inversionElement;
    TS_ASSERT_THROWS_NOTHING(inversionElement.init(inversionOperation));
    TS_ASSERT_EQUALS(inversionElement.hmSymbol(), "-1");
    TS_ASSERT_EQUALS(inversionElement.getInversionPoint(), V3R(0, 0, 0));

    SymmetryOperation shiftedInversion("-x+1/4,-y+1/4,-z+1/4");
    TS_ASSERT_THROWS_NOTHING(inversionElement.init(shiftedInversion));

    // The operation shifts the inversion center to 1/8, 1/8, 1/8
    V3R inversionPoint = V3R(1, 1, 1) / 8;
    TS_ASSERT_EQUALS(inversionElement.getInversionPoint(), inversionPoint);

    SymmetryOperation mirrorZ("x,y,-z");
    TS_ASSERT_THROWS(inversionElement.init(mirrorZ), std::invalid_argument);
  }

private:
  class MockSymmetryElement : public SymmetryElement {
    friend class SymmetryElementTest;

  public:
    MOCK_METHOD1(init, void(const SymmetryOperation &operation));
  };
};

#endif /* MANTID_GEOMETRY_SYMMETRYELEMENTTEST_H_ */
