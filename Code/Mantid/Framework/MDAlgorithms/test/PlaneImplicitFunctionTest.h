#ifndef PLANE_IMPLICIT_FUNCTION_TEST_H_
#define PLANE_IMPLICIT_FUNCTION_TEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>
#include <MantidKernel/Matrix.h>
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include <boost/shared_ptr.hpp>

//=====================================================================================
// Helper Types
//=====================================================================================
namespace
{
  class MockPoint3D: public Mantid::API::Point3D
  {
  public:
    MOCK_CONST_METHOD0 (getX, double());
    MOCK_CONST_METHOD0(getY, double());
    MOCK_CONST_METHOD0(getZ, double());
    virtual ~MockPoint3D(){}
  };
}

//=====================================================================================
// Functional Tests
//=====================================================================================
class PlaneImplicitFunctionTest: public CxxTest::TestSuite
{
private:

  Mantid::MDAlgorithms::NormalParameter normal;
  Mantid::MDAlgorithms::OriginParameter origin;
  Mantid::MDAlgorithms::WidthParameter width;
  const double PI;

public:

  PlaneImplicitFunctionTest() :
    normal(1, 1, 1), origin(2, 3, 4), width(2), PI(3.14159265)
  {
  }

  void testPlaneImplicitFunctionConstruction(void)
  {
    using namespace Mantid::MDAlgorithms;
    NormalParameter normalParam(1, 0, 0);

    PlaneImplicitFunction plane(normalParam, origin, width);
    TSM_ASSERT_EQUALS("Normal x component not wired-up correctly", 1, plane.getNormalX());
    TSM_ASSERT_EQUALS("Normal y component not wired-up correctly", 0, plane.getNormalY());
    TSM_ASSERT_EQUALS("Normal z component not wired-up correctly", 0, plane.getNormalZ());
    TSM_ASSERT_EQUALS("Origin x component not wired-up correctly", 2, plane.getOriginX());
    TSM_ASSERT_EQUALS("Origin y component not wired-up correctly", 3, plane.getOriginY());
    TSM_ASSERT_EQUALS("Origin z component not wired-up correctly", 4, plane.getOriginZ());
    TSM_ASSERT_EQUALS("Width component not wired-up correctly", 2, plane.getWidth());

  }

  void testEvaluateInsidePointOnForwardSurface()
  {
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1));
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2));
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3));

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary
    PlaneImplicitFunction plane(tNormal, tOrigin, tWidth);

    //Test using point3D API.
    TSM_ASSERT("The point should have been found to be inside the region bounded by the plane.", plane.evaluate(&point));
    
    //Test exactly same schenario using coordinate API.
    Mantid::coord_t coords[3] = {1,2,3};
    bool masks[3] = {false, false, false};
    TSM_ASSERT("The point should have been found to be inside the region bounded by the plane.", plane.evaluate(coords, masks, 3));
  }

  void testEvaluateInsidePointOnBackwardSurface()
  {
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(-1));
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(-2));
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(-3));

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    PlaneImplicitFunction plane(tNormal, tOrigin, tWidth);
    
    //Test using point3D API.
    TSM_ASSERT("The point should have been found to be inside the region bounded by the plane.", plane.evaluate(&point));
    
    //Test exactly same schenario using coordinate API.
    Mantid::coord_t coords[3] = {-1,-2,-3};
    bool masks[3] = {false, false, false};
    TSM_ASSERT("The point should have been found to be inside the region bounded by the plane.", plane.evaluate(coords, masks, 3));
  }

  void testEvaluateInsidePointReflectNormal() //Test that plane automatically relects normals where necessary.
  {
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1));
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2));
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3));

    NormalParameter tNormal(1, 2, 3);
    NormalParameter rNormal = tNormal.reflect();
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    PlaneImplicitFunction plane(rNormal, tOrigin, tWidth);

    TSM_ASSERT("The point should have been found to be inside the region bounded by the plane after the normal was reflected.", plane.evaluate(&point));

    //Test exactly same schenario using coordinate API.
    Mantid::coord_t coords[3] = {1,2,3};
    bool masks[3] = {false, false, false};
    TSM_ASSERT("The point should have been found to be inside the region bounded by the plane after the normal was reflected.", plane.evaluate(coords, masks, 3));
  }

  void testEvaluatePointOutsideForwardPlane()
  {
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1.001)); //Just outside
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2.001)); //Just outside
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3.001)); //Just outside

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    PlaneImplicitFunction plane(tNormal, tOrigin, tWidth);
    TSM_ASSERT("The point should have been found to be outside the region bounded by the plane.", !plane.evaluate(&point));

     //Test exactly same schenario using coordinate API.
    Mantid::coord_t coords[3] = {1.001,2.001,3.001};
    bool masks[3] = {false, false, false};
    TSM_ASSERT("The point should have been found to be outside the region bounded by the plane.", !plane.evaluate(coords, masks, 3));
  }

  void testEvaluatePointOutsideBackwardPlane()
  {
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(-1.001)); //Just outside
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(-2.001)); //Just outside
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(-3.001)); //Just outside

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    PlaneImplicitFunction plane(tNormal, tOrigin, tWidth);

    TSM_ASSERT("The point should have been found to be outside the region bounded by the plane.", !plane.evaluate(&point));

     //Test exactly same schenario using coordinate API.
    Mantid::coord_t coords[3] = {-1.001,-2.001,-3.001};
    bool masks[3] = {false, false, false};
    TSM_ASSERT("The point should have been found to be outside the region bounded by the plane.", !plane.evaluate(coords, masks, 3));
  }

  void testEvaluateOnPlanePointDecreaseX()
  {
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(0.999)); //Just inside
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2)); //Just on
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3)); //Just on

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    PlaneImplicitFunction plane(tNormal, tOrigin, tWidth);

    TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point x-value.", plane.evaluate(&point));

    //Test exactly same schenario using coordinate API.
    Mantid::coord_t coords[3] = {0.999,2,3};
    bool masks[3] = {false, false, false};
    TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point x-value.", plane.evaluate(coords, masks, 3));
  }

  void testEvaluateOnPlanePointIncreaseX()
  {
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1.001)); //Just outside
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2)); //Just on
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3)); //Just on

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    PlaneImplicitFunction plane(tNormal, tOrigin, tWidth);

    TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point x-value.", !plane.evaluate(&point));
    
    //Test exactly same schenario using coordinate API.
    Mantid::coord_t coords[3] = {1.001,2,3};
    bool masks[3] = {false, false, false};
    TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point x-value.", !plane.evaluate(coords, masks, 3));
 
  }

  void testEvaluateOnPlanePointDecreaseY()
  {
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1)); //Just on
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(1.999)); //Just inside
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3)); //Just on

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    PlaneImplicitFunction plane(tNormal, tOrigin, tWidth);

    TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point y-value.", plane.evaluate(&point));
  
    //Test exactly same schenario using coordinate API.
    Mantid::coord_t coords[3] = {1, 1.999,3};
    bool masks[3] = {false, false, false};
    TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point y-value.", plane.evaluate(coords, masks, 3));
 
  }

  void testEvaluateOnPlanePointIncreaseY()
  {
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1)); //Just on
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2.001)); //Just outside
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3)); //Just on

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    PlaneImplicitFunction plane(tNormal, tOrigin, tWidth);

    TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point y-value.", !plane.evaluate(&point));
  
    //Test exactly same schenario using coordinate API.
    Mantid::coord_t coords[3] = {1,2.001,3};
    bool masks[3] = {false, false, false};
    TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point y-value.", !plane.evaluate(coords, masks, 3));
 
  }

  void testEvaluateOnPlanePointDecreaseZ()
  {
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1)); //Just on
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2)); //Just on
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(2.999)); //Just inside

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    PlaneImplicitFunction plane(tNormal, tOrigin, tWidth);

    TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point z-value.", plane.evaluate(&point));
  
    //Test exactly same schenario using coordinate API.
    Mantid::coord_t coords[3] = {1, 2, 2.999};
    bool masks[3] = {false, false, false};
    TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point z-value.", plane.evaluate(coords, masks, 3));
 
  }

  void testEvaluateOnPlanePointIncreaseZ()
  {
    using namespace Mantid::MDAlgorithms;

    MockPoint3D point;
    EXPECT_CALL(point, getX()).Times(2).WillRepeatedly(testing::Return(1)); //Just on
    EXPECT_CALL(point, getY()).Times(2).WillRepeatedly(testing::Return(2)); //Just on
    EXPECT_CALL(point, getZ()).Times(2).WillRepeatedly(testing::Return(3.001)); //Just outside

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    PlaneImplicitFunction plane(tNormal, tOrigin, tWidth);

    TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point z-value.", !plane.evaluate(&point));
  
    //Test exactly same schenario using coordinate API.
    Mantid::coord_t coords[3] = {1, 2, 3.001};
    bool masks[3] = {false, false, false};
    TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point z-value.", !plane.evaluate(coords, masks, 3));
  }

  void testToXML()
  {
    using namespace Mantid::MDAlgorithms;
    NormalParameter tNormal(1, 0, 0);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(3);
    PlaneImplicitFunction plane(tNormal, tOrigin, tWidth);
    TSM_ASSERT_EQUALS("The xml generated by this function did not match the expected schema.", "<Function><Type>PlaneImplicitFunction</Type><ParameterList><Parameter><Type>NormalParameter</Type><Value>1.0000, 0.0000, 0.0000</Value></Parameter><Parameter><Type>OriginParameter</Type><Value>0.0000, 0.0000, 0.0000</Value></Parameter><Parameter><Type>WidthParameter</Type><Value>3.0000</Value></Parameter></ParameterList></Function>", plane.toXMLString());
  }

  void testEqual()
  {
    using namespace Mantid::MDAlgorithms;
    NormalParameter n(1, 2, 3);
    OriginParameter o(4, 5, 6);
    WidthParameter width(10);
    PlaneImplicitFunction A(n, o,  width);
    PlaneImplicitFunction B(n, o,  width);
    TSM_ASSERT_EQUALS("These two objects should be considered equal.", A, B);
  }

  void testNotEqual()
  {
    using namespace Mantid::MDAlgorithms;
    NormalParameter n1(1, 2, 3);
    OriginParameter o1(4, 5, 6);
    WidthParameter width1(10);
    NormalParameter n2(0, 0, 0);
    OriginParameter o2(0, 0, 0);
    WidthParameter width2(0);
    PlaneImplicitFunction A(n1, o1, width1); //Base comparison
    PlaneImplicitFunction B(n2, o1, width1); //Differ normal only
    PlaneImplicitFunction C(n1, o2, width1); //Differ origin only
    PlaneImplicitFunction D(n1, o1, width2); //Differ width only
    TSM_ASSERT_DIFFERS("These two objects should not be considered equal.", A, B);
    TSM_ASSERT_DIFFERS("These two objects should not be considered equal.", A, C);
    TSM_ASSERT_DIFFERS("These two objects should not be considered equal.", A, D);
  }

  void testTooFewCoords()
  {
    using namespace Mantid::MDAlgorithms;

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(1); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    PlaneImplicitFunction plane(tNormal, tOrigin, tWidth);

    Mantid::coord_t coords[4] = {1, 1, 1, 1};
    bool masks[4] = {true, true, false, false}; // only leaves two dimensions unmasked.

    TSM_ASSERT_THROWS("Should throw because it must have 3 unmasked coords to work.", plane.evaluate(coords, masks, 4), std::runtime_error);
  }

  void testTooManyCoords()
  {
    using namespace Mantid::MDAlgorithms;

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(1); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary

    PlaneImplicitFunction plane(tNormal, tOrigin, tWidth);

    Mantid::coord_t coords[4] = {1, 1, 1, 1};
    bool masks[4] = {false, false, false, false}; // 4 unmasked dimensions

    TSM_ASSERT_THROWS("Should throw because it must have 3 unmasked coords to work.", plane.evaluate(coords, masks, 4), std::runtime_error);
  }

};

//=====================================================================================
// Performance Tests
//=====================================================================================
class PlaneImplicitFunctionTestPerformance : public CxxTest::TestSuite
{
private:

  boost::shared_ptr<Mantid::MDAlgorithms::PlaneImplicitFunction> m_plane;

public:

  void setUp()
  {
    using namespace Mantid::MDAlgorithms;

    NormalParameter tNormal(1, 2, 3);
    OriginParameter tOrigin(0, 0, 0);
    WidthParameter tWidth(std::sqrt((1 * 1) + (2 * 2.0) + (3 * 3)) * 2.0); // Width set up so that points 1,2,3 and -1,-2,-3 are within boundary
    m_plane = boost::shared_ptr<Mantid::MDAlgorithms::PlaneImplicitFunction>(new PlaneImplicitFunction(tNormal, tOrigin, tWidth));
  }

  /// Test using point3D API.
  void testMultipleExecutionFromPoint3D()
  {
    MockPoint3D point;
    EXPECT_CALL(point, getX()).WillRepeatedly(testing::Return(1));
    EXPECT_CALL(point, getY()).WillRepeatedly(testing::Return(2));
    EXPECT_CALL(point, getZ()).WillRepeatedly(testing::Return(3));

    bool bIsInside = true;
    //Ten thousand runs should give a good benchmark.
    for(size_t i = 0; i < 10000; i++)
    {
      bIsInside = m_plane->evaluate(&point);
    }
    TS_ASSERT(bIsInside);
  }

  void testMultipleExecutionFromCoords()
  {
    //Test exactly same schenario using coordinate API.
    Mantid::coord_t coords[3] = {1,2,3};
    bool masks[3] = {false, false, false};
    bool bIsInside = true;
    //Ten thousand runs should give a good benchmark.
    for(size_t i = 0; i < 10000; i++)
    {
      bIsInside = m_plane->evaluate(coords, masks, 3);
    }
    TS_ASSERT(bIsInside);
  }

};
#endif
