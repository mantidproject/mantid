#ifndef ANALYSISDATASERVICETEST_H_
#define ANALYSISDATASERVICETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::Kernel;

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
    StatusCode status = theService->add("MySpace",space);
    TS_ASSERT( ! status.isFailure() );
    Workspace *space2 = 0;
    status = theService->add("MySpace",space2);
    TS_ASSERT( status.isFailure() );
	}

	void testAddOrReplace()
	{
    AnalysisDataService *theService = AnalysisDataService::Instance();
    Workspace *space = 0;
    StatusCode status = theService->add("MySpaceAddOrReplace",space);
    TS_ASSERT( ! status.isFailure() );
	status = theService->add("MySpaceAddOrReplace",space);
    TS_ASSERT( status.isFailure() );
	status = theService->addOrReplace("MySpaceAddOrReplace",space);
    TS_ASSERT( ! status.isFailure() );
	}

	void testRemove()
	{
    AnalysisDataService *theService = AnalysisDataService::Instance();
    Workspace *space = 0;
    theService->add("MySpace",space);
    StatusCode status = theService->remove("MySpace");
    TS_ASSERT( ! status.isFailure() );
    Workspace *work = 0;
    status = theService->retrieve("MySpace", work);
    TS_ASSERT( status.isFailure() );
    status = theService->remove("ttttt");
    TS_ASSERT( status.isFailure() );
	}

	void testRetrieve()
	{
    AnalysisDataService *theService = AnalysisDataService::Instance();
    Workspace *work = 0;
    theService->add("MySpace", work);
    Workspace *workBack = 0;
    StatusCode status = theService->retrieve("MySpace", workBack);
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT_EQUALS(work, workBack);

	}

};

#endif /*ANALYSISDATASERVICETEST_H_*/
