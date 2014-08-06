#ifndef LISTINSTRUMENTS_H_
# define LISTINSTRUMENTS_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/CatalogListInstruments.h"
#include "MantidICat/CatalogLogin.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"// why this is required to register table workspace.
#include "ICatTestHelper.h"


using namespace Mantid;
using namespace Mantid::ICat;

class CatalogListInstrumentsTest: public CxxTest::TestSuite
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
		TS_ASSERT_THROWS_NOTHING( instrList.initialize());
		TS_ASSERT( instrList.isInitialized() );
	}

	void testListInstruments()
	{
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
	CatalogListInstruments instrList;
	CatalogLogin loginobj;
};

 
#endif
