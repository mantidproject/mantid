#ifndef MYDATASEARCH_H_
#define MYDATASEARCH_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/MyDataSearch.h"
#include "MantidICat/Session.h"
#include "MantidICat/Login.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"


using namespace Mantid;
using namespace Mantid::ICat;

class MyDataSearchTest: public CxxTest::TestSuite
{
public:
	
	void testInit()
	{
		CMyDataSearch mydata;
		TS_ASSERT_THROWS_NOTHING( mydata.initialize());
		TS_ASSERT( mydata.isInitialized() );
	}
	void testMyDataSearch()
	{
		/*std::string s;
		std::getline(std::cin,s);*/

		CMyDataSearch mydata;
		Login loginobj;
		Session::Instance();
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
	
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if ( !mydata.isInitialized() ) mydata.initialize();
			
		mydata.setPropertyValue("OutputWorkspace","MyInvestigations");
				
		TS_ASSERT_THROWS_NOTHING(mydata.execute());
		TS_ASSERT( mydata.isExecuted() );

	}
			
};
#endif
