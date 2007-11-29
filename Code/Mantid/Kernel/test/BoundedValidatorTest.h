#ifndef BOUNDEDVALIDATORTEST_H_
#define BOUNDEDVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/BoundedValidator.h"

using namespace Mantid::Kernel;

class BoundedValidatorTest : public CxxTest::TestSuite
{
public:

	void testConstructor()
	{
		BoundedValidator<int> bv(2,5);
		// Test that all the base class member variables are correctly assigned to
		TS_ASSERT_EQUALS( bv.hasLower(), true );
		TS_ASSERT_EQUALS( bv.hasUpper(), true );
		TS_ASSERT_EQUALS( bv.lower(), 2 );
		TS_ASSERT_EQUALS( bv.upper(), 5 );
	}

	void testIntClear()
	{
		BoundedValidator<int> bv(2,5);
		TS_ASSERT_EQUALS( bv.hasLower(), true );
		TS_ASSERT_EQUALS( bv.hasUpper(), true );
		TS_ASSERT_EQUALS( bv.lower(), 2 );
		TS_ASSERT_EQUALS( bv.upper(), 5 );
		
		bv.clearLower();
		TS_ASSERT_EQUALS( bv.hasLower(), false );
		TS_ASSERT_EQUALS( bv.hasUpper(), true );
		TS_ASSERT_EQUALS( bv.lower(), 0 );
		TS_ASSERT_EQUALS( bv.upper(), 5 );

		bv.clearUpper();
		TS_ASSERT_EQUALS( bv.hasLower(), false );
		TS_ASSERT_EQUALS( bv.hasUpper(), false );
		TS_ASSERT_EQUALS( bv.lower(), 0 );
		TS_ASSERT_EQUALS( bv.upper(), 0 );
	}

	void testDoubleClear()
	{
		BoundedValidator<double> bv(2.0,5.0);
		TS_ASSERT_EQUALS( bv.hasLower(), true );
		TS_ASSERT_EQUALS( bv.hasUpper(), true );
		TS_ASSERT_EQUALS( bv.lower(), 2.0 );
		TS_ASSERT_EQUALS( bv.upper(), 5.0 );

		bv.clearBounds();
		TS_ASSERT_EQUALS( bv.hasLower(), false );
		TS_ASSERT_EQUALS( bv.hasUpper(), false );
		TS_ASSERT_EQUALS( bv.lower(), 0 );
		TS_ASSERT_EQUALS( bv.upper(), 0 );
	}

	void testSetBounds()
	{
		BoundedValidator<std::string> bv("A","B");
		TS_ASSERT_EQUALS( bv.hasLower(), true );
		TS_ASSERT_EQUALS( bv.hasUpper(), true );
		TS_ASSERT_EQUALS( bv.lower(), "A" );
		TS_ASSERT_EQUALS( bv.upper(), "B" );

		bv.clearBounds();
		TS_ASSERT_EQUALS( bv.hasLower(), false );
		TS_ASSERT_EQUALS( bv.hasUpper(), false );
		TS_ASSERT_EQUALS( bv.lower(), "" );
		TS_ASSERT_EQUALS( bv.upper(), "" );

		bv.setBounds("C","D");
		TS_ASSERT_EQUALS( bv.hasLower(), true );
		TS_ASSERT_EQUALS( bv.hasUpper(), true );
		TS_ASSERT_EQUALS( bv.lower(), "C" );
		TS_ASSERT_EQUALS( bv.upper(), "D" );
	}

	void testSetValues()
	{
		BoundedValidator<std::string> bv("A","B");
		TS_ASSERT_EQUALS( bv.hasLower(), true );
		TS_ASSERT_EQUALS( bv.hasUpper(), true );
		TS_ASSERT_EQUALS( bv.lower(), "A" );
		TS_ASSERT_EQUALS( bv.upper(), "B" );

		bv.clearBounds();
		TS_ASSERT_EQUALS( bv.hasLower(), false );
		TS_ASSERT_EQUALS( bv.hasUpper(), false );
		TS_ASSERT_EQUALS( bv.lower(), "" );
		TS_ASSERT_EQUALS( bv.upper(), "" );

		bv.setLower("C");
		bv.setUpper("D");
		TS_ASSERT_EQUALS( bv.hasLower(), true );
		TS_ASSERT_EQUALS( bv.hasUpper(), true );
		TS_ASSERT_EQUALS( bv.lower(), "C" );
		TS_ASSERT_EQUALS( bv.upper(), "D" );

		bv.setUpper("E");
		TS_ASSERT_EQUALS( bv.hasLower(), true );
		TS_ASSERT_EQUALS( bv.hasUpper(), true );
		TS_ASSERT_EQUALS( bv.lower(), "C" );
		TS_ASSERT_EQUALS( bv.upper(), "E" );

	}

	void testIntBoundedValidator()
	{
		BoundedValidator<int> p(1,10);
		TS_ASSERT_EQUALS(p.isValid(0), false);
		TS_ASSERT_EQUALS(p.isValid(1), true);
		TS_ASSERT_EQUALS(p.isValid(10), true);
		TS_ASSERT_EQUALS(p.isValid(11), false);

		p.clearLower();
		TS_ASSERT_EQUALS(p.isValid(0), true);
		TS_ASSERT_EQUALS(p.isValid(-1), true);
		TS_ASSERT_EQUALS(p.isValid(10), true);
		TS_ASSERT_EQUALS(p.isValid(11), false);
		p.clearUpper();
		TS_ASSERT_EQUALS(p.isValid(11), true);
	}

	void testDoubleBoundedValidator()
	{
		BoundedValidator<double> p(1.0,10.0);
		TS_ASSERT_EQUALS(p.isValid(0.9), false);
		TS_ASSERT_EQUALS(p.isValid(1.0), true);
		TS_ASSERT_EQUALS(p.isValid(10.0), true);
		TS_ASSERT_EQUALS(p.isValid(10.1), false);

		p.clearLower();
		TS_ASSERT_EQUALS(p.isValid(0.9), true);
		TS_ASSERT_EQUALS(p.isValid(-1.0), true);
		TS_ASSERT_EQUALS(p.isValid(10), true);
		TS_ASSERT_EQUALS(p.isValid(10.1), false);
		p.clearUpper();
		TS_ASSERT_EQUALS(p.isValid(10.1), true);
	}
	
	void testStringBoundedValidator()
	{
		BoundedValidator<std::string> p("B","T");
		TS_ASSERT_EQUALS(p.isValid("AZ"), false);
		TS_ASSERT_EQUALS(p.isValid("B"), true);
		TS_ASSERT_EQUALS(p.isValid("T"), true);
		TS_ASSERT_EQUALS(p.isValid("TA"), false);

		p.clearLower();
		TS_ASSERT_EQUALS(p.isValid("AZ"), true);
		TS_ASSERT_EQUALS(p.isValid("B"), true);
		TS_ASSERT_EQUALS(p.isValid("T"), true);
		TS_ASSERT_EQUALS(p.isValid("TA"), false);
		p.clearUpper();
		TS_ASSERT_EQUALS(p.isValid("TA"), true);
	}

};

#endif /*BOUNDEDVALIDATORTEST_H_*/
