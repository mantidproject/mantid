#ifndef NULLVALIDATORTEST_H_
#define NULLVALIDATORTEST_H_

#include <string>
#include <cxxtest/TestSuite.h>
#include "MantidKernel/NullValidator.h"

using namespace Mantid::Kernel;

class NullValidatorTest : public CxxTest::TestSuite
{
public:

	void testConstructor()
	{
    TS_ASSERT_THROWS_NOTHING(
		NullValidator<int> niv;
		NullValidator<double> ndv;
    NullValidator<std::string> nsv;
    );
	}
	void testIntNullValidator()
	{
		NullValidator<int> p;
		TS_ASSERT_EQUALS(p.isValid(0), true);
		TS_ASSERT_EQUALS(p.isValid(1), true);
		TS_ASSERT_EQUALS(p.isValid(10), true);
		TS_ASSERT_EQUALS(p.isValid(-11), true);
	}

	void testDoubleNullValidator()
	{
		NullValidator<double> p;
		TS_ASSERT_EQUALS(p.isValid(0.0), true);
		TS_ASSERT_EQUALS(p.isValid(1.0), true);
		TS_ASSERT_EQUALS(p.isValid(10.0), true);
		TS_ASSERT_EQUALS(p.isValid(-10.1), true);
	}
	
	void testStringNullValidator()
	{
		NullValidator<std::string> p;
		TS_ASSERT_EQUALS(p.isValid("AZ"), true);
		TS_ASSERT_EQUALS(p.isValid("B"), true);
		TS_ASSERT_EQUALS(p.isValid(""), true);
		TS_ASSERT_EQUALS(p.isValid("ta"), true);
	}

};

#endif /*NULLVALIDATORTEST_H_*/
