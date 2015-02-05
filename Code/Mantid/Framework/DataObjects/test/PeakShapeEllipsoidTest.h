#ifndef MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDTEST_H_
#define MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/V3D.h"
#include <vector>
#include <boost/assign/list_of.hpp>
#include <jsoncpp/json/json.h>

using Mantid::DataObjects::PeakShapeEllipsoid;
using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace boost::assign;

class PeakShapeEllipsoidTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakShapeEllipsoidTest *createSuite() {
    return new PeakShapeEllipsoidTest();
  }
  static void destroySuite(PeakShapeEllipsoidTest *suite) { delete suite; }

  void test_constructor() {
    auto directions = list_of(V3D(1, 0, 0))(V3D(0, 1, 0))(V3D(0, 0, 1))
                          .convert_to_container<std::vector<V3D>>();
    const MantidVec abcRadii = list_of(2)(3)(4);
    const MantidVec abcInnerRadii = list_of(5)(6)(7);
    const MantidVec abcOuterRadii = list_of(8)(9)(10);
    const SpecialCoordinateSystem frame = HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeEllipsoid shape(directions, abcRadii, abcInnerRadii, abcOuterRadii,
                             frame, algorithmName, algorithmVersion);

    TS_ASSERT_EQUALS(abcRadii, shape.abcRadii());
    TS_ASSERT_EQUALS(abcInnerRadii, shape.abcRadiiBackgroundInner());
    TS_ASSERT_EQUALS(abcOuterRadii, shape.abcRadiiBackgroundOuter());

    TS_ASSERT_EQUALS(frame, shape.frame());
    TS_ASSERT_EQUALS(algorithmName, shape.algorithmName());
    TS_ASSERT_EQUALS(algorithmVersion, shape.algorithmVersion());
  }

  void test_constructor_throws() {
    auto directions = list_of(V3D(1, 0, 0))(V3D(0, 1, 0))(V3D(0, 0, 1))
                          .convert_to_container<std::vector<V3D>>();
    auto bad_directions =
        list_of(V3D(1, 0, 0)).convert_to_container<std::vector<V3D>>();
    const MantidVec abcRadii = list_of(2)(3)(4);
    const MantidVec bad_abcRadii = list_of(2)(3)(4)(5);
    const MantidVec abcInnerRadii = list_of(5)(6)(7);
    const MantidVec bad_abcInnerRadii = list_of(5)(6);
    const MantidVec abcOuterRadii = list_of(8)(9)(10);
    const MantidVec bad_abcOuterRadii = list_of(8)(9)(10)(11);
    const SpecialCoordinateSystem frame = HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    TSM_ASSERT_THROWS("Should throw, bad directions",
                      PeakShapeEllipsoid(bad_directions, abcRadii,
                                         abcInnerRadii, abcOuterRadii, frame),
                      std::invalid_argument &);
    TSM_ASSERT_THROWS("Should throw, bad radii",
                      PeakShapeEllipsoid(directions, bad_abcRadii,
                                         abcInnerRadii, abcOuterRadii, frame),
                      std::invalid_argument &);
    TSM_ASSERT_THROWS("Should throw, bad inner radii",
                      PeakShapeEllipsoid(directions, abcRadii,
                                         bad_abcInnerRadii, abcOuterRadii,
                                         frame),
                      std::invalid_argument &);
    TSM_ASSERT_THROWS("Should throw, bad outer radii",
                      PeakShapeEllipsoid(directions, abcRadii, abcInnerRadii,
                                         bad_abcOuterRadii, frame),
                      std::invalid_argument &);
  }

  void test_copy_constructor() {
    auto directions = list_of(V3D(1, 0, 0))(V3D(0, 1, 0))(V3D(0, 0, 1))
                          .convert_to_container<std::vector<V3D>>();
    const MantidVec abcRadii = list_of(2)(3)(4);
    const MantidVec abcInnerRadii = list_of(5)(6)(7);
    const MantidVec abcOuterRadii = list_of(8)(9)(10);
    const SpecialCoordinateSystem frame = HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeEllipsoid a(directions, abcRadii, abcInnerRadii, abcOuterRadii,
                         frame, algorithmName, algorithmVersion);

    PeakShapeEllipsoid b(a);
    TS_ASSERT_EQUALS(abcRadii, b.abcRadii());
    TS_ASSERT_EQUALS(abcInnerRadii, b.abcRadiiBackgroundInner());
    TS_ASSERT_EQUALS(abcOuterRadii, b.abcRadiiBackgroundOuter());

    TS_ASSERT_EQUALS(frame, b.frame());
    TS_ASSERT_EQUALS(algorithmName, b.algorithmName());
    TS_ASSERT_EQUALS(algorithmVersion, b.algorithmVersion());
  }

  void test_assignment() {
    PeakShapeEllipsoid a(list_of(V3D(1, 0, 0))(V3D(0, 1, 0))(V3D(0, 0, 1))
                             .convert_to_container<std::vector<V3D>>(),
                         list_of(2)(3)(4), list_of(5)(6)(7), list_of(8)(9)(10),
                         HKL, "foo", 1);

    PeakShapeEllipsoid b(list_of(V3D(0, 0, 0))(V3D(0, 1, 0))(V3D(0, 0, 1))
                             .convert_to_container<std::vector<V3D>>(),
                         list_of(1)(3)(4), list_of(1)(6)(7), list_of(8)(9)(10),
                         QLab, "bar", 2);

    b = a;

    TS_ASSERT_EQUALS(a.abcRadii(), b.abcRadii());
    TS_ASSERT_EQUALS(a.abcRadiiBackgroundInner(), b.abcRadiiBackgroundInner());
    TS_ASSERT_EQUALS(a.abcRadiiBackgroundOuter(), b.abcRadiiBackgroundOuter());

    TS_ASSERT_EQUALS(a.frame(), b.frame());
    TS_ASSERT_EQUALS(a.algorithmName(), b.algorithmName());
    TS_ASSERT_EQUALS(a.algorithmVersion(), b.algorithmVersion());
  }

  void test_shape_name() {

    const double radius = 1;
    const SpecialCoordinateSystem frame = HKL;

    // Construct it.
    PeakShapeEllipsoid shape(list_of(V3D(1, 0, 0))(V3D(0, 1, 0))(V3D(0, 0, 1))
                                 .convert_to_container<std::vector<V3D>>(),
                             list_of(2)(3)(4), list_of(5)(6)(7),
                             list_of(8)(9)(10), HKL, "foo", 1);

    TS_ASSERT_EQUALS("ellipsoid", shape.shapeName());
  }

  void test_toJSON() {

      auto directions = list_of(V3D(1, 0, 0))(V3D(0, 1, 0))(V3D(0, 0, 1))
                            .convert_to_container<std::vector<V3D>>();
      const MantidVec abcRadii = list_of(2)(3)(4);
      const MantidVec abcInnerRadii = list_of(5)(6)(7);
      const MantidVec abcOuterRadii = list_of(8)(9)(10);
      const SpecialCoordinateSystem frame = HKL;
      const std::string algorithmName = "foo";
      const int algorithmVersion = 3;

      // Construct it.
      PeakShapeEllipsoid shape(directions, abcRadii, abcInnerRadii, abcOuterRadii,
                               frame, algorithmName, algorithmVersion);

    const std::string json = shape.toJSON();

    Json::Reader reader;
    Json::Value output;
    TSM_ASSERT("Should parse as JSON", reader.parse(json, output));

    TS_ASSERT_EQUALS(directions[0].toString(), output["direction0"].asString());
    TS_ASSERT_EQUALS(directions[1].toString(), output["direction1"].asString());
    TS_ASSERT_EQUALS(directions[2].toString(), output["direction2"].asString());
    TS_ASSERT_EQUALS(algorithmName, output["algorithm_name"].asString());
    TS_ASSERT_EQUALS(algorithmVersion, output["algorithm_version"].asInt());
    TS_ASSERT_EQUALS(frame, output["frame"].asInt());
    TS_ASSERT_EQUALS(abcRadii[0], output["radius0"].asDouble());
    TS_ASSERT_EQUALS(abcRadii[1], output["radius1"].asDouble());
    TS_ASSERT_EQUALS(abcRadii[2], output["radius2"].asDouble());
    TS_ASSERT_EQUALS(abcOuterRadii[0], output["background_outer_radius0"].asDouble());
    TS_ASSERT_EQUALS(abcOuterRadii[1], output["background_outer_radius1"].asDouble());
    TS_ASSERT_EQUALS(abcOuterRadii[2], output["background_outer_radius2"].asDouble());

  }



};

#endif /* MANTID_DATAOBJECTS_PEAKSHAPEELLIPSOIDTEST_H_ */
