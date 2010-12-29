#ifndef  LISTINVESTIGATIONTYPES_H_
# define LISTINVESTIGATIONTYPES_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/ListInvestigationTypes.h"
#include "MantidICat/Session.h"
#include "MantidICat/Login.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"// why this is required to register table workspace.


using namespace Mantid;
using namespace Mantid::ICat;

class CListInvestigationTypesTest: public CxxTest::TestSuite
{
public:
	void testInit()
	{
		TS_ASSERT_THROWS_NOTHING( invstTypesList.initialize());
		TS_ASSERT( invstTypesList.isInitialized() );
	}

	void testListInvestigationTypes()
	{
	
		Session::Instance();
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
			
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if (!invstTypesList.isInitialized() ) invstTypesList.initialize();
		//invstTypesList.setPropertyValue("OutputWorkspace","investigationtypes_list");
						
		TS_ASSERT_THROWS_NOTHING(invstTypesList.execute());
		TS_ASSERT( invstTypesList.isExecuted() );


	}
private:
	CListInvestigationTypes invstTypesList;
	Login loginobj;
};

 
#endif
