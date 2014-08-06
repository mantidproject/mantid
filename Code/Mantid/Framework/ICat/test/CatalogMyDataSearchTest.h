#ifndef MYDATASEARCH_H_
#define MYDATASEARCH_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/CatalogMyDataSearch.h"
#include "MantidICat/CatalogLogin.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "ICatTestHelper.h"


using namespace Mantid;
using namespace Mantid::ICat;

class CatalogMyDataSearchTest: public CxxTest::TestSuite
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
		CatalogMyDataSearch mydata;
		TS_ASSERT_THROWS_NOTHING( mydata.initialize());
		TS_ASSERT( mydata.isInitialized() );
	}
	void testMyDataSearch()
	{
		CatalogMyDataSearch mydata;
		CatalogLogin loginobj;

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
