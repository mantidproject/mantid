#ifndef PLANE_IMPLICIT_FUNCTION_TEST_H_
#define PLANE_IMPLICIT_FUNCTION_TEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <vector>
#include "PlaneImplicitFunction.h"
#include "NormalParameter.h"
#include "OriginParameter.h"
#include "MDDataObjects/point3D.h"

using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDDataObjects;

class PlaneImplicitFunctionTest : public CxxTest::TestSuite
{
private:
    NormalParameter normal;
	OriginParameter origin;

public:

	PlaneImplicitFunctionTest() : normal(1, 1, 1), origin(2, 3, 4)
	{
	}
	
	void testPlaneImplicitFunctionConstruction(void)
	{
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
	    point3D point(1, 1, 1);
	    PlaneImplicitFunction plane(normal, origin);
		bool isInside = plane.evaluate(&point);
		TSM_ASSERT("The point should have been found to be inside the region bounded by the plane.", isInside);
	}
	
	void testEvaluateInsidePointSwitchNormal()
	{
	    point3D point(1, 1, 1);
		NormalParameter reflectNormal = normal.reflect();
		
	    PlaneImplicitFunction plane(reflectNormal, origin);
		bool isInside = plane.evaluate(&point);
		TSM_ASSERT("The point should not have been found to be inside the region bounded by the plane after the normal was reflected.", !isInside);
	}
	
    void testEvaluateOutsidePoint()
	{
	    point3D point(4, 4, 4);
	    PlaneImplicitFunction plane(normal, origin);
		bool isInside = plane.evaluate(&point);
		TSM_ASSERT("The point should have been found to be outside the region bounded by the plane.", !isInside);
	}
	
	void testEvaluateOutsidePointReflectNormal()
	{
	    point3D point(4, 4, 4);
		NormalParameter reflectNormal = normal.reflect();;
	    PlaneImplicitFunction plane(reflectNormal, origin);
		bool isInside = plane.evaluate(&point);
		TSM_ASSERT("The point should not have been found to be outside the region bounded by the plane after the normal was reflected.", isInside);
	}
	
	void testEvaluateOnPlanePoint()
	{
	    point3D point(origin.getX(), origin.getY(), origin.getZ());
	    PlaneImplicitFunction plane(normal, origin);
		bool isInside = plane.evaluate(&point);
		TSM_ASSERT("The point should have been found to be on the region bounded by the plane.", isInside);
	}
	
	void testEvaluateOnPlanePointReflectNormal()
	{
	    point3D point(origin.getX(), origin.getY(), origin.getZ());
		NormalParameter reflectNormal = normal.reflect();
	    PlaneImplicitFunction plane(reflectNormal, origin);
		bool isInside = plane.evaluate(&point);
		TSM_ASSERT("The point should have been found to be on the region bounded by the plane even after the normal was reflected.", isInside);
	}
	
	void testEvaluateOnPlanePointDecreaseX()
	{

	    point3D point(origin.getX() - 0.0001, origin.getY(), origin.getZ());
	    PlaneImplicitFunction plane(normal, origin);
		bool isInside = plane.evaluate(&point);
		TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point x-value.", isInside);
	}
	
	void testEvaluateOnPlanePointIncreaseX()
	{
	    point3D point(origin.getX() + 0.0001, origin.getY(), origin.getZ());
	    PlaneImplicitFunction plane(normal, origin);
		bool isInside = plane.evaluate(&point);
		TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point x-value.", !isInside);
	}
	
    void testEvaluateOnPlanePointDecreaseY()
	{
	    point3D point(origin.getX(), origin.getY() - 0.0001, origin.getZ());
	    PlaneImplicitFunction plane(normal, origin);
		bool isInside = plane.evaluate(&point);
		TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point y-value.", isInside);
	}
	
	void testEvaluateOnPlanePointIncreaseY()
	{
	    point3D point(origin.getX(), origin.getY() + 0.0001, origin.getZ());
	    PlaneImplicitFunction plane(normal, origin);
		bool isInside = plane.evaluate(&point);
		TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point y-value.", !isInside);
	}

	    void testEvaluateOnPlanePointDecreaseZ()
	{
	    point3D point(origin.getX(), origin.getY(), origin.getZ() - 0.0001);
	    PlaneImplicitFunction plane(normal, origin);
		bool isInside = plane.evaluate(&point);
		TSM_ASSERT("The point (while on the plane origin) should have been found to be inside the region bounded by the plane after an incremental decrease in the point z-value.", isInside);
	}
	
	void testEvaluateOnPlanePointIncreaseZ()
	{
	    point3D point(origin.getX(), origin.getY(), origin.getZ() + 0.0001);
	    PlaneImplicitFunction plane(normal, origin);
		bool isInside = plane.evaluate(&point);
		TSM_ASSERT("The point (while on the plane origin) should have been found to be outside the region bounded by the plane after an incremental increase in the point z-value.", !isInside);
	}

	
};

#endif