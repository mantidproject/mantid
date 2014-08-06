#ifndef GETIDATASETS_H_
#define GETIDATASETS_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/CatalogGetDataSets.h"
#include "MantidICat/CatalogLogin.h"
#include "MantidICat/CatalogSearch.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "ICatTestHelper.h"

using namespace Mantid;
using namespace Mantid::ICat;

class CatalogGetDataSetsTest: public CxxTest::TestSuite
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

		TS_ASSERT_THROWS_NOTHING( datasets.initialize());
		TS_ASSERT( datasets.isInitialized() );
	}

	void testgetDataFiles()
	{	
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
	
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if ( !searchobj.isInitialized() ) searchobj.initialize();
		searchobj.setPropertyValue("StartRun", "100.0");
		searchobj.setPropertyValue("EndRun", "102.0");
		searchobj.setPropertyValue("Instrument","LOQ");
		searchobj.setPropertyValue("OutputWorkspace","investigations");
				
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());
		TS_ASSERT( searchobj.isExecuted() );

		if(!datasets.isInitialized()) datasets.initialize();
		datasets.setPropertyValue("InvestigationId","12576918");
		datasets.setPropertyValue("OutputWorkspace","investigation");//selected invesigation
		//		
		TS_ASSERT_THROWS_NOTHING(datasets.execute());
		TS_ASSERT( datasets.isExecuted() );
	}
private:
	CatalogLogin loginobj;
	CatalogSearch searchobj;
	//CatalogGetDataFiles datafiles;
	CatalogGetDataSets datasets;
};
#endif
