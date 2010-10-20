#ifndef TEST_INVALID_PARAMETER_H_
#define TEST_INVALID_PARAMETER_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include <memory>
#include "InvalidParameter.h"

using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDDataObjects;

class InvalidParameterTest : public CxxTest::TestSuite
{
 public:
	
	void testIsValid()
	{
	  InvalidParameter invalidParam;
	  TSM_ASSERT_EQUALS("The InvalidParameter should always be invalid", false, invalidParam.isValid());
	}
	
	void testClone()
	{
	  InvalidParameter original;
	  std::auto_ptr<InvalidParameter> cloned = original.clone();
	}
	
	void testCopy()
	{
	  InvalidParameter original;
	  InvalidParameter copy(original);
	}
	
	void testGetNameFunctionsEquivalent()
	{
	  InvalidParameter origin;
	  TSM_ASSERT_EQUALS("The static name and the dynamic name of the InvalidParameter do not match.", origin.getName(),  InvalidParameter::parameterName())
	}
	
};

#endif