#ifndef SEARCHBYADVANCED_H_
#define SEARCHBYADVANCED_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/CatalogSearch.h"
#include "MantidICat/CatalogLogin.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidAPI/FrameworkManager.h"
#include "ICatTestHelper.h"

using namespace Mantid;
using namespace Mantid::ICat;
class CatalogSearchTest: public CxxTest::TestSuite
{
public:
  /// Skip all unit tests if ICat server is down
  bool skipTests()
  {
    return ICatTestHelper::skipTests();
  }

  CatalogSearchTest()
  {
    Mantid::API::FrameworkManager::Instance();
  }
	
	void testInit()
	{
	  Mantid::Kernel::ConfigService::Instance().setString("default.facility", "ISIS");

		CatalogSearch searchobj;
		CatalogLogin loginobj;
		TS_ASSERT_THROWS_NOTHING( searchobj.initialize());
		TS_ASSERT( searchobj.isInitialized() );
	}
	void testSearchByRunNumberandInstrumentExecutes()
	{
		// Uses an unused keyword to produce an empty workspace and be fast
		
		CatalogSearch searchobj;
		CatalogLogin loginobj;

		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantidtest@fitsp10.isis.cclrc.ac.uk");
		loginobj.setPropertyValue("Password", "MantidTestUser4");
		//loginobj.setPropertyValue("DBServer", "");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if ( !searchobj.isInitialized() ) searchobj.initialize();
		
		searchobj.setPropertyValue("RunRange", "1000000-1000001");
		searchobj.setPropertyValue("Instrument","LOQ");
		searchobj.setPropertyValue("OutputWorkspace","Investigations");
					
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());
		TS_ASSERT( searchobj.isExecuted() );

	}
	void testSearchByKeywordsExecutes()
	{
		
		CatalogSearch searchobj;
		CatalogLogin loginobj;

		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantidtest@fitsp10.isis.cclrc.ac.uk");
		loginobj.setPropertyValue("Password", "MantidTestUser4");
		//loginobj.setPropertyValue("DBServer", "");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if ( !searchobj.isInitialized() ) searchobj.initialize();
				
		searchobj.setPropertyValue("Keywords","000117");
		searchobj.setPropertyValue("Instrument","HRPD");
		searchobj.setPropertyValue("OutputWorkspace","Investigations");
				
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());
		TS_ASSERT( searchobj.isExecuted() );

	}
	void testSearchByStartDateEndDateExecutes()
	{
		// Uses a search date outside of general operation to produce an empty workspace and be fast

		CatalogSearch searchobj;
		CatalogLogin loginobj;

		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantidtest@fitsp10.isis.cclrc.ac.uk");
		loginobj.setPropertyValue("Password", "MantidTestUser4");
			
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if ( !searchobj.isInitialized() ) searchobj.initialize();
		
		searchobj.setPropertyValue("StartDate","10/08/1980");
		searchobj.setPropertyValue("EndDate","22/08/1980");
		searchobj.setPropertyValue("OutputWorkspace","Investigations");
				
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());
		TS_ASSERT( searchobj.isExecuted() );

	}
	void testSearchByRunNumberInvalidInput()
	{		
		CatalogLogin loginobj;

		if ( !loginobj.isInitialized() ) loginobj.initialize();

		// Now set it...
		loginobj.setPropertyValue("Username", "mantidtest@fitsp10.isis.cclrc.ac.uk");
		loginobj.setPropertyValue("Password", "MantidTestUser4");
		//loginobj.setPropertyValue("DBServer", "");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );
		CatalogSearch searchobj;
		if ( !searchobj.isInitialized() ) searchobj.initialize();
		
		// start run number > end run number
		searchobj.setPropertyValue("RunRange", "150-102");
		searchobj.setPropertyValue("Instrument","LOQ");
				
    	searchobj.setPropertyValue("OutputWorkspace","Investigations");
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());
		//should fail
		TS_ASSERT( !searchobj.isExecuted() );

	}

	void testSearchByInvalidDates1()
	{
		CatalogSearch searchobj;
		CatalogLogin loginobj;

		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantidtest@fitsp10.isis.cclrc.ac.uk");
		loginobj.setPropertyValue("Password", "MantidTestUser4");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if ( !searchobj.isInitialized() ) searchobj.initialize();
		std::string errorMsg="Invalid value for property StartDate (string) ""sssss"": Invalid Date:date format must be DD/MM/YYYY";

		TS_ASSERT_THROWS(searchobj.setPropertyValue("StartDate","sssss"),std::invalid_argument(errorMsg));
		
		errorMsg="Invalid value for property EndDate (string) ""aaaaa"": Invalid Date:date format must be DD/MM/YYYY";
		TS_ASSERT_THROWS(searchobj.setPropertyValue("EndDate","aaaaa"),std::invalid_argument(errorMsg));

	}

	void testSearchByInvalidDates2()
	{

		CatalogSearch searchobj;
		CatalogLogin loginobj;

		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantidtest@fitsp10.isis.cclrc.ac.uk");
		loginobj.setPropertyValue("Password", "MantidTestUser4");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if ( !searchobj.isInitialized() ) searchobj.initialize();
				
		std::string errorMsg="Invalid value for property StartDate (string) ""39/22/2009"": Invalid Date:Day part of the Date parameter must be between 1 and 31";
		TS_ASSERT_THROWS(searchobj.setPropertyValue("StartDate","39/22/2009"),std::invalid_argument(errorMsg));

		errorMsg="Invalid value for property EndDate (string) ""1/22/2009"": Invalid Date:Month part of the Date parameter must be between 1 and 12";
		TS_ASSERT_THROWS(searchobj.setPropertyValue("EndDate","1/22/2009"),std::invalid_argument(errorMsg));

	}
		
};
#endif
