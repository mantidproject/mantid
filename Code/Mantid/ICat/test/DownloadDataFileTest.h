#ifndef DOWNLOADDATAFILE_H
#define DOWNLOADDATAFILE_H
#include <cxxtest/TestSuite.h>
#include "MantidICat/DownloadDataFile.h"
#include "MantidICat/Session.h"
#include "MantidICat/Login.h"
//#include "MantidICat/FileList.h"
#include "MantidICat/GetInvestigation.h"
#include "MantidICat/SearchByRunNumber.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"

using namespace Mantid;
using namespace Mantid::ICat;
class CDownloadDataFileTest: public CxxTest::TestSuite
{
public:
	void testInit()
	{
		TS_ASSERT_THROWS_NOTHING( downloadobj.initialize());
		TS_ASSERT( downloadobj.isInitialized() );
	}
	void testSearchByAdavanced()
	{
		std::string s;
		std::getline(std::cin,s);
		Session::Instance();

		if ( !loginobj.isInitialized() ) loginobj.initialize();

		// Now set it...
		loginobj.setPropertyValue("Username", "mantid_test");
		loginobj.setPropertyValue("Password", "mantidtestuser");
		loginobj.setPropertyValue("DBServer", "");
		
		TS_ASSERT_THROWS_NOTHING(loginobj.execute());
		TS_ASSERT( loginobj.isExecuted() );

		//if ( !filelistobj.isInitialized() ) filelistobj.initialize();
		//// Now set it...
		//filelistobj.setPropertyValue("StartRun", "100.0");
		//filelistobj.setPropertyValue("EndRun", "102.0");
		//filelistobj.setPropertyValue("OutputWorkspace","SearchBy_RunNumber");
		//TS_ASSERT_THROWS_NOTHING(filelistobj.execute());
		//TS_ASSERT( filelistobj.isExecuted() );
		
		if ( !searchobj.isInitialized() ) searchobj.initialize();
		searchobj.setPropertyValue("StartRun", "100.0");
		searchobj.setPropertyValue("EndRun", "102.0");
		searchobj.setPropertyValue("OutputWorkspace","SearchBy_RunNumber");
				
		TS_ASSERT_THROWS_NOTHING(searchobj.execute());
		TS_ASSERT( searchobj.isExecuted() );

		if ( !invstObj.isInitialized() ) invstObj.initialize();
		//invstObj.setPropertyValue("Title", "1-to-1 Ni Powder Top Shield on");
		invstObj.setPropertyValue("Title", "PZT1 10 130 C");
		invstObj.setPropertyValue("InputWorkspace", "SearchBy_RunNumber");
		invstObj.setPropertyValue("OutputWorkspace","filelist");
		//		
		TS_ASSERT_THROWS_NOTHING(invstObj.execute());
		TS_ASSERT( invstObj.isExecuted() );
		
		if ( !downloadobj.isInitialized() ) downloadobj.initialize();

		downloadobj.setPropertyValue("Filename","GEM19105.RAW");
		downloadobj.setPropertyValue("InputWorkspace","filelist");
						
		TS_ASSERT_THROWS_NOTHING(downloadobj.execute());
		TS_ASSERT( downloadobj.isExecuted() );

	}
private:
	   CSearchByRunNumber searchobj;
	 CGetInvestigation invstObj;
		CDownloadDataFile downloadobj;
		//CFileList filelistobj;
		Login loginobj;
};
#endif