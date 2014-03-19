#ifndef GETINVESTIGATION_H_
#define GETINVESTIGATION_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/CatalogGetDataFiles.h"
#include "MantidICat/CatalogLogin.h"
#include "MantidICat/CatalogSearch.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "ICatTestHelper.h"

using namespace Mantid;
using namespace Mantid::ICat;

class CatalogGetDataFilesTest: public CxxTest::TestSuite
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
		TS_ASSERT_THROWS_NOTHING( invstObj.initialize());
		TS_ASSERT( invstObj.isInitialized() );
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

		if (!invstObj.isInitialized() ) invstObj.initialize();
		invstObj.setPropertyValue("InvestigationId","12576918");
		//invstObj.setPropertyValue("InputWorkspace", "investigations");//records of investigations
		invstObj.setPropertyValue("OutputWorkspace","investigation");//selected invesigation data files
		//		
		TS_ASSERT_THROWS_NOTHING(invstObj.execute());
		TS_ASSERT( invstObj.isExecuted() );
	}
private:
	CatalogLogin loginobj;
	CatalogSearch searchobj;
	CatalogGetDataFiles invstObj;
};
#endif
