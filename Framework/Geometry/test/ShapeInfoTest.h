// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

using Mantid::Kernel::V3D;
using namespace Mantid::Geometry::detail;

class ShapeInfoTest : public CxxTest::TestSuite {
public:
  void testConstructEmptyInitiazesEverythingZero() {
    ShapeInfo shapeInfo;
    TS_ASSERT(shapeInfo.points().size() == 0);
    TS_ASSERT(shapeInfo.height() == 0);
    TS_ASSERT(shapeInfo.radius() == 0);
    TS_ASSERT_EQUALS(shapeInfo.innerRadius(), 0);
    TS_ASSERT(shapeInfo.shape() == ShapeInfo::GeometryShape::NOSHAPE);
  }

  void testSetSphere() {
    ShapeInfo shapeInfo;
    constexpr V3D center(0, 0, 0);
    constexpr double radius = 10;
    shapeInfo.setSphere(center, radius);

    TS_ASSERT_EQUALS(shapeInfo.shape(), ShapeInfo::GeometryShape::SPHERE);
    TS_ASSERT_EQUALS(shapeInfo.radius(), radius);
    TS_ASSERT_EQUALS(shapeInfo.height(), 0);
    TS_ASSERT_EQUALS(shapeInfo.innerRadius(), 0);
    TS_ASSERT_EQUALS(shapeInfo.points().size(), 1);
    TS_ASSERT_EQUALS(shapeInfo.points()[0], center);
  }

  void testSetCuboid() {
    ShapeInfo shapeInfo;
    constexpr V3D p1(0, 0, 0);
    constexpr V3D p2(0, 0, 1);
    constexpr V3D p3(0, 1, 0);
    constexpr V3D p4(0, 1, 1);

    shapeInfo.setCuboid(p1, p2, p3, p4);

    TS_ASSERT_EQUALS(shapeInfo.shape(), ShapeInfo::GeometryShape::CUBOID);
    TS_ASSERT_EQUALS(shapeInfo.radius(), 0);
    TS_ASSERT_EQUALS(shapeInfo.height(), 0);
    TS_ASSERT_EQUALS(shapeInfo.innerRadius(), 0);
    TS_ASSERT_EQUALS(shapeInfo.points().size(), 4);
    TS_ASSERT_EQUALS(shapeInfo.points(), (std::vector<V3D>{p1, p2, p3, p4}));
  }

  void testSetHexahedron() {
    ShapeInfo shapeInfo;
    constexpr V3D p1(0, 0, 0);
    constexpr V3D p2(0, 0, 1);
    constexpr V3D p3(0, 1, 0);
    constexpr V3D p4(0, 1, 1);
    constexpr V3D p5(1, 0, 0);
    constexpr V3D p6(1, 0, 1);
    constexpr V3D p7(1, 1, 0);
    constexpr V3D p8(1, 1, 1);

    shapeInfo.setHexahedron(p1, p2, p3, p4, p5, p6, p7, p8);

    TS_ASSERT_EQUALS(shapeInfo.shape(), ShapeInfo::GeometryShape::HEXAHEDRON);
    TS_ASSERT_EQUALS(shapeInfo.radius(), 0);
    TS_ASSERT_EQUALS(shapeInfo.height(), 0);
    TS_ASSERT_EQUALS(shapeInfo.innerRadius(), 0);
    TS_ASSERT_EQUALS(shapeInfo.points().size(), 8);
    TS_ASSERT_EQUALS(shapeInfo.points(),
                     (std::vector<V3D>{p1, p2, p3, p4, p5, p6, p7, p8}));
  }

  void testSetHollowCylinder() {
    ShapeInfo shapeInfo;
    constexpr V3D centerBottomBase(0, 0, 0);
    constexpr V3D symmetryAxis(1, 1, 1);
    constexpr double innerRadius = 5;
    constexpr double outerRadius = 6;
    constexpr double height = 3;
    shapeInfo.setHollowCylinder(centerBottomBase, symmetryAxis, innerRadius,
                                outerRadius, height);

    TS_ASSERT_EQUALS(shapeInfo.shape(),
                     ShapeInfo::GeometryShape::HOLLOWCYLINDER);
    TS_ASSERT_EQUALS(shapeInfo.points().size(), 2);
    TS_ASSERT_EQUALS(shapeInfo.points()[0], centerBottomBase);
    TS_ASSERT_EQUALS(shapeInfo.points()[1], symmetryAxis);
    TS_ASSERT_EQUALS(shapeInfo.innerRadius(), innerRadius);
    TS_ASSERT_EQUALS(shapeInfo.radius(), outerRadius);
    TS_ASSERT_EQUALS(shapeInfo.height(), height);
  }

  void testSetCone() {
    ShapeInfo shapeInfo;
    constexpr V3D center(0, 0, 0);
    constexpr V3D axis(1, 0, 0);
    constexpr double radius = 10;
    constexpr double height = 5;
    shapeInfo.setCone(center, axis, radius, height);

    TS_ASSERT_EQUALS(shapeInfo.shape(), ShapeInfo::GeometryShape::CONE);
    TS_ASSERT_EQUALS(shapeInfo.radius(), radius);
    TS_ASSERT_EQUALS(shapeInfo.innerRadius(), 0);
    TS_ASSERT_EQUALS(shapeInfo.height(), height);
    TS_ASSERT_EQUALS(shapeInfo.points().size(), 2);
    TS_ASSERT_EQUALS(shapeInfo.points()[0], center);
    TS_ASSERT_EQUALS(shapeInfo.points()[1], axis);
  }

  void testSetCylinder() {
    ShapeInfo shapeInfo;
    constexpr V3D center(0, 0, 0);
    constexpr V3D axis(1, 0, 0);
    constexpr double radius = 10;
    constexpr double height = 5;
    shapeInfo.setCylinder(center, axis, radius, height);

    TS_ASSERT_EQUALS(shapeInfo.shape(), ShapeInfo::GeometryShape::CYLINDER);
    TS_ASSERT_EQUALS(shapeInfo.radius(), radius);
    TS_ASSERT_EQUALS(shapeInfo.height(), height);
    TS_ASSERT_EQUALS(shapeInfo.innerRadius(), 0);
    TS_ASSERT_EQUALS(shapeInfo.points().size(), 2);
    TS_ASSERT_EQUALS(shapeInfo.points()[0], center);
    TS_ASSERT_EQUALS(shapeInfo.points()[1], axis);
  }

  void testGetObjectGeometry() {
    ShapeInfo shapeInfo;
    constexpr V3D center(0, 0, 0);
    constexpr double radius = 10;
    shapeInfo.setSphere(center, radius);

    ShapeInfo::GeometryShape testShape;
    std::vector<V3D> testPoints;
    double testHeight;
    double testRadius;
    double testInnerRadius;

    shapeInfo.getObjectGeometry(testShape, testPoints, testInnerRadius,
                                testRadius, testHeight);
    TS_ASSERT_EQUALS(testRadius, radius);
    TS_ASSERT_EQUALS(testHeight, 0);
    TS_ASSERT_EQUALS(testInnerRadius, 0);
    TS_ASSERT(testPoints.size() == 1);
    TS_ASSERT_EQUALS(testPoints[0], center);
    TS_ASSERT_EQUALS(testShape, ShapeInfo::GeometryShape::SPHERE);
  }

  void testGetObjectGeometryForCylinders() {
    ShapeInfo shapeInfo;
    constexpr V3D centreBottomBase(0, 0, 0);
    constexpr V3D symmetryAxis(0, 1, 1);
    constexpr double innerRadius = 1;
    constexpr double outerRadius = 2;
    constexpr double height = 5;
    shapeInfo.setHollowCylinder(centreBottomBase, symmetryAxis, innerRadius,
                                outerRadius, height);

    ShapeInfo::GeometryShape testShape;
    std::vector<V3D> testPoint;

    double testHeight;
    double tinnerRadius;
    double touterRadius;

    shapeInfo.getObjectGeometry(testShape, testPoint, tinnerRadius,

                                touterRadius, testHeight);
    TS_ASSERT_EQUALS(tinnerRadius, innerRadius);
    TS_ASSERT_EQUALS(touterRadius, outerRadius);
    TS_ASSERT_EQUALS(testHeight, height);
    TS_ASSERT(testPoint.size() == 2);
    TS_ASSERT_EQUALS(testPoint[0], centreBottomBase);
    TS_ASSERT_EQUALS(testPoint[1], symmetryAxis);
    TS_ASSERT_EQUALS(testShape, ShapeInfo::GeometryShape::HOLLOWCYLINDER);
  }

  void testCuboidGeometry() {
    ShapeInfo shapeInfo;
    constexpr const V3D p1(0, 0, 0);
    constexpr const V3D p2(0, 0, 1);
    constexpr const V3D p3(0, 1, 0);
    constexpr const V3D p4(0, 1, 1);

    shapeInfo.setCuboid(p1, p2, p3, p4);
    const auto geometry = shapeInfo.cuboidGeometry();
    TS_ASSERT_EQUALS(geometry.leftFrontBottom, p1);
    TS_ASSERT_EQUALS(geometry.leftFrontTop, p2);
    TS_ASSERT_EQUALS(geometry.leftBackBottom, p3);
    TS_ASSERT_EQUALS(geometry.rightFrontBottom, p4);
  }

  void testHexahedronGeometry() {
    ShapeInfo shapeInfo;
    constexpr V3D p1(0, 0, 0);
    constexpr V3D p2(0, 0, 1);
    constexpr V3D p3(0, 1, 0);
    constexpr V3D p4(0, 1, 1);
    constexpr V3D p5(1, 0, 0);
    constexpr V3D p6(1, 0, 1);
    constexpr V3D p7(1, 1, 0);
    constexpr V3D p8(1, 1, 1);

    shapeInfo.setHexahedron(p1, p2, p3, p4, p5, p6, p7, p8);
    const auto geometry = shapeInfo.hexahedronGeometry();
    TS_ASSERT_EQUALS(geometry.leftBackBottom, p1);
    TS_ASSERT_EQUALS(geometry.leftFrontBottom, p2);
    TS_ASSERT_EQUALS(geometry.rightFrontBottom, p3);
    TS_ASSERT_EQUALS(geometry.rightBackBottom, p4);
    TS_ASSERT_EQUALS(geometry.leftBackTop, p5);
    TS_ASSERT_EQUALS(geometry.leftFrontTop, p6);
    TS_ASSERT_EQUALS(geometry.rightFrontTop, p7);
    TS_ASSERT_EQUALS(geometry.rightBackTop, p8);
  }

  void testSphereGeometry() {
    ShapeInfo shapeInfo;
    constexpr V3D center(0, 0, 0);
    constexpr double radius = 10;
    shapeInfo.setSphere(center, radius);
    const auto geometry = shapeInfo.sphereGeometry();
    TS_ASSERT_EQUALS(geometry.centre, center);
    TS_ASSERT_EQUALS(geometry.radius, radius);
  }

  void testCylinderGeometry() {
    ShapeInfo shapeInfo;
    constexpr V3D center(0, 0, 0);
    constexpr V3D axis(1, 0, 0);
    constexpr double radius = 10;
    constexpr double height = 5;
    shapeInfo.setCylinder(center, axis, radius, height);
    const auto geometry = shapeInfo.cylinderGeometry();
    TS_ASSERT_EQUALS(geometry.centreOfBottomBase, center);
    TS_ASSERT_EQUALS(geometry.axis, axis);
    TS_ASSERT_EQUALS(geometry.radius, radius);
    TS_ASSERT_EQUALS(geometry.height, height);
  }

  void testHollowCylinderGeometry() {
    ShapeInfo shapeInfo;
    constexpr V3D centerOfBottomBase(0, 0, 0);
    constexpr V3D symmetryAxis(1, 0, 0);
    constexpr double height = 5;
    constexpr double innerRadius = 5;
    constexpr double outerRadius = 6;
    shapeInfo.setHollowCylinder(centerOfBottomBase, symmetryAxis, innerRadius,
                                outerRadius, height);
    const auto geometry = shapeInfo.hollowCylinderGeometry();
    TS_ASSERT_EQUALS(geometry.centreOfBottomBase, centerOfBottomBase)
    TS_ASSERT_EQUALS(geometry.axis, symmetryAxis);
    TS_ASSERT_EQUALS(geometry.innerRadius, innerRadius);
    TS_ASSERT_EQUALS(geometry.radius, outerRadius);
    TS_ASSERT_EQUALS(geometry.height, height);
  }

  void testConeGeometry() {
    ShapeInfo shapeInfo;
    constexpr V3D center(0, 0, 0);
    constexpr V3D axis(1, 0, 0);
    constexpr double radius = 10;
    constexpr double height = 5;
    shapeInfo.setCone(center, axis, radius, height);
    const auto geometry = shapeInfo.coneGeometry();
    TS_ASSERT_EQUALS(geometry.centre, center);
    TS_ASSERT_EQUALS(geometry.axis, axis);
    TS_ASSERT_EQUALS(geometry.radius, radius);
    TS_ASSERT_EQUALS(geometry.height, height);
  }

  void testCopyConstructor() {
    ShapeInfo shapeInfo;
    constexpr V3D center(0, 2, 1);
    constexpr double radius = 10;
    shapeInfo.setSphere(center, radius);

    ShapeInfo shapeInfoCopy(shapeInfo);

    TS_ASSERT_EQUALS(shapeInfo.shape(), shapeInfoCopy.shape());
    TS_ASSERT_EQUALS(shapeInfo.radius(), shapeInfoCopy.radius());
    TS_ASSERT_EQUALS(shapeInfo.height(), shapeInfoCopy.height());
    TS_ASSERT_EQUALS(shapeInfo.points(), shapeInfoCopy.points());
  }

  void testEquality() {
    ShapeInfo shapeInfo;
    constexpr V3D center(0, 2, 1);
    constexpr double radius = 10;
    shapeInfo.setSphere(center, radius);
    ShapeInfo shapeInfo2, shapeInfo3;
    shapeInfo2.setSphere(center, radius);
    shapeInfo3.setCuboid(V3D(), V3D(), V3D(), V3D());
    TS_ASSERT_EQUALS(shapeInfo2, shapeInfo);
    TS_ASSERT_DIFFERS(shapeInfo3, shapeInfo);
  }
};
