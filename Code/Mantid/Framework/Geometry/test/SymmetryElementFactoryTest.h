#ifndef MANTID_GEOMETRY_SYMMETRYELEMENTFACTORYTEST_H_
#define MANTID_GEOMETRY_SYMMETRYELEMENTFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidGeometry/Crystal/SymmetryElementFactory.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class SymmetryElementFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SymmetryElementFactoryTest *createSuite() {
    return new SymmetryElementFactoryTest();
  }
  static void destroySuite(SymmetryElementFactoryTest *suite) { delete suite; }

  void testSymmetryElementIdentityGenerator() {
    // This generator processes Identity.
    SymmetryOperation identity("x,y,z");

    SymmetryElementIdentityGenerator identityGenerator;
    TS_ASSERT(identityGenerator.canProcess(identity));
    TS_ASSERT_THROWS_NOTHING(identityGenerator.generateElement(identity));

    SymmetryElement_sptr identityElement =
        identityGenerator.generateElement(identity);

    TS_ASSERT(identityElement);
    TS_ASSERT_EQUALS(identityElement->hmSymbol(), "1");

    SymmetryElementIdentity_sptr castedElement =
        boost::dynamic_pointer_cast<SymmetryElementIdentity>(identityElement);
    TS_ASSERT(castedElement);

    // But not other operations.
    SymmetryOperation inversion("-x,-y,-z");
    TS_ASSERT(!identityGenerator.canProcess(inversion));

    SymmetryOperation translation("x+1/2,y+1/2,z");
    TS_ASSERT(!identityGenerator.canProcess(translation));
  }

  void testSymmetryElementTranslationGenerator() {
    // This generator processes Translations.
    SymmetryOperation bodyCentering("x+1/2,y+1/2,z+1/2");

    SymmetryElementTranslationGenerator translationGenerator;
    TS_ASSERT(translationGenerator.canProcess(bodyCentering));
    TS_ASSERT_THROWS_NOTHING(
        translationGenerator.generateElement(bodyCentering));

    SymmetryElement_sptr translationElement =
        translationGenerator.generateElement(bodyCentering);

    TS_ASSERT(translationElement);
    TS_ASSERT_EQUALS(translationElement->hmSymbol(), "t");

    SymmetryElementTranslation_sptr castedElement =
        boost::dynamic_pointer_cast<SymmetryElementTranslation>(
            translationElement);
    TS_ASSERT(castedElement);
    TS_ASSERT_EQUALS(castedElement->getTranslation(), V3R(1, 1, 1) / 2);

    // But not other operations.
    SymmetryOperation inversion("-x,-y,-z");
    TS_ASSERT(!translationGenerator.canProcess(inversion));

    SymmetryOperation identity("x,y,z");
    TS_ASSERT(!translationGenerator.canProcess(identity));
  }

  void testSymmetryElementInversionGenerator() {
    // This generator processes Inversion.
    SymmetryOperation inversion("-x,-y,-z");

    SymmetryElementInversionGenerator inversionGenerator;
    TS_ASSERT(inversionGenerator.canProcess(inversion));
    TS_ASSERT_THROWS_NOTHING(inversionGenerator.generateElement(inversion));

    SymmetryElement_sptr inversionElement =
        inversionGenerator.generateElement(inversion);

    TS_ASSERT(inversionElement);
    TS_ASSERT_EQUALS(inversionElement->hmSymbol(), "-1");

    // But not other operations.
    SymmetryOperation identity("x,y,z");
    TS_ASSERT(!inversionGenerator.canProcess(identity));

    SymmetryOperation translation("x+1/2,y+1/2,z");
    TS_ASSERT(!inversionGenerator.canProcess(translation));

    // Inversion can also be at another point
    SymmetryOperation shiftedInversion("-x+1/4,-y+1/4,-z+1/4");
    SymmetryElement_sptr shiftedElement =
        inversionGenerator.generateElement(shiftedInversion);

    SymmetryElementInversion_sptr castedElement =
        boost::dynamic_pointer_cast<SymmetryElementInversion>(shiftedElement);

    TS_ASSERT(castedElement);
    TS_ASSERT_EQUALS(castedElement->getInversionPoint(), V3R(1, 1, 1) / 8);
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

  void testSymmetryElementWithAxisGeneratorDetermineAxis() {
    TestableSymmetryElementWithAxisGenerator generator;

    V3R rotationAxisZ = V3R(0, 0, 1);
    SymmetryOperation fourFoldRotoInversionZ("y,-x,-z");
    TS_ASSERT_EQUALS(generator.determineAxis(fourFoldRotoInversionZ.matrix()),
                     rotationAxisZ);

    SymmetryOperation sixFoldRotationZ("-y,x-y,z");
    TS_ASSERT_EQUALS(generator.determineAxis(sixFoldRotationZ.matrix()),
                     rotationAxisZ);

    V3R rotationAxisY = V3R(0, 1, 0);
    SymmetryOperation glideMirrorCY("x,-y,z+1/2");
    TS_ASSERT_EQUALS(generator.determineAxis(glideMirrorCY.matrix()),
                     rotationAxisY);

    V3R rotationAxisXYZ = V3R(1, 1, 1);
    SymmetryOperation threeFoldRation111("z,x,y");
    TS_ASSERT_EQUALS(generator.determineAxis(threeFoldRation111.matrix()),
                     rotationAxisXYZ);

    V3R rotationAxisxyZ = V3R(1, -1, -1);
    SymmetryOperation threeFoldRationmm1("-z,-x,y");
    TS_ASSERT_EQUALS(generator.determineAxis(threeFoldRationmm1.matrix()),
                     rotationAxisxyZ);

    V3R rotoInversionAxisxYz = V3R(-1, 1, -1);
    SymmetryOperation threeFoldRotoInversionm1mPlus("-z,x,y");
    TS_ASSERT_EQUALS(
        generator.determineAxis(threeFoldRotoInversionm1mPlus.matrix()),
        rotoInversionAxisxYz);

    V3R rotationAxis2xx0 = V3R(2, 1, 0);
    SymmetryOperation twoFoldRotationHex210("x,x-y,-z");
    TS_ASSERT_EQUALS(generator.determineAxis(twoFoldRotationHex210.matrix()),
                     rotationAxis2xx0);

    V3R rotationAxisx2x0 = V3R(1, 2, 0);
    SymmetryOperation twoFoldRotationHex120("y-x,y,-z");
    TS_ASSERT_EQUALS(generator.determineAxis(twoFoldRotationHex120.matrix()),
                     rotationAxisx2x0);
  }

  void testSymmetryElementWithAxisGeneratorDetermineTranslation() {
    TestableSymmetryElementWithAxisGenerator generator;

    V3R screwVectorOneHalf = V3R(0, 0, 1) / 2;
    SymmetryOperation twoOneScrew("-x,-y,z+1/2");
    TS_ASSERT_EQUALS(generator.determineTranslation(twoOneScrew),
                     screwVectorOneHalf);

    V3R screwVectorOneThird = V3R(0, 0, 1) / 3;
    SymmetryOperation threeOneScrew("-y,x-y,z+1/3");
    TS_ASSERT_EQUALS(generator.determineTranslation(threeOneScrew),
                     screwVectorOneThird);

    V3R screwVectorTwoThirds = V3R(0, 0, 2) / 3;
    SymmetryOperation threeTwoScrew("-y,x-y,z+2/3");
    TS_ASSERT_EQUALS(generator.determineTranslation(threeTwoScrew),
                     screwVectorTwoThirds);

    V3R glideVectorC = V3R(0, 0, 1) / 2;
    SymmetryOperation glidePlaneC("x,-y,z+1/2");
    TS_ASSERT_EQUALS(generator.determineTranslation(glidePlaneC), glideVectorC);
  }

  void testSymmetryElementRotationDetermineRotationSense() {
    TestableSymmetryElementRotationGenerator generator;

    // Test case 1: 3 [-1 1 -1] (Positive/Negative) in orthogonal system
    SymmetryOperation threeFoldRotoInversionm1mPlus("-z,x,y");
    V3R rotationAxism1m =
        generator.determineAxis(threeFoldRotoInversionm1mPlus.matrix());
    TS_ASSERT_EQUALS(generator.determineRotationSense(
                         threeFoldRotoInversionm1mPlus, rotationAxism1m),
                     SymmetryElementRotation::Positive);

    SymmetryOperation threeFoldRotoInversionm1mMinus("y,z,-x");
    V3R rotationAxism1m2 =
        generator.determineAxis(threeFoldRotoInversionm1mPlus.matrix());

    TS_ASSERT_EQUALS(rotationAxism1m, rotationAxism1m2);

    TS_ASSERT_EQUALS(generator.determineRotationSense(
                         threeFoldRotoInversionm1mMinus, rotationAxism1m2),
                     SymmetryElementRotation::Negative);

    // Test case 2: 6 [0 0 1] (Positive/Negative) in hexagonal system
    SymmetryOperation sixFoldRotationZPlus("x-y,x,z");
    V3R rotationAxisZ = generator.determineAxis(sixFoldRotationZPlus.matrix());
    TS_ASSERT_EQUALS(
        generator.determineRotationSense(sixFoldRotationZPlus, rotationAxisZ),
        SymmetryElementRotation::Positive);

    SymmetryOperation sixFoldRotationZMinus("y,y-x,z");
    V3R rotationAxisZ2 =
        generator.determineAxis(sixFoldRotationZMinus.matrix());

    TS_ASSERT_EQUALS(rotationAxisZ, rotationAxisZ2);

    TS_ASSERT_EQUALS(
        generator.determineRotationSense(sixFoldRotationZMinus, rotationAxisZ2),
        SymmetryElementRotation::Negative);
  }

  void testSymmetryElementRotationDetermineSymbol() {
    TestableSymmetryElementRotationGenerator generator;

    SymmetryOperation sixFoldRotationZMinus("y,y-x,z");
    TS_ASSERT_EQUALS(generator.determineSymbol(sixFoldRotationZMinus), "6");

    SymmetryOperation fourThreeScrewAxis("x+3/4,z+1/4,-y+3/4");
    TS_ASSERT_EQUALS(generator.determineSymbol(fourThreeScrewAxis), "43");

    SymmetryOperation threeFoldRotoInversion("-z+1/4,-x+1/4,-y+1/4");
    TS_ASSERT_EQUALS(generator.determineSymbol(threeFoldRotoInversion), "-3");

    SymmetryOperation twoOneScrewAxis("-x+1/2,y+1/2,-z");
    TS_ASSERT_EQUALS(generator.determineSymbol(twoOneScrewAxis), "21");
  }

  void testSymmetryElementRotationGenerator() {
    // This generator processes Rotations/Rotoinversions.
    SymmetryOperation rotation("x+3/4,z+1/4,-y+3/4");

    SymmetryElementRotationGenerator rotationGenerator;
    TS_ASSERT(rotationGenerator.canProcess(rotation));
    TS_ASSERT_THROWS_NOTHING(rotationGenerator.generateElement(rotation));

    SymmetryElement_sptr rotationElement =
        rotationGenerator.generateElement(rotation);

    TS_ASSERT(rotationElement);
    TS_ASSERT_EQUALS(rotationElement->hmSymbol(), "43");

    SymmetryElementRotation_sptr castedElement =
        boost::dynamic_pointer_cast<SymmetryElementRotation>(rotationElement);

    TS_ASSERT(castedElement);
    TS_ASSERT_EQUALS(castedElement->getRotationSense(),
                     SymmetryElementRotation::Negative);
    TS_ASSERT_EQUALS(castedElement->getAxis(), V3R(1, 0, 0));
    TS_ASSERT_EQUALS(castedElement->getTranslation(), V3R(3, 0, 0) / 4);

    // But not other operations.
    SymmetryOperation identity("x,y,z");
    TS_ASSERT(!rotationGenerator.canProcess(identity));

    SymmetryOperation translation("x+1/2,y+1/2,z");
    TS_ASSERT(!rotationGenerator.canProcess(translation));
  }

  void testSymmetryElementMirrorDetermineSymbol() {
    TestableSymmetryElementMirrorGenerator generator;

    SymmetryOperation dGlide("x+1/4,y+3/4,-z+3/4");
    TS_ASSERT_EQUALS(generator.determineSymbol(dGlide), "d");

    SymmetryOperation gGlide("x+1/2,-z+1/2,-y");
    TS_ASSERT_EQUALS(generator.determineSymbol(gGlide), "g");

    SymmetryOperation mirror("y,x,z");
    TS_ASSERT_EQUALS(generator.determineSymbol(mirror), "m");
  }

  void testSymmetryElementMirrorGenerator() {
    // This generator processes Mirrors/Glides.
    SymmetryOperation mirror("x+1/4,y+3/4,-z+3/4");

    SymmetryElementMirrorGenerator mirrorGenerator;
    TS_ASSERT(mirrorGenerator.canProcess(mirror));
    TS_ASSERT_THROWS_NOTHING(mirrorGenerator.generateElement(mirror));

    SymmetryElement_sptr mirrorElement =
        mirrorGenerator.generateElement(mirror);

    TS_ASSERT(mirrorElement);
    TS_ASSERT_EQUALS(mirrorElement->hmSymbol(), "d");

    SymmetryElementMirror_sptr castedElement =
        boost::dynamic_pointer_cast<SymmetryElementMirror>(mirrorElement);

    TS_ASSERT(castedElement);
    TS_ASSERT_EQUALS(castedElement->getAxis(), V3R(0, 0, 1));
    TS_ASSERT_EQUALS(castedElement->getTranslation(), V3R(1, 3, 0) / 4);

    // But not other operations.
    SymmetryOperation identity("x,y,z");
    TS_ASSERT(!mirrorGenerator.canProcess(identity));

    SymmetryOperation translation("x+1/2,y+1/2,z");
    TS_ASSERT(!mirrorGenerator.canProcess(translation));
  }

  void testSymmetryElementFactoryInstantiation() {
    TS_ASSERT_THROWS_NOTHING(SymmetryElementFactory::Instance());
  }

  void testSymmetryElementFactorySubscribe() {
    TestableSymmetryElementFactory factory;
    TS_ASSERT(!factory.isSubscribed("SymmetryElementMirrorGenerator"));

    TS_ASSERT_THROWS_NOTHING(factory.subscribeSymmetryElementGenerator<
        TestableSymmetryElementMirrorGenerator>(
        "SymmetryElementMirrorGenerator"));

    TS_ASSERT(factory.isSubscribed("SymmetryElementMirrorGenerator"));

    TS_ASSERT_THROWS(factory.subscribeSymmetryElementGenerator<
                         TestableSymmetryElementMirrorGenerator>(
                         "SymmetryElementMirrorGenerator"),
                     std::runtime_error);
  }

  void testSymmetryElementFactoryCreateSymElem() {
    SymmetryOperation mirror("x,y,-z");

    TestableSymmetryElementFactory factory;
    factory.subscribeSymmetryElementGenerator<
        TestableSymmetryElementMirrorGenerator>(
        "SymmetryElementMirrorGenerator");

    // There is no prototype yet.
    TS_ASSERT(!factory.createFromPrototype(mirror.identifier()));

    // But an appropriate generator has been registered.
    AbstractSymmetryElementGenerator_sptr generator =
        factory.getGenerator(mirror);
    TS_ASSERT(generator);

    // It's really the correct one.
    boost::shared_ptr<SymmetryElementMirrorGenerator> castedGenerator =
        boost::dynamic_pointer_cast<SymmetryElementMirrorGenerator>(generator);
    TS_ASSERT(castedGenerator);

    // Now we can create the corresponding element
    SymmetryElement_sptr mirrorElement = factory.createSymElement(mirror);

    // Make sure it's correct.
    TS_ASSERT(mirrorElement);
    TS_ASSERT_EQUALS(mirrorElement->hmSymbol(), "m");

    // At this point we have a prototype stored
    SymmetryElement_sptr anotherMirror =
        factory.createFromPrototype(mirror.identifier());
    TS_ASSERT(anotherMirror);

    // It should also be a mirror.
    TS_ASSERT_EQUALS(anotherMirror->hmSymbol(), "m");
  }

private:
  class TestableSymmetryElementWithAxisGenerator
      : public SymmetryElementWithAxisGenerator {
    friend class SymmetryElementFactoryTest;

    MOCK_CONST_METHOD1(generateElement,
                       SymmetryElement_sptr(const SymmetryOperation &));
    MOCK_CONST_METHOD1(canProcess, bool(const SymmetryOperation &));
    MOCK_CONST_METHOD1(determineSymbol, std::string(const SymmetryOperation &));
  };

  class TestableSymmetryElementRotationGenerator
      : public SymmetryElementRotationGenerator {
    friend class SymmetryElementFactoryTest;
  };

  class TestableSymmetryElementMirrorGenerator
      : public SymmetryElementMirrorGenerator {
    friend class SymmetryElementFactoryTest;
  };

  class TestableSymmetryElementFactory : public SymmetryElementFactoryImpl {
    friend class SymmetryElementFactoryTest;
  };
};

#endif /* MANTID_GEOMETRY_SYMMETRYELEMENTFACTORYTEST_H_ */
