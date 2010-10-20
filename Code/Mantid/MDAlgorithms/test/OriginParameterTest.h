#ifndef TEST_ORIGIN_PARAMETER_H_
#define TEST_ORIGIN_PARAMETER_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <memory>
#include "OriginParameter.h"

using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDDataObjects;

class OriginParameterTest : public CxxTest::TestSuite
{
 public:
	
    void testCreate()
	{
	  OriginParameter origin(0, 1, 2);
	  TSM_ASSERT_EQUALS("OriginParameter getX() is not wired-up correctly.", 0, origin.getX() );
	  TSM_ASSERT_EQUALS("OriginParameter getY() is not wired-up correctly.", 1, origin.getY() );
	  TSM_ASSERT_EQUALS("OriginParameter getZ() is not wired-up correctly.", 2, origin.getZ() );
	}
	
	void testIsValid()
	{
	  OriginParameter origin(0, 0, 0);
	  TSM_ASSERT_EQUALS("The OriginParameter should be valid.", true, origin.isValid());
	}
	
	void testClone()
	{
	  OriginParameter original(0, 1, 2);
	  std::auto_ptr<OriginParameter> cloned = original.clone();
	  
	  TSM_ASSERT_EQUALS("Cloned OriginParameter getX() is not same as original.", 0, cloned->getX() );
	  TSM_ASSERT_EQUALS("Cloned OriginParameter getY() is not same as original.", 1, cloned->getY() );
	  TSM_ASSERT_EQUALS("Cloned OriginParameter getZ() is not same as original.", 2, cloned->getZ() );
	}
	
	void testCopy()
	{
	  OriginParameter original(0, 1, 2);
	  OriginParameter copy(original);
	  TSM_ASSERT_EQUALS("Copied OriginParameter getX() is not same as original.", 0, copy.getX() );
	  TSM_ASSERT_EQUALS("Copied OriginParameter getY() is not same as original.", 1, copy.getY() );
	  TSM_ASSERT_EQUALS("Copied OriginParameter getZ() is not same as original.", 2, copy.getZ() );
	}
	
	void testGetNameFunctionsEquivalent()
	{
	  OriginParameter origin(0, 0, 0);
	  TSM_ASSERT_EQUALS("The static name and the dynamic name of the OriginParameter do not match.", origin.getName(),  OriginParameter::parameterName())
	}
	
	
};

#endif