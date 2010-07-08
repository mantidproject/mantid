#ifndef FILELISTTEST_H
#define FILELISTTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidICat/FileList.h"
#include "MantidICat/Session.h"
#include "MantidICat/Login.h"
#include "MantidICat/GetInvestigation.h"
#include "MantidICat/SearchByRunNumber.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

using namespace Mantid;
using namespace Mantid::ICat;
class CFileListTest: public CxxTest::TestSuite
{
public:
	void testInit()
	{
		TS_ASSERT_THROWS_NOTHING( invstObj.initialize());
		TS_ASSERT( invstObj.isInitialized() );
	}
	void xtestSearchByAdavanced()
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
		searchobj.setPropertyValue("StartRun", "100.0");
		searchobj.setPropertyValue("EndRun", "102.0");
		searchobj.setPropertyValue("OutputWorkspace","SearchBy_RunNumber");
				
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());
		TS_ASSERT( searchobj.isExecuted() );

		if ( !invstObj.isInitialized() ) invstObj.initialize();
		invstObj.setPropertyValue("Title", "1-to-1 Ni Powder Top Shield on");
		invstObj.setPropertyValue("InputWorkspace", "SearchBy_RunNumber");
		invstObj.setPropertyValue("OutputWorkspace","filelist");
		//		
		TS_ASSERT_THROWS_NOTHING(invstObj.execute());
		TS_ASSERT( invstObj.isExecuted() );


	}
private:
	CFileList filelistobj;
	CSearchByRunNumber searchobj;
	CGetDataFiles invstObj;
	Login loginobj;
};
#endif