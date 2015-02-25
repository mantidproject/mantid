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

  void testSomething() {
      TS_ASSERT(false);
  }

/*
void xtestHMSymbolGetSet() {
  MockSymmetryElement element;

  TS_ASSERT_EQUALS(element.hmSymbol(), "");

  TS_ASSERT_THROWS_NOTHING(element.setHMSymbol("SomeSymbol"));
  TS_ASSERT_EQUALS(element.hmSymbol(), "SomeSymbol");
}


void xtestSymmetryElementIdentity() {
  TS_ASSERT_THROWS_NOTHING(SymmetryElementIdentity identity);

  SymmetryOperation identityOperation("x,y,z");

  /* SymmetryElementIdentity can only be initialized with the identity
   * operation x,y,z. All others operations throw std::invalid_argument
  SymmetryElementIdentity identityElement;
  TS_ASSERT_THROWS_NOTHING(identityElement.init(identityOperation));
  TS_ASSERT_EQUALS(identityElement.hmSymbol(), "1");

  SymmetryOperation mirrorZ("x,y,-z");
  TS_ASSERT_THROWS(identityElement.init(mirrorZ), std::invalid_argument);
}

void xtestSymmetryElementInversion() {
  TS_ASSERT_THROWS_NOTHING(SymmetryElementInversion inversion(V3R(0, 0, 0)));

  SymmetryOperation inversionOperation("-x,-y,-z");

  /* SymmetryElementInversion can only be initialized with the inversion
   * operation -x,-y,-z. All others operations throw std::invalid_argument
  SymmetryElementInversion inversionElement(V3R(0, 0, 0));
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

void xtestSymmetryElementWithAxisSetAxis() {
  MockSymmetryElementWithAxis element;

  V3R invalidAxis(0, 0, 0);
  TS_ASSERT_THROWS(element.setAxis(invalidAxis), std::invalid_argument);

  V3R validAxis(1, 0, 0);
  TS_ASSERT_THROWS_NOTHING(element.setAxis(validAxis));

  TS_ASSERT_EQUALS(element.getAxis(), validAxis);
}

void xtestSymmetryElementWithAxisSetTranslation() {
  MockSymmetryElementWithAxis element;

  V3R validAxis(1, 0, 0);
  TS_ASSERT_THROWS_NOTHING(element.setTranslation(validAxis));

  TS_ASSERT_EQUALS(element.getTranslation(), validAxis);
}

void xtestSymmetryElementWithAxisSpaceGroup() {
  TestableSymmetryElementRotation element;
  TestableSymmetryElementMirror mirror;

  SpaceGroup_const_sptr sg =
      SpaceGroupFactory::Instance().createSpaceGroup("I a -3 d");

  std::vector<SymmetryOperation> ops = sg->getSymmetryOperations();
  for (auto it = ops.begin(); it != ops.end(); ++it) {
    V3R axis = element.determineAxis((*it).matrix());
    SymmetryElementRotation::RotationSense sense =
        element.determineRotationSense(*it, axis);

   std::string sym = element.determineSymbol(*it);

    if(sym.substr(0,2) == "-2" && (*it).matrix().Trace() != -3) {

    std::cout << (*it).identifier() << ": " << (*it).order() << " "
              //<< pg->getReflectionFamily(axis) << " "
              << axis << " "
              << element.determineSymbol(*it)
              << (sense == SymmetryElementRotation::Positive ? "+" : "-")
              << " " << mirror.determineTranslation(*it)
              << " " << mirror.determineSymbol(*it)
              << std::endl;
    std::cout << (*it).matrix() << " " << (*it).matrix().determinant() <<
std::endl;
    }
  }
}
*/
private:
class MockSymmetryElement : public SymmetryElement {
  friend class SymmetryElementTest;

public:
  MockSymmetryElement() : SymmetryElement("") {}
  MOCK_CONST_METHOD0(clone, SymmetryElement_sptr());
};
};

#endif /* MANTID_GEOMETRY_SYMMETRYELEMENTTEST_H_ */
