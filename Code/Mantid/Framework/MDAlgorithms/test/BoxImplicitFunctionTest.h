#ifndef BOX_IMPLICIT_FUNCTION_TEST_H_
#define BOX_IMPLICIT_FUNCTION_TEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>
#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include <boost/scoped_ptr.hpp>

using namespace Mantid::MDAlgorithms;

class BoxImplicitFunctionTest: public CxxTest::TestSuite
{
private:

  //Helper class to verify correct behaviour against a 3D Point.
  class MockPoint3D: public Mantid::API::Point3D
  {
  public:
    MOCK_CONST_METHOD0  (getX, double());
    MOCK_CONST_METHOD0(getY, double());
    MOCK_CONST_METHOD0(getZ, double());
  };

  //Helper method to construct a valid vanilla box implicit function.
  static BoxImplicitFunction* constructBoxImplicitFunction()
  {
    OriginParameter origin(1, 2, 3); //Non-orthogonal normal used so that getters can be properly verified
    WidthParameter width(5);
    HeightParameter height(4);
    DepthParameter depth(6);
    return new BoxImplicitFunction(width, height, depth, origin);
  }

public:

void testBoxImplicitFunctionConstruction(void)
{
  boost::scoped_ptr<BoxImplicitFunction> box(constructBoxImplicitFunction());

  TSM_ASSERT_EQUALS("Upper x component not wired-up correctly", 3.5, box->getUpperX());
  TSM_ASSERT_EQUALS("Lower x component not wired-up correctly", -1.5, box->getLowerX());
  TSM_ASSERT_EQUALS("Upper y component not wired-up correctly", 4, box->getUpperY());
  TSM_ASSERT_EQUALS("Lower y component not wired-up correctly", 0, box->getLowerY());
  TSM_ASSERT_EQUALS("Upper z component not wired-up correctly", 6, box->getUpperZ());
  TSM_ASSERT_EQUALS("Lower z component not wired-up correctly", 0, box->getLowerZ());
}

void testEvaluateInsidePoint()
{
  boost::scoped_ptr<BoxImplicitFunction> box(constructBoxImplicitFunction());

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(0));
  EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(0));
  EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(0));

  bool isInside = box->evaluate(&point);
  TSM_ASSERT("The point should have been found to be inside the region bounded by the box.", isInside);
}

void testEvaluateOutsideXMax()
{
  boost::scoped_ptr<BoxImplicitFunction> box(constructBoxImplicitFunction());

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(1).WillRepeatedly(testing::Return(10));  //puts point outside of box.
  EXPECT_CALL(point, getY()).Times(0);
  EXPECT_CALL(point, getZ()).Times(0);

  bool isInside = box->evaluate(&point);
  TSM_ASSERT("The point should not have been found to be inside the region bounded by the box.", !isInside);
}

void testEvaluateOutsideXMin()
{
  boost::scoped_ptr<BoxImplicitFunction> box(constructBoxImplicitFunction());

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(-10));  //puts point outside of box.
  EXPECT_CALL(point, getY()).Times(0);
  EXPECT_CALL(point, getZ()).Times(0);

  bool isInside = box->evaluate(&point);
  TSM_ASSERT("The point should not have been found to be inside the region bounded by the box.", !isInside);
}

void testEvaluateOutsideYMax()
{
  boost::scoped_ptr<BoxImplicitFunction> box(constructBoxImplicitFunction());

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(0));
  EXPECT_CALL(point, getY()).Times(1).WillRepeatedly(testing::Return(10)); //puts point outside of box.
  EXPECT_CALL(point, getZ()).Times(0);

  bool isInside = box->evaluate(&point);
  TSM_ASSERT("The point should not have been found to be inside the region bounded by the box.", !isInside);
}

void testEvaluateOutsideYMin()
{
  boost::scoped_ptr<BoxImplicitFunction> box(constructBoxImplicitFunction());

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(0));
  EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(-10));  //puts point outside of box.
  EXPECT_CALL(point, getZ()).Times(0);

  bool isInside = box->evaluate(&point);
  TSM_ASSERT("The point should not have been found to be inside the region bounded by the box.", !isInside);
}

void testEvaluateOutsideZMax()
{
  boost::scoped_ptr<BoxImplicitFunction> box(constructBoxImplicitFunction());

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(0));
  EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(0));
  EXPECT_CALL(point, getZ()).Times(1).WillRepeatedly(testing::Return(10));  //puts point outside of box.

  bool isInside = box->evaluate(&point);
  TSM_ASSERT("The point should not have been found to be inside the region bounded by the box.", !isInside);
}

void testEvaluateOutsideZMin()
{
  boost::scoped_ptr<BoxImplicitFunction> box(constructBoxImplicitFunction());

  MockPoint3D point;
  EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(0));
  EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(0));
  EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(-10));  //puts point outside of box.

  bool isInside = box->evaluate(&point);
  TSM_ASSERT("The point should not have been found to be inside the region bounded by the box.", !isInside);
}

void testToXML()
{
  boost::scoped_ptr<BoxImplicitFunction> box(constructBoxImplicitFunction());
  //string comparison on generated xml.
  TSM_ASSERT_EQUALS("The xml generated by this function did not match the expected schema.", "<Function><Type>BoxImplicitFunction</Type><ParameterList><Parameter><Type>WidthParameter</Type><Value>5.0000</Value></Parameter><Parameter><Type>HeightParameter</Type><Value>4.0000</Value></Parameter><Parameter><Type>DepthParameter</Type><Value>6.0000</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>1.0000, 2.0000, 3.0000</Value></Parameter></ParameterList></Function>", box->toXMLString());
}

void testEqual()
{
  OriginParameter o(4, 5, 6);
  WidthParameter width(1);
  HeightParameter height(2);
  DepthParameter depth(3);
  BoxImplicitFunction A(width, height, depth, o);
  BoxImplicitFunction B(width, height, depth, o);
  TSM_ASSERT_EQUALS("These two objects should be considered equal.", A, B);
}

void testNotEqual()
{
  OriginParameter originA(4, 5, 6);
  OriginParameter originB(4, 5, 2); //differs
  WidthParameter widthA(1);
  WidthParameter widthB(2); //differs
  HeightParameter heightA(2);
  HeightParameter heightB(3); //differs
  DepthParameter depthA(3);
  DepthParameter depthB(4); //differs
  BoxImplicitFunction A(widthA, heightA, depthA, originA); //base-line to compare to.
  BoxImplicitFunction B(widthB, heightA, depthA, originA);
  BoxImplicitFunction C(widthA, heightB, depthA, originA);
  BoxImplicitFunction D(widthA, heightA, depthB, originA);
  BoxImplicitFunction E(widthA, heightA, depthA, originB);

  TSM_ASSERT_DIFFERS("These two objects should NOT be considered equal.", A, B);
  TSM_ASSERT_DIFFERS("These two objects should NOT be considered equal.", A, C);
  TSM_ASSERT_DIFFERS("These two objects should NOT be considered equal.", A, D);
  TSM_ASSERT_DIFFERS("These two objects should NOT be considered equal.", A, E);
}


};

#endif
