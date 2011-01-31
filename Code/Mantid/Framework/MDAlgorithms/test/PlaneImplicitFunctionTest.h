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
const double PI;

public:

PlaneImplicitFunctionTest() : normal(1, 1, 1), origin(2, 3, 4), PI(3.14159265)
{
}

void testPlaneImplicitFunctionConstruction(void)
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter nonOrthogonalNormal(0, 0.5, 1); //Non-orthogonal normal used so that getters can be properly verified

  PlaneImplicitFunction plane(nonOrthogonalNormal, origin);
  TSM_ASSERT_EQUALS("Normal x component not wired-up correctly", 0, plane.getNormalX());
  TSM_ASSERT_EQUALS("Normal y component not wired-up correctly", 0.5, plane.getNormalY());
  TSM_ASSERT_EQUALS("Normal z component not wired-up correctly", 1, plane.getNormalZ());
  TSM_ASSERT_EQUALS("Origin x component not wired-up correctly", 2, plane.getOriginX());
  TSM_ASSERT_EQUALS("Origin y component not wired-up correctly", 3, plane.getOriginY());
  TSM_ASSERT_EQUALS("Origin z component not wired-up correctly", 4, plane.getOriginZ());
}

void testEvaluateInsidePoint()
{
  using namespace Mantid::MDDataObjects;
  using namespace Mantid::MDAlgorithms;

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(1).WillOnce(testing::Return(1));
  EXPECT_CALL(point, getY()).Times(1).WillOnce(testing::Return(1));
  EXPECT_CALL(point, getZ()).Times(1).WillOnce(testing::Return(1));

  PlaneImplicitFunction plane(normal, origin);
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
  PlaneImplicitFunction plane(rNormal, origin);
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

  PlaneImplicitFunction plane(normal, origin);
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
  PlaneImplicitFunction plane(rNormal, origin);
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

  PlaneImplicitFunction plane(normal, origin);
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
  PlaneImplicitFunction plane(rNormal, origin);
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

  PlaneImplicitFunction plane(normal, origin);
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

  PlaneImplicitFunction plane(normal, origin);
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

  PlaneImplicitFunction plane(normal, origin);
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

  PlaneImplicitFunction plane(normal, origin);
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

  PlaneImplicitFunction plane(normal, origin);
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

  PlaneImplicitFunction plane(normal, origin);
  bool isInside = plane.evaluate(&point);
  TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point z-value.", !isInside);
}

void testToXML()
{
  using namespace Mantid::MDAlgorithms;
  PlaneImplicitFunction plane(normal, origin);
  TSM_ASSERT_EQUALS("The xml generated by this function did not match the expected schema.", "<Function><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>1.0000, 1.0000, 1.0000</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>2.0000, 3.0000, 4.0000</Value></Parameter></ParameterList></Function>", plane.toXMLString());
}

void testEqual()
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter n(1, 2, 3);
  OriginParameter o(4, 5, 6);
  PlaneImplicitFunction A(n, o);
  PlaneImplicitFunction B(n, o);
  TSM_ASSERT_EQUALS("These two objects should be considered equal.", A, B);
}

void testNotEqual()
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter n1(1, 2, 3);
  OriginParameter o1(4, 5, 6);
  NormalParameter n2(0, 0, 0);
  OriginParameter o2(0, 0, 0);
  PlaneImplicitFunction A(n1, o1);
  PlaneImplicitFunction B(n2, o1);
  PlaneImplicitFunction C(n1, o2);
  TSM_ASSERT_DIFFERS("These two objects should not be considered equal.", A, B);
  TSM_ASSERT_DIFFERS("These two objects should not be considered equal.", A, C);
}

void testGetAngleWithXAxis()
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter normalPerpendicular(0, 2, 0);
  NormalParameter normalParallel(3, 0, 0);
  OriginParameter origin(0, 0, 0);
  PlaneImplicitFunction A(normalPerpendicular, origin);
  PlaneImplicitFunction B(normalParallel, origin);

  TSM_ASSERT_DELTA("The angle made with the x-axis should be PI/2", PI/2 , A.getAngleMadeWithXAxis(), 0.001);
  TSM_ASSERT_DELTA("The angle made with the x-axis should be 0", 0 , B.getAngleMadeWithXAxis(), 0.001);
}

void testGetAngleWithYAxis()
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter normalPerpendicular(2, 0, 0);
  NormalParameter normalParallel(0, 3, 0);
  OriginParameter origin(0, 0, 0);
  PlaneImplicitFunction A(normalPerpendicular, origin);
  PlaneImplicitFunction B(normalParallel, origin);

  TSM_ASSERT_DELTA("The angle made with the y-axis should be PI/2", PI/2 , A.getAngleMadeWithYAxis(), 0.001);
  TSM_ASSERT_DELTA("The angle made with the y-axis should be 0", 0 , B.getAngleMadeWithYAxis(), 0.001);
}

void testGetAngleWithZAxis()
{
  using namespace Mantid::MDAlgorithms;
  NormalParameter normalPerpendicular(2, 0, 0);
  NormalParameter normalParallel(0, 0, 3);
  OriginParameter origin(0, 0, 0);
  PlaneImplicitFunction A(normalPerpendicular, origin);
    PlaneImplicitFunction B(normalParallel, origin);

    TSM_ASSERT_DELTA("The angle made with the z-axis should be PI/2", PI/2 , A.getAngleMadeWithZAxis(), 0.001);
    TSM_ASSERT_DELTA("The angle made with the z-axis should be 0", 0 , B.getAngleMadeWithZAxis(), 0.001);
  }

  void testWellFormedRotationMatrix()
  {
    using namespace Mantid::MDAlgorithms;
    using namespace Mantid::Geometry;

    NormalParameter normal(1, 2, 3);
    OriginParameter origin(0, 0, 0);
    PlaneImplicitFunction plane(normal, origin);

    Matrix<double> rotationMatrix = extractRotationMatrix(plane);
    //Copy and modify.
    Matrix<double> transposeRotationMatrix = rotationMatrix;
    transposeRotationMatrix.Transpose();
    //Copy and modify.
    Matrix<double> invertRotationMatrix = rotationMatrix;
    invertRotationMatrix.Invert();

    TSM_ASSERT_EQUALS("The determinant of a rotation matrix is always 1", 1, rotationMatrix.determinant());
    TSM_ASSERT_EQUALS("The inverse of a rotation matrix is equal to its transpose", invertRotationMatrix, transposeRotationMatrix);
  }

};

#endif
