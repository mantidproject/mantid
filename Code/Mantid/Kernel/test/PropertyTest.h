#ifndef PROPERTYTEST_H_
#define PROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Property.h"

using namespace Mantid::Kernel;

// Helper class
class PropertyHelper : public Property
{
public:
  PropertyHelper() : Property( "Test", typeid( int ) ) {}
  std::string value() const { return "Nothing"; }
  bool setValue( const std::string& value ) { return true; }
};

class PropertyTest : public CxxTest::TestSuite
{
public:
  PropertyTest()
  {
    p = new PropertyHelper;
  }
  
	void testName()
	{
		TS_ASSERT( ! p->name().compare("Test") )
	}

	void testDocumentation()
	{
	  Property *pp = new PropertyHelper;
	  TS_ASSERT( ! pp->documentation().compare("") )
	}

	void testType_info()
	{
	  TS_ASSERT( typeid( int ) == *p->type_info()  )
	}

	void testType()
	{
#ifdef __GNUC__
		TS_ASSERT( ! p->type().compare("i") )
#else
    TS_ASSERT( ! p->type().compare("int") )
#endif		
	}

	void testIsValid()
	{
		TS_ASSERT( p->isValid() )
	}

	void testIsDefault()
	{
    TS_ASSERT( p->isDefault() )
	}

	void testSetDocumentation()
	{
	  const std::string str("Documentation comment");
	  p->setDocumentation(str);
	  TS_ASSERT( ! p->documentation().compare(str) )
	}

private:
  Property *p;
	
};

#endif /*PROPERTYTEST_H_*/
