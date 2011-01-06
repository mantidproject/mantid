#ifndef LOGINTEST_H_
#define LOGINTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/Login.h"
#include "MantidICat/Session.h"

using namespace Mantid::ICat;
class LoginTest: public CxxTest::TestSuite
{
public:
	void testInit()
	{
		Login loginobj;
		TS_ASSERT_THROWS_NOTHING( loginobj.initialize());
		TS_ASSERT( loginobj.isInitialized() );
	}
	void testLogin()
	{
		/*std::string s;
		std::getline(std::cin,s);*/
		Session::Instance();
		Login loginobj;

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
		
		Login loginobj;
		Session::Instance();

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
		//empty sessionid
		TS_ASSERT(!Session::Instance().getSessionId().empty());
		
	}

		
};
#endif
