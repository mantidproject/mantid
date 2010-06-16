#ifndef LOGINTEST_H
#define LOGINTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidICat/Login.h"
#include "MantidICat/Session.h"

using namespace Mantid::ICat;
class CLoginTest: public CxxTest::TestSuite
{
public:
	void testInit()
	{
		TS_ASSERT_THROWS_NOTHING( loginobj.initialize());
		TS_ASSERT( loginobj.isInitialized() );
	}
	void testLogin()
	{
		std::string s;
		std::getline(std::cin,s);
		Session::Instance();

	if ( !loginobj.isInitialized() ) loginobj.initialize();

		// Should fail because mandatory parameter has not been set
		TS_ASSERT_THROWS(loginobj.execute(),std::runtime_error);

		// Now set it...
		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
		loginobj.setPropertyValue("DBServer", "");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		std::string sessionid;//=loginobj.getPropertyValue("SessionId");
		//std::cout<<"test method session id "<<sessionid<<std::endl;
		sessionid=Session::Instance().getSessionId();
		std::cout<<"test method session id "<<sessionid<<std::endl;
	}
private:

		Login loginobj;
};
#endif