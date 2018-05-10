#ifndef MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDFACTORYTEST_H_
#define MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDFACTORYTEST_H_

#ifdef _MSC_VER
// 'identifier' : class 'type' needs to have dll-interface to be used by clients
// of class 'type2'
#pragma warning(disable : 4251)
// JSON: non-DLL-interface classkey 'identifier' used as base for
// DLL-interface classkey 'identifier'
#pragma warning(disable : 4275)
#endif

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <json/json.h>

#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/VMD.h"
#include "MantidKernel/cow_ptr.h"
#include "MockObjects.h"

#include "MantidDataObjects/PeakShapeEllipsoidFactory.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::Kernel::SpecialCoordinateSystem;

class PeakShapeEllipsoidFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakShapeEllipsoidFactoryTest *createSuite() {
    return new PeakShapeEllipsoidFactoryTest();
  }
  static void destroySuite(PeakShapeEllipsoidFactoryTest *suite) {
    delete suite;
  }

  void test_invalid_json_with_no_successor() {
    PeakShapeEllipsoidFactory factory;
    TS_ASSERT_THROWS(factory.create(""), std::invalid_argument &);
  }

  void test_use_successor_when_different_shape_found() {
    using namespace testing;

    // We expect it to try to use the deletate factory. If it cannot process the
    // json.
    MockPeakShapeFactory *delegate = new MockPeakShapeFactory;
    EXPECT_CALL(*delegate, create(_)).Times(1);

    PeakShapeEllipsoidFactory factory;
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

    auto directions = {V3D(1, 0, 0), V3D(0, 1, 0), V3D(0, 0, 1)};
    const MantidVec abcRadii = {2, 3, 4};
    const MantidVec abcInnerRadii = {5, 6, 7};
    const MantidVec abcOuterRadii = {8, 9, 10};
    const SpecialCoordinateSystem frame = Mantid::Kernel::HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Make a source shape
    PeakShapeEllipsoid sourceShape(directions, abcRadii, abcInnerRadii,
                                   abcOuterRadii, frame, algorithmName,
                                   algorithmVersion);

    PeakShapeEllipsoidFactory factory;
    Mantid::Geometry::PeakShape *productShape =
        factory.create(sourceShape.toJSON());

    PeakShapeEllipsoid *ellipsoidShapeProduct =
        dynamic_cast<PeakShapeEllipsoid *>(productShape);
    TS_ASSERT(ellipsoidShapeProduct);

    TS_ASSERT_EQUALS(sourceShape, *ellipsoidShapeProduct);
    delete productShape;
  }
};

#endif /* MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDFACTORYTEST_H_ */
