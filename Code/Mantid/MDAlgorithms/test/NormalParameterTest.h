#ifndef TEST_NORMAL_PARAMETER_H_
#define TEST_NORMAL_PARAMETER_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <memory>
#include "NormalParameter.h"

using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDDataObjects;

class NormalParameterTest : public CxxTest::TestSuite
{
 public:
	
	void testCreate()
	{
	  NormalParameter normal(0, 1, 2);
	  TSM_ASSERT_EQUALS("NormalParameter getX() is not wired-up correctly.", 0, normal.getX() );
	  TSM_ASSERT_EQUALS("NormalParameter getY() is not wired-up correctly.", 1, normal.getY() );
	  TSM_ASSERT_EQUALS("NormalParameter getZ() is not wired-up correctly.", 2, normal.getZ() );
	}
	
	void testIsValid()
	{
	  NormalParameter normal(0, 0, 0);
	  TSM_ASSERT_EQUALS("The NormalParameter should be valid.", true, normal.isValid());
	}
	
	void testClone()
	{
	  NormalParameter original(0, 1, 2);
	  std::auto_ptr<NormalParameter> cloned = original.clone();
	  
	  TSM_ASSERT_EQUALS("Cloned NormalParameter getX() is not same as original.", 0, cloned->getX() );
	  TSM_ASSERT_EQUALS("Cloned NormalParameter getY() is not same as original.", 1, cloned->getY() );
	  TSM_ASSERT_EQUALS("Cloned NormalParameter getZ() is not same as original.", 2, cloned->getZ() );
	}
	
	void testCopy()
	{
	  NormalParameter original(0, 1, 2);
	  NormalParameter copy(original);
	  TSM_ASSERT_EQUALS("Copied NormalParameter getX() is not same as original.", 0, copy.getX() );
	  TSM_ASSERT_EQUALS("Copied NormalParameter getY() is not same as original.", 1, copy.getY() );
	  TSM_ASSERT_EQUALS("Copied NormalParameter getZ() is not same as original.", 2, copy.getZ() );
	}
	
	void testGetNameFunctionsEquivalent()
	{
	  NormalParameter normal(0, 0, 0);
	  TSM_ASSERT_EQUALS("The static name and the dynamic name of the NormalParameter do not match.", normal.getName(),  NormalParameter::parameterName())
	}
	
	void testReflect()
	{
	  NormalParameter normal(1, 2, 3);
	  NormalParameter reflected = normal.reflect();
	  
	  TSM_ASSERT_EQUALS("Reflected normal x value is not negative of original.", -1, reflected.getX() );
	  TSM_ASSERT_EQUALS("Reflected normal y value is not negative of original.", -2, reflected.getY() );
	  TSM_ASSERT_EQUALS("Reflected normal z value is not negative of original.", -3, reflected.getZ() );
	}
	
};

#endif