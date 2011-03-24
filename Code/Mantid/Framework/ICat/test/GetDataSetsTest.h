#ifndef GETIDATASETS_H_
#define GETIDATASETS_H_

#include <cxxtest/TestSuite.h>
#include "MantidICat/GetDataSets.h"
#include "MantidICat/Session.h"
#include "MantidICat/Login.h"
#include "MantidICat/Search.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

using namespace Mantid;
using namespace Mantid::ICat;

class GetDataSetsTest: public CxxTest::TestSuite
{
public:
	void testInit()
	{
		Mantid::Kernel::ConfigService::Instance().setString("default.facility", "ISIS");

		TS_ASSERT_THROWS_NOTHING( datasets.initialize());
		TS_ASSERT( datasets.isInitialized() );
	}

	void testgetDataFiles()
	{	
		/*std::string str;
		std::getline(std::cin,str);*/
	
		Session::Instance();
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
	Login loginobj;
	CSearch searchobj;
	//CGetDataFiles datafiles;
	CGetDataSets datasets;
};
#endif
