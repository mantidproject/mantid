#ifndef PLANE_IMPLICIT_FUNCTION_TEST_H_
#define PLANE_IMPLICIT_FUNCTION_TEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>
#include <MantidGeometry/Math/Matrix.h>
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"

class PlaneImplicitFunctionTest: public CxxTest::TestSuite
{
private:

  class MockPoint3D: public Mantid::API::Point3D
  {
  public:
  MOCK_CONST_METHOD0  (getX, double());
  MOCK_CONST_METHOD0(getY, double());
  MOCK_CONST_METHOD0(getZ, double());
};

Mantid::MDAlgorithms::NormalParameter normal;
Mantid::MDAlgorithms::OriginParameter origin;
Mantid::MDAlgorithms::WidthParameter width;
Mantid::MDAlgorithms::UpParameter up;
const double PI;

public:

PlaneImplicitFunctionTest() : normal(1, 1, 1), origin(2, 3, 4), up(0, 1, 0), width(2), PI(3.14159265)
{
}

void testPlaneImplicitFunctionConstruction(void)
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter normalParam(1, 0, 0);

  PlaneImplicitFunction plane(normalParam, origin, up, width);
  TSM_ASSERT_EQUALS("Normal x component not wired-up correctly", 1, plane.getNormalX());
  TSM_ASSERT_EQUALS("Normal y component not wired-up correctly", 0, plane.getNormalY());
  TSM_ASSERT_EQUALS("Normal z component not wired-up correctly", 0, plane.getNormalZ());
  TSM_ASSERT_EQUALS("Origin x component not wired-up correctly", 2, plane.getOriginX());
  TSM_ASSERT_EQUALS("Origin y component not wired-up correctly", 3, plane.getOriginY());
  TSM_ASSERT_EQUALS("Origin z component not wired-up correctly", 4, plane.getOriginZ());
  TSM_ASSERT_EQUALS("Up x component not wired-up correctly", 0, plane.getUpX());
  TSM_ASSERT_EQUALS("Up y component not wired-up correctly", 1, plane.getUpY());
  TSM_ASSERT_EQUALS("Up z component not wired-up correctly", 0, plane.getUpZ());
  TSM_ASSERT_EQUALS("Perpendicular x component not wired-up correctly", 0, plane.getPerpendicularX());
  TSM_ASSERT_EQUALS("Perpendicular y component not wired-up correctly", 0, plane.getPerpendicularY());
  TSM_ASSERT_EQUALS("Perpendicular z component not wired-up correctly", 1, plane.getPerpendicularZ());
  TSM_ASSERT_EQUALS("Width component not wired-up correctly", 2, plane.getWidth());

}

void testEvaluateInsidePoint()
{
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(1).WillOnce(testing::Return(1));
  EXPECT_CALL(point, getY()).Times(1).WillOnce(testing::Return(1));
  EXPECT_CALL(point, getZ()).Times(1).WillOnce(testing::Return(1));

  PlaneImplicitFunction plane(normal, origin, up, width);
  bool isInside = plane.evaluate(&point);
  TSM_ASSERT("The point should have been found to be inside the region bounded by the plane.", isInside);
}

void testEvaluateInsidePointReflectNormal()
{
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(1).WillOnce(testing::Return(1));
  EXPECT_CALL(point, getY()).Times(1).WillOnce(testing::Return(1));
  EXPECT_CALL(point, getZ()).Times(1).WillOnce(testing::Return(1));

  NormalParameter* reflectNormal = normal.reflect();
  NormalParameter rNormal(reflectNormal->getX(), reflectNormal->getY(), reflectNormal->getZ());
  PlaneImplicitFunction plane(rNormal, origin, up, width);
  bool isInside = plane.evaluate(&point);
  TSM_ASSERT("The point should not have been found to be inside the region bounded by the plane after the normal was reflected.", !isInside);
  delete reflectNormal;
}

void testEvaluateOutsidePoint()
{
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(1).WillOnce(testing::Return(4));
  EXPECT_CALL(point, getY()).Times(1).WillOnce(testing::Return(4));
  EXPECT_CALL(point, getZ()).Times(1).WillOnce(testing::Return(4));

  PlaneImplicitFunction plane(normal, origin, up, width);
  bool isInside = plane.evaluate(&point);
  TSM_ASSERT("The point should have been found to be outside the region bounded by the plane.", !isInside);
}

void testEvaluateOutsidePointReflectNormal()
{
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(1).WillOnce(testing::Return(4));
  EXPECT_CALL(point, getY()).Times(1).WillOnce(testing::Return(4));
  EXPECT_CALL(point, getZ()).Times(1).WillOnce(testing::Return(4));

  NormalParameter* reflectNormal = normal.reflect();;
  NormalParameter rNormal(reflectNormal->getX(), reflectNormal->getY(), reflectNormal->getZ());
  PlaneImplicitFunction plane(rNormal, origin, up, width);
  bool isInside = plane.evaluate(&point);
  TSM_ASSERT("The point should not have been found to be outside the region bounded by the plane after the normal was reflected.", isInside);
  delete reflectNormal;
}

void testEvaluateOnPlanePoint()
{
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(1).WillOnce(testing::Return(origin.getX()));
  EXPECT_CALL(point, getY()).Times(1).WillOnce(testing::Return(origin.getY()));
  EXPECT_CALL(point, getZ()).Times(1).WillOnce(testing::Return(origin.getZ()));

  PlaneImplicitFunction plane(normal, origin, up, width);
  bool isInside = plane.evaluate(&point);
  TSM_ASSERT("The point should have been found to be on the region bounded by the plane.", isInside);
}

void testEvaluateOnPlanePointReflectNormal()
{
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(1).WillOnce(testing::Return(origin.getX()));
  EXPECT_CALL(point, getY()).Times(1).WillOnce(testing::Return(origin.getY()));
  EXPECT_CALL(point, getZ()).Times(1).WillOnce(testing::Return(origin.getZ()));

  NormalParameter* reflectNormal = normal.reflect();
  NormalParameter rNormal(reflectNormal->getX(), reflectNormal->getY(), reflectNormal->getZ());
  PlaneImplicitFunction plane(rNormal, origin, up, width);
  bool isInside = plane.evaluate(&point);
  TSM_ASSERT("The point should have been found to be on the region bounded by the plane even after the normal was reflected.", isInside);
  delete reflectNormal;
}

void testEvaluateOnPlanePointDecreaseX()
{
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(1).WillOnce(testing::Return(origin.getX() - 0.0001));
  EXPECT_CALL(point, getY()).Times(1).WillOnce(testing::Return(origin.getY()));
  EXPECT_CALL(point, getZ()).Times(1).WillOnce(testing::Return(origin.getZ()));

  PlaneImplicitFunction plane(normal, origin, up, width);
  bool isInside = plane.evaluate(&point);
  TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point x-value.", isInside);
}

void testEvaluateOnPlanePointIncreaseX()
{
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(1).WillOnce(testing::Return(origin.getX()+ 0.0001));
  EXPECT_CALL(point, getY()).Times(1).WillOnce(testing::Return(origin.getY()));
  EXPECT_CALL(point, getZ()).Times(1).WillOnce(testing::Return(origin.getZ()));

  PlaneImplicitFunction plane(normal, origin, up, width);
  bool isInside = plane.evaluate(&point);
  TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point x-value.", !isInside);
}

void testEvaluateOnPlanePointDecreaseY()
{
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(1).WillOnce(testing::Return(origin.getX()));
  EXPECT_CALL(point, getY()).Times(1).WillOnce(testing::Return(origin.getY() - 0.0001));
  EXPECT_CALL(point, getZ()).Times(1).WillOnce(testing::Return(origin.getZ()));

  PlaneImplicitFunction plane(normal, origin, up, width);
  bool isInside = plane.evaluate(&point);
  TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point y-value.", isInside);
}

void testEvaluateOnPlanePointIncreaseY()
{
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(1).WillOnce(testing::Return(origin.getX()));
  EXPECT_CALL(point, getY()).Times(1).WillOnce(testing::Return(origin.getY() + 0.0001));
  EXPECT_CALL(point, getZ()).Times(1).WillOnce(testing::Return(origin.getZ()));

  PlaneImplicitFunction plane(normal, origin, up, width);
  bool isInside = plane.evaluate(&point);
  TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point y-value.", !isInside);
}

void testEvaluateOnPlanePointDecreaseZ()
{
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(1).WillOnce(testing::Return(origin.getX()));
  EXPECT_CALL(point, getY()).Times(1).WillOnce(testing::Return(origin.getY()));
  EXPECT_CALL(point, getZ()).Times(1).WillOnce(testing::Return(origin.getZ() - 0.0001));

  PlaneImplicitFunction plane(normal, origin, up, width);
  bool isInside = plane.evaluate(&point);
  TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point z-value.", isInside);
}

void testEvaluateOnPlanePointIncreaseZ()
{
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(1).WillOnce(testing::Return(origin.getX()));
  EXPECT_CALL(point, getY()).Times(1).WillOnce(testing::Return(origin.getY()));
  EXPECT_CALL(point, getZ()).Times(1).WillOnce(testing::Return(origin.getZ() + 0.0001));

  PlaneImplicitFunction plane(normal, origin, up, width);
  bool isInside = plane.evaluate(&point);
  TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point z-value.", !isInside);
}

void testToXML()
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter tNormal(1, 0, 0);
  OriginParameter tOrigin(0, 0, 0);
  UpParameter tUp(0, 1, 0);
  WidthParameter tWidth(3);
  PlaneImplicitFunction plane(tNormal, tOrigin, tUp, tWidth);
  TSM_ASSERT_EQUALS("The xml generated by this function did not match the expected schema.", "<Function><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>1.0000, 0.0000, 0.0000</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>0.0000, 0.0000, 0.0000</Value></Parameter><Parameter><Type>UpParameter</Type><Value>0.0000, 1.0000, 0.0000</Value></Parameter><Parameter><Type>WidthParameter</Type><Value>3.0000</Value></Parameter></ParameterList></Function>", plane.toXMLString());
}

void testEqual()
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter n(1, 2, 3);
  OriginParameter o(4, 5, 6);
  UpParameter up(7, 8, 9);
  WidthParameter width(10);
  PlaneImplicitFunction A(n, o, up, width);
  PlaneImplicitFunction B(n, o, up, width);
  TSM_ASSERT_EQUALS("These two objects should be considered equal.", A, B);
}

void testNotEqual()
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter n1(1, 2, 3);
  OriginParameter o1(4, 5, 6);
  WidthParameter width1(10);
  UpParameter up1(7, 8, 9);
  NormalParameter n2(0, 0, 0);
  OriginParameter o2(0, 0, 0);
  UpParameter up2(0, 0, 0);
  WidthParameter width2(0);
  PlaneImplicitFunction A(n1, o1, up1, width1); //Base comparison
  PlaneImplicitFunction B(n2, o1, up1, width1); //Differ normal only
  PlaneImplicitFunction C(n1, o2, up1, width1); //Differ origin only
  PlaneImplicitFunction D(n1, o1, up2, width1); //Differ up only
  PlaneImplicitFunction E(n1, o1, up1, width2); //Differ width only
  TSM_ASSERT_DIFFERS("These two objects should not be considered equal.", A, B);
  TSM_ASSERT_DIFFERS("These two objects should not be considered equal.", A, C);
  TSM_ASSERT_DIFFERS("These two objects should not be considered equal.", A, D);
  TSM_ASSERT_DIFFERS("These two objects should not be considered equal.", A, E);
}

void testGetAngleWithXAxis()
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter normalPerpendicular(0, 1, 0);
  NormalParameter normalParallel(1, 0, 0);
  OriginParameter origin(0, 0, 0);
  PlaneImplicitFunction A(normalPerpendicular, origin, up, width);
  PlaneImplicitFunction B(normalParallel, origin, up, width);

  TSM_ASSERT_DELTA("The angle made with the x-axis should be PI/2", PI/2 , A.getAngleMadeWithXAxis(), 0.001);
  TSM_ASSERT_DELTA("The angle made with the x-axis should be 0", 0 , B.getAngleMadeWithXAxis(), 0.001);
}

void testGetAngleWithYAxis()
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter normalPerpendicular(1, 0, 0);
  NormalParameter normalParallel(0, 1, 0);
  OriginParameter origin(0, 0, 0);
  PlaneImplicitFunction A(normalPerpendicular, origin, up, width);
  PlaneImplicitFunction B(normalParallel, origin, up, width);

  TSM_ASSERT_DELTA("The angle made with the y-axis should be PI/2", PI/2 , A.getAngleMadeWithYAxis(), 0.001);
  TSM_ASSERT_DELTA("The angle made with the y-axis should be 0", 0 , B.getAngleMadeWithYAxis(), 0.001);
}

void testGetAngleWithZAxis()
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter normalPerpendicular(1, 0, 0);
  NormalParameter normalParallel(0, 0, 1);
  OriginParameter origin(0, 0, 0);
  PlaneImplicitFunction A(normalPerpendicular, origin, up, width);
    PlaneImplicitFunction B(normalParallel, origin, up, width);

    TSM_ASSERT_DELTA("The angle made with the z-axis should be PI/2", PI/2 , A.getAngleMadeWithZAxis(), 0.001);
    TSM_ASSERT_DELTA("The angle made with the z-axis should be 0", 0 , B.getAngleMadeWithZAxis(), 0.001);
  }

  void testWellFormedRotationMatrix()
  {
    using namespace Mantid::MDAlgorithms;
    using namespace Mantid::Geometry;

    NormalParameter normal(1, 2, 3);
    OriginParameter origin(0, 0, 0);
    PlaneImplicitFunction plane(normal, origin, up, width);

    Matrix<double> rotationMatrix = extractRotationMatrix(plane);
    //Copy and modify.
    Matrix<double> transposeRotationMatrix = rotationMatrix;
    transposeRotationMatrix.Transpose();
    //Copy and modify.
    Matrix<double> invertRotationMatrix = rotationMatrix;
    invertRotationMatrix.Invert();

    TSM_ASSERT_DELTA("The determinant of a rotation matrix is always 1", 1, rotationMatrix.determinant(), 0.001);
    TSM_ASSERT_EQUALS("The inverse of a rotation matrix is equal to its transpose", invertRotationMatrix, transposeRotationMatrix);
  }

};

#endif
