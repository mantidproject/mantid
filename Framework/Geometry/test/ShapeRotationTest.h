// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/MeshObject2D.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/ShapeRotation.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/V3D.h"

#include <cxxtest/TestSuite.h>

#include <cmath>
#include <memory>
#include <string>

using namespace Mantid;
using namespace Mantid::Geometry;
using Mantid::Kernel::Material;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::V3D;

namespace {

const Matrix<double> IDENTITY(3, 3, true);
constexpr double PI = 3.14159265358979323846;

/// Rotation by the given angle about z, so a point on +x moves towards +y.
Matrix<double> rotationZ(const double degrees) {
  const double radians = degrees * PI / 180.0;
  const double c = std::cos(radians);
  const double s = std::sin(radians);
  Matrix<double> rotation(3, 3);
  rotation[0][0] = c;
  rotation[0][1] = -s;
  rotation[0][2] = 0.0;
  rotation[1][0] = s;
  rotation[1][1] = c;
  rotation[1][2] = 0.0;
  rotation[2][0] = 0.0;
  rotation[2][1] = 0.0;
  rotation[2][2] = 1.0;
  return rotation;
}

Material testMaterial() { return Material("testium", PhysicalConstants::getNeutronAtom(23, 0), 0.072); }

/// A sphere well away from the origin, so any rotation visibly moves it.
std::string offsetSphereXML(const std::string &extraTags = "") {
  return "<sphere id=\"offset\">"
         "<centre x=\"2.0\" y=\"0.0\" z=\"0.0\"/>"
         "<radius val=\"0.5\"/>"
         "</sphere>" +
         extraTags;
}

std::shared_ptr<CSGObject> createOffsetSphere(const std::string &extraTags = "") {
  auto shape = ShapeFactory().createShape(offsetSphereXML(extraTags));
  shape->setMaterial(testMaterial());
  return shape;
}

/// Serialise a matrix into the attribute form the goniometer tags use.
std::string matrixTag(const std::string &name, const Matrix<double> &matrix) {
  std::string tag = "<" + name;
  for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < 3; ++j) {
      tag += " a" + std::to_string(i + 1) + std::to_string(j + 1) + "=\"" + std::to_string(matrix[i][j]) + "\"";
    }
  }
  return tag + "/>";
}

std::unique_ptr<MeshObject> createCube(const double size) {
  const double h = 0.5 * size;
  std::vector<V3D> vertices{V3D(h, h, h),  V3D(-h, h, h),  V3D(h, -h, h),  V3D(-h, -h, h),
                            V3D(h, h, -h), V3D(-h, h, -h), V3D(h, -h, -h), V3D(-h, -h, -h)};
  std::vector<uint32_t> triangles{0, 1, 2, 2, 1, 3, 0, 2, 4, 4, 2, 6, 0, 4, 1, 1, 4, 5,
                                  7, 5, 6, 6, 5, 4, 7, 3, 5, 5, 3, 1, 7, 6, 3, 3, 6, 2};
  return std::make_unique<MeshObject>(std::move(triangles), std::move(vertices), testMaterial());
}

/// A flat plate, which has no rotation mechanism at all.
std::unique_ptr<MeshObject2D> createFlatPlate() {
  std::vector<V3D> vertices{V3D(0, 0, 0), V3D(1, 0, 0), V3D(1, 1, 0), V3D(0, 1, 0)};
  std::vector<uint32_t> triangles{0, 1, 2, 0, 2, 3};
  return std::make_unique<MeshObject2D>(std::move(triangles), std::move(vertices), testMaterial());
}

void assertMatrixEquals(const Matrix<double> &actual, const Matrix<double> &expected, const double tolerance = 1e-12) {
  for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < 3; ++j) {
      TS_ASSERT_DELTA(actual[i][j], expected[i][j], tolerance);
    }
  }
}

} // namespace

class ShapeRotationTest : public CxxTest::TestSuite {
public:
  static ShapeRotationTest *createSuite() { return new ShapeRotationTest(); }
  static void destroySuite(ShapeRotationTest *suite) { delete suite; }

  // ---------------------------------------------------------------------------------------------
  // outstandingGoniometerRotation
  // ---------------------------------------------------------------------------------------------

  void test_outstanding_is_the_whole_goniometer_for_a_shape_in_its_own_frame() {
    const auto rotation = rotationZ(90.0);

    const auto csgShape = createOffsetSphere();
    TS_ASSERT_EQUALS(csgShape->getAppliedRotation(), IDENTITY);
    assertMatrixEquals(outstandingGoniometerRotation(*csgShape, rotation), rotation);

    const auto meshShape = createCube(2.0);
    assertMatrixEquals(outstandingGoniometerRotation(*meshShape, rotation), rotation);
  }

  void test_outstanding_is_identity_for_a_shape_already_in_the_lab_frame() {
    const auto rotation = rotationZ(90.0);

    // A legacy <goniometer> tag with no <applied-goniometer> beside it is read as a full bake.
    const auto csgShape = createOffsetSphere(matrixTag("goniometer", rotation));
    assertMatrixEquals(csgShape->getAppliedRotation(), rotation, 1e-6);
    assertMatrixEquals(outstandingGoniometerRotation(*csgShape, csgShape->getAppliedRotation()), IDENTITY);

    auto meshShape = createCube(2.0);
    meshShape->bakeGoniometerRotation(rotation);
    assertMatrixEquals(outstandingGoniometerRotation(*meshShape, rotation), IDENTITY);
  }

  void test_outstanding_is_the_remainder_when_partially_baked() {
    const auto baked = rotationZ(30.0);
    const auto goniometer = rotationZ(90.0);

    auto meshShape = createCube(2.0);
    meshShape->bakeGoniometerRotation(baked);

    // What is left is the 60 degrees between them, not the difference of the angles by luck -
    // assert against R * B^T explicitly as well as against the equivalent single rotation.
    assertMatrixEquals(outstandingGoniometerRotation(*meshShape, goniometer), goniometer * baked.Tprime());
    assertMatrixEquals(outstandingGoniometerRotation(*meshShape, goniometer), rotationZ(60.0));
  }

  void test_a_definition_frame_rotation_does_not_count_as_baked() {
    // rotate() re-expresses the shape within its own frame, so the whole goniometer is still
    // outstanding. This is the distinction the marker exists to make.
    const auto rotation = rotationZ(90.0);
    auto meshShape = createCube(2.0);
    meshShape->rotate(rotationZ(45.0));

    TS_ASSERT_EQUALS(meshShape->getAppliedRotation(), IDENTITY);
    assertMatrixEquals(outstandingGoniometerRotation(*meshShape, rotation), rotation);
  }

  // ---------------------------------------------------------------------------------------------
  // getLabFrameShape - mesh
  // ---------------------------------------------------------------------------------------------

  void test_lab_frame_mesh_shape_is_rotated_and_marked() {
    const auto rotation = rotationZ(90.0);
    const auto meshShape = createCube(2.0);
    const auto original = meshShape->getV3Ds();

    const auto labShape = getLabFrameShape(*meshShape, rotation);
    assertMatrixEquals(labShape->getAppliedRotation(), rotation);

    const auto rotated = std::dynamic_pointer_cast<MeshObject>(labShape)->getV3Ds();
    TS_ASSERT_EQUALS(rotated.size(), original.size());
    for (size_t i = 0; i < original.size(); ++i) {
      const V3D expected = rotation * original[i];
      TS_ASSERT_DELTA(rotated[i].X(), expected.X(), 1e-12);
      TS_ASSERT_DELTA(rotated[i].Y(), expected.Y(), 1e-12);
      TS_ASSERT_DELTA(rotated[i].Z(), expected.Z(), 1e-12);
    }
  }

  void test_lab_frame_mesh_shape_leaves_the_source_untouched() {
    const auto rotation = rotationZ(90.0);
    const auto meshShape = createCube(2.0);
    const auto before = meshShape->getV3Ds();

    const auto labShape = getLabFrameShape(*meshShape, rotation);

    TS_ASSERT_EQUALS(meshShape->getAppliedRotation(), IDENTITY);
    TS_ASSERT_EQUALS(meshShape->getV3Ds(), before);
  }

  void test_lab_frame_mesh_shape_is_unrotated_when_already_baked() {
    const auto rotation = rotationZ(90.0);
    auto meshShape = createCube(2.0);
    meshShape->bakeGoniometerRotation(rotation);
    const auto baked = meshShape->getV3Ds();

    const auto labShape = getLabFrameShape(*meshShape, rotation);

    // Bit-for-bit, not merely close: rotating by R then R^T would perturb the last bits for nothing.
    TS_ASSERT_EQUALS(std::dynamic_pointer_cast<MeshObject>(labShape)->getV3Ds(), baked);
    assertMatrixEquals(labShape->getAppliedRotation(), rotation);
  }

  void test_lab_frame_mesh_shape_applies_only_the_remainder() {
    const auto baked = rotationZ(30.0);
    const auto goniometer = rotationZ(90.0);
    auto meshShape = createCube(2.0);
    meshShape->bakeGoniometerRotation(baked);
    const auto partial = meshShape->getV3Ds();

    const auto labShape = getLabFrameShape(*meshShape, goniometer);

    // The result reports the full goniometer, but the vertices moved by only the outstanding 60.
    assertMatrixEquals(labShape->getAppliedRotation(), goniometer);
    const auto rotated = std::dynamic_pointer_cast<MeshObject>(labShape)->getV3Ds();
    for (size_t i = 0; i < partial.size(); ++i) {
      const V3D expected = rotationZ(60.0) * partial[i];
      TS_ASSERT_DELTA(rotated[i].X(), expected.X(), 1e-12);
      TS_ASSERT_DELTA(rotated[i].Y(), expected.Y(), 1e-12);
      TS_ASSERT_DELTA(rotated[i].Z(), expected.Z(), 1e-12);
    }
  }

  // ---------------------------------------------------------------------------------------------
  // getLabFrameShape - CSG
  // ---------------------------------------------------------------------------------------------

  void test_lab_frame_csg_shape_is_rotated_and_marked() {
    const auto rotation = rotationZ(90.0);
    const auto csgShape = createOffsetSphere();

    // The sphere sits on +x to start with.
    TS_ASSERT(csgShape->isValid(V3D(2.0, 0.0, 0.0)));
    TS_ASSERT(!csgShape->isValid(V3D(0.0, 2.0, 0.0)));

    const auto labShape = getLabFrameShape(*csgShape, rotation);

    // and on +y afterwards.
    TS_ASSERT(!labShape->isValid(V3D(2.0, 0.0, 0.0)));
    TS_ASSERT(labShape->isValid(V3D(0.0, 2.0, 0.0)));
    assertMatrixEquals(labShape->getAppliedRotation(), rotation, 1e-6);
  }

  void test_lab_frame_csg_shape_leaves_the_source_untouched() {
    const auto rotation = rotationZ(90.0);
    const auto csgShape = createOffsetSphere();

    const auto labShape = getLabFrameShape(*csgShape, rotation);

    TS_ASSERT(csgShape->isValid(V3D(2.0, 0.0, 0.0)));
    TS_ASSERT_EQUALS(csgShape->getAppliedRotation(), IDENTITY);
  }

  void test_lab_frame_csg_shape_preserves_a_definition_frame_rotation() {
    // <goniometer> holds the total, <applied-goniometer> says none of it is a bake - so the 90
    // degrees is a rotation of the shape within its own frame and must survive the move to the
    // lab frame, leaving the sphere at 90 + 90 = 180 degrees, on -x.
    const auto definitionRotation = rotationZ(90.0);
    const auto goniometer = rotationZ(90.0);
    const auto csgShape =
        createOffsetSphere(matrixTag("goniometer", definitionRotation) + matrixTag("applied-goniometer", IDENTITY));

    TS_ASSERT_EQUALS(csgShape->getAppliedRotation(), IDENTITY);
    TS_ASSERT(csgShape->isValid(V3D(0.0, 2.0, 0.0)));

    const auto labShape = getLabFrameShape(*csgShape, goniometer);

    TS_ASSERT(labShape->isValid(V3D(-2.0, 0.0, 0.0)));
    TS_ASSERT(!labShape->isValid(V3D(0.0, 2.0, 0.0)));
    assertMatrixEquals(labShape->getAppliedRotation(), goniometer, 1e-6);
  }

  void test_lab_frame_csg_shape_is_a_plain_clone_when_already_baked() {
    const auto rotation = rotationZ(90.0);
    const auto csgShape = createOffsetSphere(matrixTag("goniometer", rotation));
    const auto baked = csgShape->getAppliedRotation();

    const auto labShape = getLabFrameShape(*csgShape, baked);

    TS_ASSERT_EQUALS(std::dynamic_pointer_cast<CSGObject>(labShape)->getShapeXML(), csgShape->getShapeXML());
    TS_ASSERT_EQUALS(labShape->getAppliedRotation(), baked);
  }

  // ---------------------------------------------------------------------------------------------
  // Material, and shapes that cannot be rotated
  // ---------------------------------------------------------------------------------------------

  void test_material_survives_the_move_to_the_lab_frame() {
    const auto rotation = rotationZ(90.0);

    const auto csgShape = createOffsetSphere();
    TS_ASSERT_EQUALS(getLabFrameShape(*csgShape, rotation)->material().name(), csgShape->material().name());

    const auto meshShape = createCube(2.0);
    TS_ASSERT_EQUALS(getLabFrameShape(*meshShape, rotation)->material().name(), meshShape->material().name());
  }

  void test_a_shape_that_cannot_be_rotated_is_returned_unchanged() {
    // MeshObject2D offers no way to express a rotation, so it is taken to be defined in the frame
    // it is meant to be used in. The caller gets it back as it stands, plus a warning in the log.
    const auto plate = createFlatPlate();
    const auto before = plate->getVertices();

    std::shared_ptr<IObject> labShape;
    TS_ASSERT_THROWS_NOTHING(labShape = getLabFrameShape(*plate, rotationZ(90.0)));
    TS_ASSERT(labShape);
    TS_ASSERT_EQUALS(std::dynamic_pointer_cast<MeshObject2D>(labShape)->getVertices(), before);
  }

  void test_a_shape_that_cannot_be_rotated_is_returned_when_nothing_is_outstanding() {
    const auto plate = createFlatPlate();
    std::shared_ptr<IObject> labShape;
    TS_ASSERT_THROWS_NOTHING(labShape = getLabFrameShape(*plate, IDENTITY));
    TS_ASSERT(labShape);
  }
};
