#ifndef SEARCHBYADVANCED_H_
#define SEARCHBYADVANCED_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/CatalogSearch.h"
#include "MantidICat/Session.h"
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
	void testSearchByRunNumberandInstrument()
	{
		
		CatalogSearch searchobj;
		CatalogLogin loginobj;
		ICat::Session::Instance();
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
		//loginobj.setPropertyValue("DBServer", "");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if ( !searchobj.isInitialized() ) searchobj.initialize();
		
		searchobj.setPropertyValue("StartRun", "100.0");
		searchobj.setPropertyValue("EndRun", "109.0");
		searchobj.setPropertyValue("Instrument","LOQ");
		searchobj.setPropertyValue("OutputWorkspace","Investigations");
					
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());
		TS_ASSERT( searchobj.isExecuted() );

	}
	void testSearchByKeywords()
	{
		
		CatalogSearch searchobj;
		CatalogLogin loginobj;
		ICat::Session::Instance();
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
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
	void testSearchByStartDateEndDate()
	{
		CatalogSearch searchobj;
		CatalogLogin loginobj;
		ICat::Session::Instance();
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
			
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if ( !searchobj.isInitialized() ) searchobj.initialize();
		
		searchobj.setPropertyValue("StartDate","10/08/2008");
		searchobj.setPropertyValue("EndDate","22/08/2008");
		searchobj.setPropertyValue("OutputWorkspace","Investigations");
				
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());
		TS_ASSERT( searchobj.isExecuted() );

	}
	void testSearchByRunNumberInvalidInput()
	{		
		CatalogLogin loginobj;
		ICat::Session::Instance();
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		// Now set it...
		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
		//loginobj.setPropertyValue("DBServer", "");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );
		CatalogSearch searchobj;
		if ( !searchobj.isInitialized() ) searchobj.initialize();
		
		// start run number > end run number
		searchobj.setPropertyValue("StartRun", "150.0");
		searchobj.setPropertyValue("EndRun", "102.0");
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
		ICat::Session::Instance();
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if ( !searchobj.isInitialized() ) searchobj.initialize();
		std::string errorMsg="Invalid value for property StartDate (string) ""sssss"": Invalid Date:date format must be DD/MM/YYYY";

		TS_ASSERT_THROWS(searchobj.setPropertyValue("StartDate","sssss"),std::invalid_argument(errorMsg));
		
		errorMsg="Invalid value for property StartDate (string) ""aaaaa"": Invalid Date:date format must be DD/MM/YYYY";
		TS_ASSERT_THROWS(searchobj.setPropertyValue("EndDate","aaaaa"),std::invalid_argument(errorMsg));

	}

	void xtestSearchByInvalidDates2()
	{

		CatalogSearch searchobj;
		CatalogLogin loginobj;
		ICat::Session::Instance();
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if ( !searchobj.isInitialized() ) searchobj.initialize();
				
		TS_ASSERT_THROWS(searchobj.setPropertyValue("StartDate","39/22/2009"),std::runtime_error);
		TS_ASSERT_THROWS(searchobj.setPropertyValue("EndDate","aaaaa"),std::runtime_error);
		searchobj.setPropertyValue("OutputWorkspace","Investigations");
		
		TS_ASSERT_THROWS(searchobj.execute(),std::runtime_error);

		//should fail
		TS_ASSERT(! searchobj.isExecuted() );

	}
		
};
#endif
