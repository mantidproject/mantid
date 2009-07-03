#ifndef DYNAMICFACTORYTEST_H_
#define DYNAMICFACTORYTEST_H_

#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>
#include <vector>
#include <string>

#include "MantidKernel/DynamicFactory.h"

using namespace Mantid::Kernel;

// Helper class
class AFactory : public DynamicFactory<int>
{
};

class DynamicFactoryTest : public CxxTest::TestSuite
{
  typedef boost::shared_ptr<int> int_ptr;
public:
  void testCreate()
  {
    TS_ASSERT_THROWS( factory.create("testEntry"), std::runtime_error )
    factory.subscribe<int>("testEntry");
    TS_ASSERT_THROWS_NOTHING( int_ptr i = factory.create("testEntry") );
  }

  void testCreateUnwrapped()
  {
    TS_ASSERT_THROWS( factory.createUnwrapped("testUnrappedEntry"), std::runtime_error )
    factory.subscribe<int>("testUnwrappedEntry");
    int *i;
    TS_ASSERT_THROWS_NOTHING( i = factory.createUnwrapped("testEntry") );
    delete i;
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
	
  void testGetKeys()
  {
    std::vector<std::string> keys = factory.getKeys();
    TS_ASSERT(!keys.empty());
  }

private:
  AFactory factory;
	
};

#endif /*DYNAMICFACTORYTEST_H_*/
