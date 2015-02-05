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

  void testSymmetryElementWithAxisSetAxis() {
    MockSymmetryElementWithAxis element;

    V3R invalidAxis(0, 0, 0);
    TS_ASSERT_THROWS(element.setAxis(invalidAxis), std::invalid_argument);

    V3R validAxis(1, 0, 0);
    TS_ASSERT_THROWS_NOTHING(element.setAxis(validAxis));

    TS_ASSERT_EQUALS(element.getAxis(), validAxis);
  }

  void testSymmetryElementWithAxisSetTranslation() {
    MockSymmetryElementWithAxis element;

    V3R validAxis(1, 0, 0);
    TS_ASSERT_THROWS_NOTHING(element.setTranslation(validAxis));

    TS_ASSERT_EQUALS(element.getTranslation(), validAxis);
  }

  void testSymmetryElementWithAxisSetFixPoint() {
    MockSymmetryElementWithAxis element;

    V3R validAxis(1, 0, 0);
    TS_ASSERT_THROWS_NOTHING(element.setFixPoint(validAxis));

    TS_ASSERT_EQUALS(element.getFixPoint(), validAxis);
  }

  void testSymmetryElementWithAxisDetermineTranslation() {
    MockSymmetryElementWithAxis element;

    V3R screwVectorOneHalf = V3R(0, 0, 1) / 2;
    SymmetryOperation twoOneScrew("-x,-y,z+1/2");
    TS_ASSERT_EQUALS(element.determineTranslation(twoOneScrew),
                     screwVectorOneHalf);

    V3R screwVectorOneThird = V3R(0, 0, 1) / 3;
    SymmetryOperation threeOneScrew("-y,x-y,z+1/3");
    TS_ASSERT_EQUALS(element.determineTranslation(threeOneScrew),
                     screwVectorOneThird);

    V3R screwVectorTwoThirds = V3R(0, 0, 2) / 3;
    SymmetryOperation threeTwoScrew("-y,x-y,z+2/3");
    TS_ASSERT_EQUALS(element.determineTranslation(threeTwoScrew),
                     screwVectorTwoThirds);

    V3R glideVectorC = V3R(0, 0, 1) / 2;
    SymmetryOperation glidePlaneC("x,-y,z+1/2");
    TS_ASSERT_EQUALS(element.determineTranslation(glidePlaneC), glideVectorC);
  }

  void testGetGSLMatrix() {
    IntMatrix mantidMatrix(3, 3, true);
    gsl_matrix *matrix = getGSLMatrix(mantidMatrix);

    TS_ASSERT(matrix);
    TS_ASSERT_EQUALS(matrix->size1, mantidMatrix.numRows());
    TS_ASSERT_EQUALS(matrix->size2, mantidMatrix.numCols());

    for (size_t r = 0; r < mantidMatrix.numRows(); ++r) {
      for (size_t c = 0; c < mantidMatrix.numCols(); ++c) {
        TS_ASSERT_EQUALS(gsl_matrix_get(matrix, r, c), mantidMatrix[r][c]);
      }
    }

    gsl_matrix_free(matrix);
  }

  void testGetGSLIdentityMatrix() {
    gsl_matrix *matrix = getGSLIdentityMatrix(3, 3);

    TS_ASSERT_EQUALS(matrix->size1, 3);
    TS_ASSERT_EQUALS(matrix->size2, 3);

    gsl_matrix_free(matrix);
  }

  void testSymmetryElementWithAxisDetermineAxis() {
    MockSymmetryElementWithAxis element;

    V3R rotationAxisZ = V3R(0, 0, 1);
    SymmetryOperation twoFoldRotationZ("-x,-y,z");
    TS_ASSERT_EQUALS(element.determineAxis(twoFoldRotationZ.matrix()),
                     rotationAxisZ);
  }

private:
  class MockSymmetryElement : public SymmetryElement {
    friend class SymmetryElementTest;

  public:
    MOCK_METHOD1(init, void(const SymmetryOperation &operation));
  };

  class MockSymmetryElementWithAxis : public SymmetryElementWithAxis {
    friend class SymmetryElementTest;

  public:
    MOCK_METHOD1(init, void(const SymmetryOperation &operation));
  };
};

#endif /* MANTID_GEOMETRY_SYMMETRYELEMENTTEST_H_ */
