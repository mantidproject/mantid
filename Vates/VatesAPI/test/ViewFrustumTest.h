#ifndef VIEWFRUSTUM_TEST_H_
#define VIEWFRUSTUM_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/ViewFrustum.h"

using namespace Mantid::VATES;

class ViewFrustumTest: public CxxTest::TestSuite
{
public:
  void testThatExtentsAreFoundForStandardFrustum()
  {
    // Arrange
    // Create a standard cube
    LeftPlane left(1.0, 0.0, 0.0, 1.0);
    RightPlane right(-1.0, 0.0, 0.0, 1.0);

    BottomPlane bottom(0.0, 1.0, 0.0, 1.0);
    TopPlane top(0.0, -1.0, 0.0, 1.0);

    FarPlane far(0.0, 0.0, 1.0, 1.0);
    NearPlane near(0.0, 0.0, -1.0,1.0);

    ViewFrustum frustum(left, right, bottom, top, far, near);

    //Act 
    std::vector<std::pair<double, double>> extents;
    TSM_ASSERT_THROWS_NOTHING("Frustum is well defined, should not throw.", extents = frustum.toExtents());

    //Assert
    TSM_ASSERT_EQUALS("Extents should exist for x, y and z.", 3, extents.size());
    TSM_ASSERT_EQUALS("Frustum is well defined and should have xmin = -1", -1.0, extents[0].first);
    TSM_ASSERT_EQUALS("Frustum is well defined and should have xmax = 1", 1.0, extents[0].second);
    TSM_ASSERT_EQUALS("Frustum is well defined and should have ymin = -1", -1.0, extents[1].first);
    TSM_ASSERT_EQUALS("Frustum is well defined and should have ymin = -1", 1.0, extents[1].second);
    TSM_ASSERT_EQUALS("Frustum is well defined and should have zmin = -1", -1.0, extents[2].first);
    TSM_ASSERT_EQUALS("Frustum is well defined and should have zmax = 1", 1.0, extents[2].second);
  }

  void testThatExtentsAreFoundForFrustumWithRotation()
  {
    // Arrange
    // Create skewed cube
    LeftPlane left(1.0, -0.5, 0.0, 1.0);
    RightPlane right(-1.0, 0.5, 0.0, 1.0);

    BottomPlane bottom(1.0, 0.5, 0.0, 1.0);
    TopPlane top(-1.0, -0.5, 0.0, 1.0);

    FarPlane far(0.0, 0.0, 1.0, 1.0);
    NearPlane near(0.0, 0.0, -1.0,1.0);

    ViewFrustum frustum(left, right, bottom, top, far, near);

    //Act 
    std::vector<std::pair<double, double>> extents;
    TSM_ASSERT_THROWS_NOTHING("Frustum is well defined, should not throw.", extents = frustum.toExtents());

    //Assert
    TSM_ASSERT_EQUALS("Extents should exist for x, y and z.", 3, extents.size());
    TSM_ASSERT_EQUALS("Frustum is well defined and should have xmin = -1", -1.0, extents[0].first);
    TSM_ASSERT_EQUALS("Frustum is well defined and should have xmax = 1", 1.0, extents[0].second);
    TSM_ASSERT_EQUALS("Frustum is well defined and should have ymin = -1", -2.0, extents[1].first);
    TSM_ASSERT_EQUALS("Frustum is well defined and should have ymin = -1", 2.0, extents[1].second);
    TSM_ASSERT_EQUALS("Frustum is well defined and should have zmin = -1", -1.0, extents[2].first);
    TSM_ASSERT_EQUALS("Frustum is well defined and should have zmax = 1", 1.0, extents[2].second);
  }

  void testThatWrongPlanesThrowErrors()
  {
    // Arrange
    // Just have one plane type. This should fail the calculation of intersection points 
    LeftPlane left(1.0, -0.5, 0.0, 1.0);
    RightPlane right(1.0, -0.5, 0.0, 1.0);

    BottomPlane bottom(1.0, -0.5, 0.0, 1.0);
    TopPlane top(1.0, -0.5, 0.0, 1.0);

    FarPlane far(1.0, -0.5, 0.0, 1.0);
    NearPlane near(1.0, -0.5, 0.0, 1.0);

    ViewFrustum frustum(left, right, bottom, top, far, near);

    //Assert
    TSM_ASSERT_THROWS("Frustum is not well defined, should throw error",frustum.toExtents(), std::runtime_error);
  }

};

#endif