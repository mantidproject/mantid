#ifndef VECTOR_MATHEMATICS_TEST_H_
#define VECTOR_MATHEMATICS_TEST_H_


#include "VectorMathematics.h"

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
		std::vector<double> result;
		dotProduct<double>(a1, a2, a3, b1, b2, b3, result);
		TSM_ASSERT_EQUALS("The calculated x component of the dot product is incorrect", 0, result.at(0));
		TSM_ASSERT_EQUALS("The calculated y component of the dot product is incorrect", 0, result.at(1));
		TSM_ASSERT_EQUALS("The calculated z component of the dot product is incorrect", 0, result.at(2));
	}
	
	void testDotProductParallel(void)
	{
		
		double a1 = 1;
		double a2 = 2;
		double a3 = 3;
		double b1 = 1;
		double b2 = 2;
		double b3 = 3;
		std::vector<double> result;
		dotProduct<double>(a1, a2, a3, b1, b2, b3, result);
		TSM_ASSERT_EQUALS("The calculated x component of the dot product is incorrect", 1, result.at(0));
		TSM_ASSERT_EQUALS("The calculated y component of the dot product is incorrect", 4, result.at(1));
		TSM_ASSERT_EQUALS("The calculated z component of the dot product is incorrect", 9, result.at(2));
	}
	
	void testAbsolute(void)
	{
	    double a = 0;
		double b = 3;
		double c = 4;
	    TSM_ASSERT_EQUALS("The magnitude value has been calculated incorrectly", 5, absolute<double>(a, b, c));
	}

	
};

#endif