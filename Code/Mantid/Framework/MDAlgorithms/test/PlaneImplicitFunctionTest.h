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
    MOCK_CONST_METHOD0 (getX, double());
    MOCK_CONST_METHOD0(getY, double());
    MOCK_CONST_METHOD0(getZ, double());
  };

  Mantid::MDAlgorithms::NormalParameter normal;
  Mantid::MDAlgorithms::OriginParameter origin;
  Mantid::MDAlgorithms::WidthParameter width;
  Mantid::MDAlgorithms::UpParameter up;
  const double PI;

public:

  PlaneImplicitFunctionTest() :
    normal(1, 1, 1), origin(2, 3, 4), width(2), up(0, 1, 0), PI(3.14159265)
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
    TSM_ASSERT_EQUALS("Width component not wired-up correctly", 2, plane.getWidth());

  }

  void testEvaluateInsidePointOnForwardSurface()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1));
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2));
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3));

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    UpParameter tUp;//Not important at all in this test

    PlaneImplicitFunction plane(tNormal, tOrigin, tUp, tWidth);
    bool isInside = plane.evaluate(&point);
    TSM_ASSERT("The point should have been found to be inside the region bounded by the plane.", isInside);
  }

  void testEvaluateInsidePointOnBackwardSurface()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(-1));
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(-2));
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(-3));

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    UpParameter tUp;//Not important at all in this test

    PlaneImplicitFunction plane(tNormal, tOrigin, tUp, tWidth);
    bool isInside = plane.evaluate(&point);
    TSM_ASSERT("The point should have been found to be inside the region bounded by the plane.", isInside);
  }

  void testEvaluateInsidePointReflectNormal() //Test that plane automatically relects normals where necessary.
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1));
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2));
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3));

    NormalParameter tNormal(1, 2, 3);
    NormalParameter rNormal = tNormal.reflect();
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    UpParameter tUp;//Not important at all in this test

    PlaneImplicitFunction plane(rNormal, tOrigin, tUp, tWidth);
    bool isInside = plane.evaluate(&point);
    TSM_ASSERT("The point should have been found to be inside the region bounded by the plane after the normal was reflected.", isInside);
  }

  void testEvaluatePointOutsideForwardPlane()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1.001)); //Just outside
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2.001)); //Just outside
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3.001)); //Just outside

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    UpParameter tUp;//Not important at all in this test

    PlaneImplicitFunction plane(tNormal, tOrigin, tUp, tWidth);
    bool isInside = plane.evaluate(&point);
    TSM_ASSERT("The point should have been found to be outside the region bounded by the plane.", !isInside);
  }

  void testEvaluatePointOutsideBackwardPlane()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(-1.001)); //Just outside
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(-2.001)); //Just outside
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(-3.001)); //Just outside

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    UpParameter tUp;//Not important at all in this test

    PlaneImplicitFunction plane(tNormal, tOrigin, tUp, tWidth);
    bool isInside = plane.evaluate(&point);
    TSM_ASSERT("The point should have been found to be outside the region bounded by the plane.", !isInside);
  }

  void testEvaluateOnPlanePointDecreaseX()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(0.999)); //Just inside
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2)); //Just on
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3)); //Just on

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    UpParameter tUp;//Not important at all in this test

    PlaneImplicitFunction plane(tNormal, tOrigin, tUp, tWidth);

    bool isInside = plane.evaluate(&point);
    TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point x-value.", isInside);
  }

  void testEvaluateOnPlanePointIncreaseX()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1.001)); //Just outside
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2)); //Just on
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3)); //Just on

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    UpParameter tUp;//Not important at all in this test

    PlaneImplicitFunction plane(tNormal, tOrigin, tUp, tWidth);

    bool isInside = plane.evaluate(&point);
    TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point x-value.", !isInside);
  }

  void testEvaluateOnPlanePointDecreaseY()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1)); //Just on
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(1.999)); //Just inside
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3)); //Just on

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    UpParameter tUp;//Not important at all in this test

    PlaneImplicitFunction plane(tNormal, tOrigin, tUp, tWidth);

    bool isInside = plane.evaluate(&point);
    TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point y-value.", isInside);
  }

  void testEvaluateOnPlanePointIncreaseY()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1)); //Just on
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2.001)); //Just outside
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3)); //Just on

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    UpParameter tUp;//Not important at all in this test

    PlaneImplicitFunction plane(tNormal, tOrigin, tUp, tWidth);

    bool isInside = plane.evaluate(&point);
    TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point y-value.", !isInside);
  }

  void testEvaluateOnPlanePointDecreaseZ()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1)); //Just on
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2)); //Just on
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(2.999)); //Just inside

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    UpParameter tUp;//Not important at all in this test

    PlaneImplicitFunction plane(tNormal, tOrigin, tUp, tWidth);

    bool isInside = plane.evaluate(&point);
    TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point z-value.", isInside);
  }

  void testEvaluateOnPlanePointIncreaseZ()
  {
    using namespace Mantid::MDDataObjects;
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1)); //Just on
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2)); //Just on
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3.001)); //Just outside

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    UpParameter tUp;//Not important at all in this test

    PlaneImplicitFunction plane(tNormal, tOrigin, tUp, tWidth);

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


  void testWellFormedRotationMatrix()
  {
    using namespace Mantid::MDAlgorithms;
    using namespace Mantid::Geometry;

    NormalParameter tNormal(0.5, 0, 0.5);
    UpParameter tUp(0, 1, 0);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth; // Width unimportant for this test, so left invalid.
    PlaneImplicitFunction plane(tNormal, tOrigin, tUp, tWidth);

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

  void testNonOrthogonalUpandNormalThows()
  {
    using namespace Mantid::MDAlgorithms;
    using namespace Mantid::Geometry;

    NormalParameter tNormal(0.5, 1, 0.5);
    UpParameter tUp(0, 1, 0); // tUp and tNormal are not orthogonal!
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth; // Width unimportant for this test, so left invalid.
    PlaneImplicitFunction plane(tNormal, tOrigin, tUp, tWidth);

    TSM_ASSERT_THROWS("Attempting to calculate rotation matrix from two vectors that are not orthogonal. Should throw.", plane.asRotationMatrixVector(), std::logic_error);
  }

};

#endif
