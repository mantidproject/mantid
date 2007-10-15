#ifndef DYNAMICFACTORYTEST_H_
#define DYNAMICFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/DynamicFactory.h"

using namespace Mantid;

// Helper class
class AFactory : public DynamicFactory<int>
{
};

class DynamicFactoryTest : public CxxTest::TestSuite
{
public:
	void testCreate()
	{
	  TS_ASSERT_THROWS( factory.create("testEntry"), std::runtime_error )
	  factory.subscribe<int>("testEntry");
	  TS_ASSERT_THROWS_NOTHING( int *i = factory.create("testEntry") );
	}

	void testSubscribe()
	{
		TS_ASSERT_THROWS_NOTHING( factory.subscribe<int>("int") );
		TS_ASSERT_THROWS_NOTHING( factory.subscribe("int2",new Instantiator<int, int>));
		TS_ASSERT_THROWS( factory.subscribe<int>("int"), std::runtime_error);
	}

	void testUnsubscribe()
	{
	  TS_ASSERT_THROWS( factory.unsubscribe("tester"), std::runtime_error);
	  factory.subscribe<int>("tester");
	  TS_ASSERT_THROWS_NOTHING( factory.unsubscribe("tester"));
	}

	void testExists()
	{
	  TS_ASSERT( ! factory.exists("testing") );
		factory.subscribe<int>("testing");
		TS_ASSERT( factory.exists("testing") );
	}

private:
  AFactory factory;
	
};

#endif /*DYNAMICFACTORYTEST_H_*/
