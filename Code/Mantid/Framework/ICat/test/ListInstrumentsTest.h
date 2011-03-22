#ifndef LISTINSTRUMENTS_H_
# define LISTINSTRUMENTS_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/ListInstruments.h"
#include "MantidICat/Session.h"
#include "MantidICat/Login.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"// why this is required to register table workspace.


using namespace Mantid;
using namespace Mantid::ICat;

class ListInstrumentsTest: public CxxTest::TestSuite
{
public:
	void testInit()
	{
		TS_ASSERT_THROWS_NOTHING( instrList.initialize());
		TS_ASSERT( instrList.isInitialized() );
	}

	void testListInstruments()
	{
		/*std::string s;
		std::getline(std::cin,s);*/
	
		Session::Instance();
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
			
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if (!instrList.isInitialized() ) instrList.initialize();
		//instrList.setPropertyValue("OutputWorkspace","instrument_list");
						
		TS_ASSERT_THROWS_NOTHING(instrList.execute());
		TS_ASSERT( instrList.isExecuted() );


	}
private:
	CListInstruments instrList;
	Login loginobj;
};

 
#endif
