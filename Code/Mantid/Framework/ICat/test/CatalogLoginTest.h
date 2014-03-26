#ifndef LOGINTEST_H_
#define LOGINTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/CatalogLogin.h"
#include "ICatTestHelper.h"

using namespace Mantid::ICat;
class CatalogLoginTest: public CxxTest::TestSuite
{
public:
  /// Skip all unit tests if ICat server is down
  bool skipTests()
  {
    return ICatTestHelper::skipTests();
  }

	void testInit()
	{    
		Mantid::Kernel::ConfigService::Instance().setString("default.facility", "ISIS");

		CatalogLogin loginobj;
		TS_ASSERT_THROWS_NOTHING( loginobj.initialize());
		TS_ASSERT( loginobj.isInitialized() );
	}
	void testLogin()
	{
		CatalogLogin loginobj;

	   if ( !loginobj.isInitialized() ) loginobj.initialize();

		// Should fail because mandatory parameter has not been set
		TS_ASSERT_THROWS(loginobj.execute(),std::runtime_error);

		// Now set it...
		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
		//loginobj.setPropertyValue("DBServer", "");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT(loginobj.isExecuted() );
		
	}
	void testLoginFail()
	{
		
		CatalogLogin loginobj;

	   if ( !loginobj.isInitialized() ) loginobj.initialize();

		// Should fail because mandatory parameter has not been set
		TS_ASSERT_THROWS(loginobj.execute(),std::runtime_error);

		//invalid username
		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser1");
		//loginobj.setPropertyValue("DBServer", "");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		//should fail
		TS_ASSERT( !loginobj.isExecuted() );
	}

		
};
#endif
