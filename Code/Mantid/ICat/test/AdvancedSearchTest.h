#ifndef ADVANCEDSEARCH_H_
#define ADVANCEDSEARCH_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/AdvancedSearch.h"
#include "MantidICat/Session.h"
#include "MantidICat/Login.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

using namespace Mantid;
using namespace Mantid::ICat;
class CAdvancedSearchTest: public CxxTest::TestSuite
{
public:
	
	void testInit()
	{
		CAdvancedSearch searchobj;
		Login loginobj;
		TS_ASSERT_THROWS_NOTHING( searchobj.initialize());
		TS_ASSERT( searchobj.isInitialized() );
	}
	void testSearchByRunNumberandInstrument()
	{
		/*std::string s;
		std::getline(std::cin,s);*/

		CAdvancedSearch searchobj;
		Login loginobj;
		Session::Instance();
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
		
		/*std::string s;
		std::getline(std::cin,s);*/
		
		CAdvancedSearch searchobj;
		Login loginobj;
		Session::Instance();
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
		CAdvancedSearch searchobj;
		Login loginobj;
		Session::Instance();
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
		//loginobj.setPropertyValue("DBServer", "");
		
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
		Login loginobj;
		Session::Instance();
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		// Now set it...
		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
		//loginobj.setPropertyValue("DBServer", "");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );
		CAdvancedSearch searchobj;
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

	void xtestSearchByInvalidDates()
	{

		CAdvancedSearch searchobj;
		Login loginobj;
		Session::Instance();
		if ( !loginobj.isInitialized() ) loginobj.initialize();

		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if ( !searchobj.isInitialized() ) searchobj.initialize();
				
		searchobj.setPropertyValue("StartDate","sssss");
		searchobj.setPropertyValue("EndDate","sofia");
		searchobj.setPropertyValue("OutputWorkspace","Investigations");
				
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());

		//should fail
		TS_ASSERT(! searchobj.isExecuted() );

	}
		
};
#endif