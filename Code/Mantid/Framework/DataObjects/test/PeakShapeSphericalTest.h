#ifndef MANTID_DATAOBJECTS_PEAKSHAPESPHERICALTEST_H_
#define MANTID_DATAOBJECTS_PEAKSHAPESPHERICALTEST_H_

#ifdef _WIN32
#pragma warning(disable : 4251)
#endif

#include <cxxtest/TestSuite.h>
#include <jsoncpp/json/json.h>

#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidKernel/V3D.h"
#include "MantidAPI/SpecialCoordinateSystem.h"

using Mantid::DataObjects::PeakShapeSpherical;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class PeakShapeSphericalTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakShapeSphericalTest *createSuite() {
    return new PeakShapeSphericalTest();
  }
  static void destroySuite(PeakShapeSphericalTest *suite) { delete suite; }

  void test_constructor() {
    const double radius = 2;
    const SpecialCoordinateSystem frame = HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical shape(radius, frame, algorithmName,
                             algorithmVersion);

    TS_ASSERT_EQUALS(radius, shape.radius());
    TS_ASSERT_EQUALS(frame, shape.frame());
    TS_ASSERT_EQUALS(algorithmName, shape.algorithmName());
    TS_ASSERT_EQUALS(algorithmVersion, shape.algorithmVersion());
    TS_ASSERT(!shape.backgroundInnerRadius().is_initialized());
    TS_ASSERT(!shape.backgroundOuterRadius().is_initialized());
  }

  void test_multiple_radii_constructor()
  {
      const double radius = 2;
      const double backgroundInnerRadius = 3;
      const double backgroundOuterRadius = 4;
      const SpecialCoordinateSystem frame = HKL;
      const std::string algorithmName = "foo";
      const int algorithmVersion = 3;

      // Construct it.
      PeakShapeSpherical shape(radius, backgroundInnerRadius, backgroundOuterRadius, frame, algorithmName,
                               algorithmVersion);

      TS_ASSERT_EQUALS(radius, shape.radius());
      TS_ASSERT_EQUALS(frame, shape.frame());
      TS_ASSERT_EQUALS(algorithmName, shape.algorithmName());
      TS_ASSERT_EQUALS(algorithmVersion, shape.algorithmVersion());
      TS_ASSERT_EQUALS(backgroundInnerRadius, shape.backgroundInnerRadius().get());
      TS_ASSERT_EQUALS(backgroundOuterRadius, shape.backgroundOuterRadius().get());

      PeakShapeSpherical badShape(radius, radius, radius, frame, algorithmName,
                                  algorithmVersion);

      TSM_ASSERT("Background inner radius should be unset since is same as radius", !badShape.backgroundInnerRadius().is_initialized());
      TSM_ASSERT("Background outer radius should be unset since is same as radius", !badShape.backgroundOuterRadius().is_initialized());
  }

  void test_copy_constructor() {
    const double radius = 2;
    const double backgroundInnerRadius = 3;
    const double backgroundOuterRadius = 4;
    const SpecialCoordinateSystem frame = HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical a(radius, backgroundInnerRadius, backgroundOuterRadius, frame, algorithmName,
                         algorithmVersion);
    // Copy construct it
    PeakShapeSpherical b(a);

    TS_ASSERT_EQUALS(radius, b.radius());
    TS_ASSERT_EQUALS(frame, b.frame());
    TS_ASSERT_EQUALS(algorithmName, b.algorithmName());
    TS_ASSERT_EQUALS(algorithmVersion, b.algorithmVersion());
    TS_ASSERT_EQUALS(backgroundInnerRadius, b.backgroundInnerRadius().get());
    TS_ASSERT_EQUALS(backgroundOuterRadius, b.backgroundOuterRadius().get());
  }

  void test_assignment() {
    const double radius = 2;
    const double backgroundInnerRadius = 3;
    const double backgroundOuterRadius = 4;
    const SpecialCoordinateSystem frame = HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical a(radius, backgroundInnerRadius, backgroundOuterRadius, frame, algorithmName,
                         algorithmVersion);
    PeakShapeSpherical b(1.0, QSample, "bar", -2);

    // Assign to it
    b = a;

    // Test the assignments
    TS_ASSERT_EQUALS(a.radius(), b.radius());
    TS_ASSERT_EQUALS(a.frame(), b.frame());
    TS_ASSERT_EQUALS(a.algorithmName(), b.algorithmName());
    TS_ASSERT_EQUALS(a.algorithmVersion(), b.algorithmVersion());
    TS_ASSERT_EQUALS(a.backgroundInnerRadius(), b.backgroundInnerRadius().get());
    TS_ASSERT_EQUALS(a.backgroundOuterRadius(), b.backgroundOuterRadius().get());
  }

  void test_clone() {
    const double radius = 2;
    const double backgroundInnerRadius = 3;
    const double backgroundOuterRadius = 4;
    const SpecialCoordinateSystem frame = HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical a(radius, backgroundInnerRadius, backgroundOuterRadius, frame, algorithmName,
                         algorithmVersion);
    PeakShapeSpherical *clone = a.clone();

    TS_ASSERT_EQUALS(a.radius(), clone->radius());
    TS_ASSERT_EQUALS(a.frame(), clone->frame());
    TS_ASSERT_EQUALS(a.algorithmName(), clone->algorithmName());
    TS_ASSERT_EQUALS(a.algorithmVersion(), clone->algorithmVersion());
    TS_ASSERT_EQUALS(a.backgroundInnerRadius(), clone->backgroundInnerRadius().get());
    TS_ASSERT_EQUALS(a.backgroundOuterRadius(), clone->backgroundOuterRadius().get());
    TS_ASSERT_DIFFERS(clone, &a);
  }

  void test_toJSON() {
    const double radius = 2;
    const SpecialCoordinateSystem frame = HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical shape(radius, frame, algorithmName,
                             algorithmVersion);
    const std::string json = shape.toJSON();

    Json::Reader reader;
    Json::Value output;
    TSM_ASSERT("Should parse as JSON", reader.parse(json, output));

    TS_ASSERT_EQUALS(algorithmName, output["algorithm_name"].asString());
    TS_ASSERT_EQUALS(algorithmVersion, output["algorithm_version"].asInt());
    TS_ASSERT_EQUALS(frame, output["frame"].asInt());
    TS_ASSERT_EQUALS(radius, output["radius"].asDouble());
  }

  void test_toJSON_multiple_radii()
  {
      const double radius = 2;
      const double backgroundInnerRadius = 3;
      const double backgroundOuterRadius = 4;
      const SpecialCoordinateSystem frame = HKL;
      const std::string algorithmName = "foo";
      const int algorithmVersion = 3;

      // Construct it.
      PeakShapeSpherical shape(radius, backgroundInnerRadius, backgroundOuterRadius, frame, algorithmName,
                               algorithmVersion);
      const std::string json = shape.toJSON();

      Json::Reader reader;
      Json::Value output;
      TSM_ASSERT("Should parse as JSON", reader.parse(json, output));

      TS_ASSERT_EQUALS(algorithmName, output["algorithm_name"].asString());
      TS_ASSERT_EQUALS(algorithmVersion, output["algorithm_version"].asInt());
      TS_ASSERT_EQUALS(frame, output["frame"].asInt());
      TS_ASSERT_EQUALS(radius, output["radius"].asDouble());
      TS_ASSERT_EQUALS(backgroundInnerRadius, output["background_inner_radius"].asDouble());
      TS_ASSERT_EQUALS(backgroundOuterRadius, output["background_outer_radius"].asDouble());
  }

  void test_equals() {
    TS_ASSERT_EQUALS(PeakShapeSpherical(1.0, QSample),
                     PeakShapeSpherical(1.0, QSample));

    TS_ASSERT_EQUALS(PeakShapeSpherical(1.0,2.0,3.0, QSample),
                     PeakShapeSpherical(1.0,2.0,3.0, QSample));

    TSM_ASSERT_DIFFERS("Different radius",
                       PeakShapeSpherical(1.0, QSample),
                       PeakShapeSpherical(2.0, QSample));

    TSM_ASSERT_DIFFERS("Different frame",
                       PeakShapeSpherical(1.0, QSample),
                       PeakShapeSpherical(1.0, QLab));

    TSM_ASSERT_DIFFERS("Different background inner",
                       PeakShapeSpherical(1.0, 1.0, 3.0, QSample),
                       PeakShapeSpherical(1.0, 2.0, 3.0, QSample));

    TSM_ASSERT_DIFFERS("Different background outer",
                       PeakShapeSpherical(1.0, 2.0, 2.0,  QSample),
                       PeakShapeSpherical(1.0, 2.0, 3.0, QSample));
  }

  void test_shape_name() {

    const double radius = 1;
    const SpecialCoordinateSystem frame = HKL;

    // Construct it.
    PeakShapeSpherical shape(radius, frame);

    TS_ASSERT_EQUALS("spherical", shape.shapeName());
  }
};

#endif /* MANTID_DATAOBJECTS_PEAKSHAPESPHERICALTEST_H_ */
