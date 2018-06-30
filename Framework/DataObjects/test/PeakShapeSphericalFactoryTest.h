#ifndef MANTID_DATAOBJECTS_PEAKSHAPESPHERICALFACTORYTEST_H_
#define MANTID_DATAOBJECTS_PEAKSHAPESPHERICALFACTORYTEST_H_

#ifdef _MSC_VER
// 'identifier' : class 'type' needs to have dll-interface to be used by clients
// of class 'type2'
#pragma warning(disable : 4251)
// JSON: non- DLL-interface classkey 'identifier' used as base for
// DLL-interface classkey 'identifier'
#pragma warning(disable : 4275)
#endif

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <json/json.h>

#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeakShapeSphericalFactory.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/VMD.h"
#include "MockObjects.h"

using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class PeakShapeSphericalFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakShapeSphericalFactoryTest *createSuite() {
    return new PeakShapeSphericalFactoryTest();
  }
  static void destroySuite(PeakShapeSphericalFactoryTest *suite) {
    delete suite;
  }

  void test_invalid_json_with_no_successor() {
    PeakShapeSphericalFactory factory;
    TS_ASSERT_THROWS(factory.create(""), std::invalid_argument &);
  }

  void test_use_successor_when_different_shape_found() {
    using namespace testing;

    // We expect it to try to use the deletate factory. If it cannot process the
    // json.
    MockPeakShapeFactory *delegate = new MockPeakShapeFactory;
    EXPECT_CALL(*delegate, create(_)).Times(1);

    PeakShapeSphericalFactory factory;
    factory.setSuccessor(PeakShapeFactory_const_sptr(delegate));

    // Minimal valid JSON for describing the shape.
    Json::Value root;
    root["shape"] = "square";
    Json::StyledWriter writer;
    const std::string str_json = writer.write(root);

    factory.create(str_json);

    TS_ASSERT(Mock::VerifyAndClearExpectations(delegate));
  }

  void test_create() {
    const double radius = 2;
    const SpecialCoordinateSystem frame = HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Make a source shape
    PeakShapeSpherical sourceShape(radius, frame, algorithmName,
                                   algorithmVersion);

    PeakShapeSphericalFactory factory;
    Mantid::Geometry::PeakShape *productShape =
        factory.create(sourceShape.toJSON());

    PeakShapeSpherical *sphericalShapeProduct =
        dynamic_cast<PeakShapeSpherical *>(productShape);
    TS_ASSERT(sphericalShapeProduct);

    TS_ASSERT_EQUALS(sourceShape, *sphericalShapeProduct);
    delete productShape;
  }

  void test_create_with_multiple_radii() {
    const double radius = 2;
    const double backgroundInnerRadius = 3;
    const double backgroundOuterRadius = 4;
    const SpecialCoordinateSystem frame = HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Make a source shape with background outer and inner radius
    PeakShapeSpherical sourceShape(radius, backgroundInnerRadius,
                                   backgroundOuterRadius, frame, algorithmName,
                                   algorithmVersion);

    PeakShapeSphericalFactory factory;
    Mantid::Geometry::PeakShape *productShape =
        factory.create(sourceShape.toJSON());

    PeakShapeSpherical *sphericalShapeProduct =
        dynamic_cast<PeakShapeSpherical *>(productShape);
    TS_ASSERT(sphericalShapeProduct);

    TS_ASSERT_EQUALS(sourceShape, *sphericalShapeProduct);
    delete productShape;
  }
};

#endif /* MANTID_DATAOBJECTS_PEAKSHAPESPHERICALFACTORYTEST_H_ */
