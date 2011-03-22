#ifndef VECTOR_MATHEMATICS_TEST_H_
#define VECTOR_MATHEMATICS_TEST_H_

#include "MantidGeometry/V3D.h"
#include "MantidMDAlgorithms/VectorMathematics.h"

using namespace Mantid::MDAlgorithms;

class VectorMathematicsTest : public CxxTest::TestSuite
{

public:

	void testDotProductOrthogonal(void)
	{
		//a.b = 0 if a and b are orthogonal.
		double a1 = 0;
		double a2 = 1;
		double a3 = 0;
		double b1 = 1;
		double b2 = 0;
		double b3 = 0;
		double result = dotProduct(a1, a2, a3, b1, b2, b3);
		TSM_ASSERT_EQUALS("The calculated dot product is incorrect", 0, result);
	}
	
	void testDotProductParallel(void)
	{
		//a.b = 1 if a and b are parallel.
		double a1 = 1;
		double a2 = 0;
		double a3 = 0;
		double b1 = 1;
		double b2 = 0;
		double b3 = 0;
		double result = dotProduct(a1, a2, a3, b1, b2, b3);
		TSM_ASSERT_EQUALS("The calculated dot product is incorrect", 1, result);
	}

	void testCrossProductOrthogonal(void)
	{
	  using Mantid::Geometry::V3D;
	  double a1 = 1;
	  double a2 = 0;
	  double a3 = 0;
	  double b1 = 0;
	  double b2 = 1;
	  double b3 = 0;
	  V3D result = crossProduct(a1, a2, a3, b1, b2, b3);
	  TSM_ASSERT_EQUALS("The calculated x component of the cross product is incorrect", 0, result.X());
	  TSM_ASSERT_EQUALS("The calculated y component of the cross product is incorrect", 0, result.Y());
	  TSM_ASSERT_EQUALS("The calculated z component of the cross product is incorrect", 1, result.Z());
	}
	
  void testCrossProductParallel(void)
  {
    using Mantid::Geometry::V3D;
    double a1 = 1;
    double a2 = 0;
    double a3 = 0;
    double b1 = 1;
    double b2 = 0;
    double b3 = 0;
    V3D result = crossProduct(a1, a2, a3, b1, b2, b3);
    TSM_ASSERT_EQUALS("The calculated x component of the cross product is incorrect", 0, result.X());
    TSM_ASSERT_EQUALS("The calculated y component of the cross product is incorrect", 0, result.Y());
    TSM_ASSERT_EQUALS("The calculated z component of the cross product is incorrect", 0, result.Z());
  }

	void testAbsolute(void)
	{
	  double a = 0;
		double b = 3;
		double c = 4;
	  TSM_ASSERT_EQUALS("The magnitude value has been calculated incorrectly", 5, absolute(a, b, c));
	}

	
};

#endif
