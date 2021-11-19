// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#ifdef _MSC_VER
// 'identifier' : class 'type' needs to have dll-interface to be used by clients
// of class 'type2'
#pragma warning(disable : 4251)
// JSON: non-DLL-interface classkey 'identifier' used as base for
// DLL-interface classkey 'identifier'
#pragma warning(disable : 4275)
#endif

#include <cxxtest/TestSuite.h>
#include <json/json.h>

#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidJson/Json.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidKernel/V3D.h"

using Mantid::DataObjects::PeakShapeSpherical;
using namespace Mantid::Kernel;

class PeakShapeSphericalTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakShapeSphericalTest *createSuite() { return new PeakShapeSphericalTest(); }
  static void destroySuite(PeakShapeSphericalTest *suite) { delete suite; }

  void test_constructor() {
    const double radius = 2;
    const SpecialCoordinateSystem frame = SpecialCoordinateSystem::HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical shape(radius, frame, algorithmName, algorithmVersion);

    TS_ASSERT_EQUALS(radius, shape.radius());
    TS_ASSERT_EQUALS(frame, shape.frame());
    TS_ASSERT_EQUALS(algorithmName, shape.algorithmName());
    TS_ASSERT_EQUALS(algorithmVersion, shape.algorithmVersion());
    TS_ASSERT(!shape.backgroundInnerRadius().has_value());
    TS_ASSERT(!shape.backgroundOuterRadius().has_value());
  }

  void test_multiple_radii_constructor() {
    const double radius = 2;
    const double backgroundInnerRadius = 3;
    const double backgroundOuterRadius = 4;
    const SpecialCoordinateSystem frame = SpecialCoordinateSystem::HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical shape(radius, backgroundInnerRadius, backgroundOuterRadius, frame, algorithmName,
                             algorithmVersion);

    TS_ASSERT_EQUALS(radius, shape.radius());
    TS_ASSERT_EQUALS(radius, shape.radius(Mantid::Geometry::PeakShape::RadiusType::Radius));
    TS_ASSERT_EQUALS(backgroundInnerRadius, shape.radius(Mantid::Geometry::PeakShape::RadiusType::InnerRadius));
    TS_ASSERT_EQUALS(backgroundOuterRadius, shape.radius(Mantid::Geometry::PeakShape::RadiusType::OuterRadius));

    TS_ASSERT_EQUALS(frame, shape.frame());
    TS_ASSERT_EQUALS(algorithmName, shape.algorithmName());
    TS_ASSERT_EQUALS(algorithmVersion, shape.algorithmVersion());
    TS_ASSERT_EQUALS(backgroundInnerRadius, shape.backgroundInnerRadius().value());
    TS_ASSERT_EQUALS(backgroundOuterRadius, shape.backgroundOuterRadius().value());

    PeakShapeSpherical badShape(radius, radius, radius, frame, algorithmName, algorithmVersion);

    TSM_ASSERT("Background inner radius should be set even when same as radius",
               badShape.backgroundInnerRadius().has_value());
    TSM_ASSERT("Background outer radius should be unset since is same as radius",
               !badShape.backgroundOuterRadius().has_value());
  }

  void test_copy_constructor() {
    const double radius = 2;
    const double backgroundInnerRadius = 3;
    const double backgroundOuterRadius = 4;
    const SpecialCoordinateSystem frame = SpecialCoordinateSystem::HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical a(radius, backgroundInnerRadius, backgroundOuterRadius, frame, algorithmName, algorithmVersion);
    // Copy construct it
    PeakShapeSpherical b(a);

    TS_ASSERT_EQUALS(radius, b.radius());
    TS_ASSERT_EQUALS(frame, b.frame());
    TS_ASSERT_EQUALS(algorithmName, b.algorithmName());
    TS_ASSERT_EQUALS(algorithmVersion, b.algorithmVersion());
    TS_ASSERT_EQUALS(backgroundInnerRadius, b.backgroundInnerRadius().value());
    TS_ASSERT_EQUALS(backgroundOuterRadius, b.backgroundOuterRadius().value());
  }

  void test_assignment() {
    const double radius = 2;
    const double backgroundInnerRadius = 3;
    const double backgroundOuterRadius = 4;
    const SpecialCoordinateSystem frame = SpecialCoordinateSystem::HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical a(radius, backgroundInnerRadius, backgroundOuterRadius, frame, algorithmName, algorithmVersion);
    PeakShapeSpherical b(1.0, QSample, "bar", -2);

    // Assign to it
    b = a;

    // Test the assignments
    TS_ASSERT_EQUALS(a.radius(), b.radius());
    TS_ASSERT_EQUALS(a.frame(), b.frame());
    TS_ASSERT_EQUALS(a.algorithmName(), b.algorithmName());
    TS_ASSERT_EQUALS(a.algorithmVersion(), b.algorithmVersion());
    TS_ASSERT_EQUALS(a.backgroundInnerRadius(), b.backgroundInnerRadius().value());
    TS_ASSERT_EQUALS(a.backgroundOuterRadius(), b.backgroundOuterRadius().value());
  }

  void test_clone() {
    const double radius = 2;
    const double backgroundInnerRadius = 3;
    const double backgroundOuterRadius = 4;
    const SpecialCoordinateSystem frame = SpecialCoordinateSystem::HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical a(radius, backgroundInnerRadius, backgroundOuterRadius, frame, algorithmName, algorithmVersion);
    PeakShapeSpherical *clone = a.clone();

    TS_ASSERT_EQUALS(a.radius(), clone->radius());
    TS_ASSERT_EQUALS(a.frame(), clone->frame());
    TS_ASSERT_EQUALS(a.algorithmName(), clone->algorithmName());
    TS_ASSERT_EQUALS(a.algorithmVersion(), clone->algorithmVersion());
    TS_ASSERT_EQUALS(a.backgroundInnerRadius(), clone->backgroundInnerRadius().value());
    TS_ASSERT_EQUALS(a.backgroundOuterRadius(), clone->backgroundOuterRadius().value());
    TS_ASSERT_DIFFERS(clone, &a);
    delete clone;
  }

  void test_toJSON() {
    const double radius = 2;
    const SpecialCoordinateSystem frame = SpecialCoordinateSystem::HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical shape(radius, frame, algorithmName, algorithmVersion);
    const std::string json = shape.toJSON();

    Json::Value output;
    TSM_ASSERT("Should parse as JSON", Mantid::JsonHelpers::parse(json, &output));

    TS_ASSERT_EQUALS(algorithmName, output["algorithm_name"].asString());
    TS_ASSERT_EQUALS(algorithmVersion, output["algorithm_version"].asInt());
    TS_ASSERT_EQUALS(frame, output["frame"].asInt());
    TS_ASSERT_EQUALS(radius, output["radius"].asDouble());
  }

  void test_toJSON_multiple_radii() {
    const double radius = 2;
    const double backgroundInnerRadius = 3;
    const double backgroundOuterRadius = 4;
    const SpecialCoordinateSystem frame = SpecialCoordinateSystem::HKL;
    const std::string algorithmName = "foo";
    const int algorithmVersion = 3;

    // Construct it.
    PeakShapeSpherical shape(radius, backgroundInnerRadius, backgroundOuterRadius, frame, algorithmName,
                             algorithmVersion);
    const std::string json = shape.toJSON();

    Json::Value output;
    TSM_ASSERT("Should parse as JSON", Mantid::JsonHelpers::parse(json, &output));

    TS_ASSERT_EQUALS(algorithmName, output["algorithm_name"].asString());
    TS_ASSERT_EQUALS(algorithmVersion, output["algorithm_version"].asInt());
    TS_ASSERT_EQUALS(frame, output["frame"].asInt());
    TS_ASSERT_EQUALS(radius, output["radius"].asDouble());
    TS_ASSERT_EQUALS(backgroundInnerRadius, output["background_inner_radius"].asDouble());
    TS_ASSERT_EQUALS(backgroundOuterRadius, output["background_outer_radius"].asDouble());
  }

  void test_equals() {
    TS_ASSERT_EQUALS(PeakShapeSpherical(1.0, QSample), PeakShapeSpherical(1.0, QSample));

    TS_ASSERT_EQUALS(PeakShapeSpherical(1.0, 2.0, 3.0, QSample), PeakShapeSpherical(1.0, 2.0, 3.0, QSample));

    TSM_ASSERT_DIFFERS("Different radius", PeakShapeSpherical(1.0, QSample), PeakShapeSpherical(2.0, QSample));

    TSM_ASSERT_DIFFERS("Different frame", PeakShapeSpherical(1.0, QSample), PeakShapeSpherical(1.0, QLab));

    TSM_ASSERT_DIFFERS("Different background inner", PeakShapeSpherical(1.0, 1.0, 3.0, QSample),
                       PeakShapeSpherical(1.0, 2.0, 3.0, QSample));

    TSM_ASSERT_DIFFERS("Different background outer", PeakShapeSpherical(1.0, 2.0, 2.0, QSample),
                       PeakShapeSpherical(1.0, 2.0, 3.0, QSample));
  }

  void test_shape_name() {

    const double radius = 1;
    const SpecialCoordinateSystem frame = SpecialCoordinateSystem::HKL;

    // Construct it.
    PeakShapeSpherical shape(radius, frame);

    TS_ASSERT_EQUALS("spherical", shape.shapeName());
  }
};
