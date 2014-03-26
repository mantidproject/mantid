#ifndef  LISTINVESTIGATIONTYPES_H_
# define LISTINVESTIGATIONTYPES_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/CatalogListInvestigationTypes.h"
#include "MantidICat/CatalogLogin.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"// why this is required to register table workspace.
#include "ICatTestHelper.h"


using namespace Mantid;
using namespace Mantid::ICat;

class CatalogListInvestigationTypesTest: public CxxTest::TestSuite
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
		TS_ASSERT_THROWS_NOTHING( invstTypesList.initialize());
		TS_ASSERT( invstTypesList.isInitialized() );
	}

	void testListInvestigationTypes()
	{
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
	CatalogListInvestigationTypes invstTypesList;
	CatalogLogin loginobj;
};

 
#endif
