#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

using Mantid::Kernel::V3D;
using namespace Mantid::Geometry::detail;

class ShapeInfoTest : public CxxTest::TestSuite {
private:
  ShapeInfo m_shapeInfo;

public:
  void testConstructEmptyInitiazesEverythingZero() {
    TS_ASSERT(m_shapeInfo.points().size() == 0);
    TS_ASSERT(m_shapeInfo.height() == 0);
    TS_ASSERT(m_shapeInfo.radius() == 0);
    TS_ASSERT(m_shapeInfo.shape() == ShapeInfo::GeometryShape::NOSHAPE);
  }

  void testSetSphere() {
    V3D center(0, 0, 0);
    double radius = 10;
    m_shapeInfo.setSphere(center, radius);

    TS_ASSERT_EQUALS(m_shapeInfo.shape(), ShapeInfo::GeometryShape::SPHERE);
    TS_ASSERT_EQUALS(m_shapeInfo.radius(), radius);
    TS_ASSERT_EQUALS(m_shapeInfo.height(), 0);
    TS_ASSERT_EQUALS(m_shapeInfo.points().size(), 1);
    TS_ASSERT_EQUALS(m_shapeInfo.points()[0], center);
  }

  void testSetCuboid() {
    V3D p1(0, 0, 0);
    V3D p2(0, 0, 1);
    V3D p3(0, 1, 0);
    V3D p4(0, 1, 1);

    m_shapeInfo.setCuboid(p1, p2, p3, p4);

    TS_ASSERT_EQUALS(m_shapeInfo.shape(), ShapeInfo::GeometryShape::CUBOID);
    TS_ASSERT_EQUALS(m_shapeInfo.radius(), 0);
    TS_ASSERT_EQUALS(m_shapeInfo.height(), 0);
    TS_ASSERT_EQUALS(m_shapeInfo.points().size(), 4);
    TS_ASSERT_EQUALS(m_shapeInfo.points(), (std::vector<V3D>{p1, p2, p3, p4}));
  }

  void testSetHexahedron() {
    V3D p1(0, 0, 0);
    V3D p2(0, 0, 1);
    V3D p3(0, 1, 0);
    V3D p4(0, 1, 1);
    V3D p5(1, 0, 0);
    V3D p6(1, 0, 1);
    V3D p7(1, 1, 0);
    V3D p8(1, 1, 1);

    m_shapeInfo.setHexahedron(p1, p2, p3, p4, p5, p6, p7, p8);

    TS_ASSERT_EQUALS(m_shapeInfo.shape(), ShapeInfo::GeometryShape::HEXAHEDRON);
    TS_ASSERT_EQUALS(m_shapeInfo.radius(), 0);
    TS_ASSERT_EQUALS(m_shapeInfo.height(), 0);
    TS_ASSERT_EQUALS(m_shapeInfo.points().size(), 8);
    TS_ASSERT_EQUALS(m_shapeInfo.points(),
                     (std::vector<V3D>{p1, p2, p3, p4, p5, p6, p7, p8}));
  }

  void testSetCone() {
    V3D center(0, 0, 0);
    V3D axis(1, 0, 0);
    double radius = 10;
    double height = 5;
    m_shapeInfo.setCone(center, axis, radius, height);

    TS_ASSERT_EQUALS(m_shapeInfo.shape(), ShapeInfo::GeometryShape::CONE);
    TS_ASSERT_EQUALS(m_shapeInfo.radius(), radius);
    TS_ASSERT_EQUALS(m_shapeInfo.height(), height);
    TS_ASSERT_EQUALS(m_shapeInfo.points().size(), 2);
    TS_ASSERT_EQUALS(m_shapeInfo.points()[0], center);
    TS_ASSERT_EQUALS(m_shapeInfo.points()[1], axis);
  }

  void testSetCylinder() {
    V3D center(0, 0, 0);
    V3D axis(1, 0, 0);
    double radius = 10;
    double height = 5;
    m_shapeInfo.setCylinder(center, axis, radius, height);

    TS_ASSERT_EQUALS(m_shapeInfo.shape(), ShapeInfo::GeometryShape::CYLINDER);
    TS_ASSERT_EQUALS(m_shapeInfo.radius(), radius);
    TS_ASSERT_EQUALS(m_shapeInfo.height(), height);
    TS_ASSERT_EQUALS(m_shapeInfo.points().size(), 2);
    TS_ASSERT_EQUALS(m_shapeInfo.points()[0], center);
    TS_ASSERT_EQUALS(m_shapeInfo.points()[1], axis);
  }

  void testGetObjectGeometry() {
    V3D center(0, 0, 0);
    double radius = 10;
    m_shapeInfo.setSphere(center, radius);

    ShapeInfo::GeometryShape tshape;
    std::vector<V3D> tpoints;
    double theight;
    double tradius;

    m_shapeInfo.getObjectGeometry(tshape, tpoints, tradius, theight);
    TS_ASSERT_EQUALS(tradius, radius);
    TS_ASSERT(theight == 0);
    TS_ASSERT(tpoints.size() == 1);
    TS_ASSERT_EQUALS(tpoints[0], center);
    TS_ASSERT_EQUALS(tshape, ShapeInfo::GeometryShape::SPHERE);
  }

  void testCopyConstructor() {
    V3D center(0, 2, 1);
    double radius = 10;
    m_shapeInfo.setSphere(center, radius);

    ShapeInfo shapeInfoCopy(m_shapeInfo);

    TS_ASSERT_EQUALS(m_shapeInfo.shape(), shapeInfoCopy.shape());
    TS_ASSERT_EQUALS(m_shapeInfo.radius(), shapeInfoCopy.radius());
    TS_ASSERT_EQUALS(m_shapeInfo.height(), shapeInfoCopy.height());
    TS_ASSERT_EQUALS(m_shapeInfo.points(), shapeInfoCopy.points());
  }

  void testEquality() {
    V3D center(0, 2, 1);
    double radius = 10;
    m_shapeInfo.setSphere(center, radius);
    ShapeInfo shapeInfo2, shapeInfo3;
    shapeInfo2.setSphere(center, radius);
    shapeInfo3.setCuboid(V3D(), V3D(), V3D(), V3D());
    TS_ASSERT_EQUALS(shapeInfo2, m_shapeInfo);
    TS_ASSERT_DIFFERS(shapeInfo3, m_shapeInfo);
  }
};
