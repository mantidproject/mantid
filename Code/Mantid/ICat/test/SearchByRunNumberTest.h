#ifndef SEARCHBYADVANCED_H_
#define SEARCHBYADVANCED_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/SearchByRunNumber.h"
#include "MantidICat/Session.h"
#include "MantidICat/Login.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
//#include <iostream>

using namespace Mantid;
using namespace Mantid::ICat;
class CSearchByAdvancedTest: public CxxTest::TestSuite
{
public:
	
	void testInit()
	{
		TS_ASSERT_THROWS_NOTHING( searchobj.initialize());
		TS_ASSERT( searchobj.isInitialized() );
	}
	void testSearchByRunNumberandInstrument()
	{
		/*std::string s;
		std::getline(std::cin,s);*/
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
		searchobj.setPropertyValue("Instruments","LOQ");
		searchobj.setPropertyValue("OutputWorkspace","SearchBy_RunNumber");
				
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());
		TS_ASSERT( searchobj.isExecuted() );

	}
	void xtestSearchByRunNumberInvalidInput()
	{

		/*std::string s;
		std::getline(std::cin,s);*/

		Session::Instance();

		if ( !loginobj.isInitialized() ) loginobj.initialize();

		// Now set it...
		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
		//loginobj.setPropertyValue("DBServer", "");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		if ( !searchobj.isInitialized() ) searchobj.initialize();
		
		// start run number < end run number
		searchobj.setPropertyValue("StartRun", "150.0");
		searchobj.setPropertyValue("EndRun", "102.0");
		searchobj.setPropertyValue("Instruments","LOQ");
		searchobj.setPropertyValue("OutputWorkspace","SearchBy_RunNumber");
				
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());
		//should fail
		TS_ASSERT( !loginobj.isExecuted() );

	}
private:
		CSearchByRunNumber searchobj;
		Login loginobj;
};
#endif