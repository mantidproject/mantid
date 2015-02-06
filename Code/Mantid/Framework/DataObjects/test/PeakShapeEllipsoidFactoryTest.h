#ifndef MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDFACTORYTEST_H_
#define MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDFACTORYTEST_H_

#ifdef _WIN32
#pragma warning(disable : 4251)
#endif

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <jsoncpp/json/json.h>
#include <boost/assign/list_of.hpp>

#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidKernel/VMD.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidAPI/SpecialCoordinateSystem.h"
#include "MockObjects.h"

#include "MantidDataObjects/PeakShapeEllipsoidFactory.h"

using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace boost::assign;

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

    auto directions = list_of(V3D(1, 0, 0))(V3D(0, 1, 0))(V3D(0, 0, 1))
                          .convert_to_container<std::vector<V3D>>();
    const MantidVec abcRadii = list_of(2)(3)(4);
    const MantidVec abcInnerRadii = list_of(5)(6)(7);
    const MantidVec abcOuterRadii = list_of(8)(9)(10);
    const SpecialCoordinateSystem frame = HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Make a source shape
    PeakShapeEllipsoid sourceShape(directions, abcRadii, abcInnerRadii,
                                   abcOuterRadii, frame, algorithmName,
                                   algorithmVersion);

    PeakShapeEllipsoidFactory factory;
    Mantid::Geometry::PeakShape *productShape = factory.create(sourceShape.toJSON());

    PeakShapeEllipsoid *ellipsoidShapeProduct =
        dynamic_cast<PeakShapeEllipsoid *>(productShape);
    TS_ASSERT(ellipsoidShapeProduct);

    TS_ASSERT_EQUALS(sourceShape, *ellipsoidShapeProduct);
    delete productShape;
  }
};

#endif /* MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDFACTORYTEST_H_ */
