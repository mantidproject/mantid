#ifndef ANALYSISDATASERVICETEST_H_
#define ANALYSISDATASERVICETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

class AnalysisDataServiceTest : public CxxTest::TestSuite
{
public:
	void testInstance()
	{
		AnalysisDataService *theService = AnalysisDataService::Instance();
		AnalysisDataService *theService2 = AnalysisDataService::Instance();
		TS_ASSERT_EQUALS( theService, theService2);
	}

	void testAdd()
	{
    AnalysisDataService *theService = AnalysisDataService::Instance();
    Workspace *space = 0;
    TS_ASSERT_THROWS_NOTHING( theService->add("MySpace",space));
    Workspace *space2 = 0;
    TS_ASSERT_THROWS( theService->add("MySpace",space2),std::runtime_error);
    //clean up the ADS for other tests
    theService->remove("MySpace");
	}

	void testAddOrReplace()
	{
    AnalysisDataService *theService = AnalysisDataService::Instance();
    Workspace *space = 0;
    TS_ASSERT_THROWS_NOTHING(theService->add("MySpaceAddOrReplace",space));
	 TS_ASSERT_THROWS(theService->add("MySpaceAddOrReplace",space),std::runtime_error);
	 TS_ASSERT_THROWS_NOTHING(theService->addOrReplace("MySpaceAddOrReplace",space));
    //clean up the ADS for other tests
    theService->remove("MySpaceAddOrReplace");
	}

	void testRemove()
	{
    AnalysisDataService *theService = AnalysisDataService::Instance();
    Workspace *space = 0;
    theService->add("MySpace",space);
    TS_ASSERT_THROWS_NOTHING(theService->remove("MySpace"));
    Workspace *work = 0;
    TS_ASSERT_THROWS(theService->retrieve("MySpace", work),std::runtime_error);    
    TS_ASSERT_THROWS(theService->remove("ttttt"),std::runtime_error);
	}

	void testRetrieve()
	{
    AnalysisDataService *theService = AnalysisDataService::Instance();
    Workspace *work = 0;
    theService->add("MySpace", work);
    Workspace *workBack = 0;
    TS_ASSERT_THROWS_NOTHING(theService->retrieve("MySpace", workBack));
    TS_ASSERT_EQUALS(work, workBack);
    //clean up the ADS for other tests
    theService->remove("MySpace");
	}

};

#endif /*ANALYSISDATASERVICETEST_H_*/
