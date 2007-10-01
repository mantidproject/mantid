#ifndef ANALYSISDATASERVICETEST_H_
#define ANALYSISDATASERVICETEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/AnalysisDataService.h"

using namespace Mantid;

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
    StatusCode status = theService->add("MySpace",new Workspace());
    TS_ASSERT( ! status.isFailure() );
    status = theService->add("MySpace",new Workspace());
    TS_ASSERT( status.isFailure() );
	}

	void testRemove()
	{
    AnalysisDataService *theService = AnalysisDataService::Instance();
    theService->add("MySpace",new Workspace());
    StatusCode status = theService->remove("MySpace");
    TS_ASSERT( ! status.isFailure() );
    Workspace *work;
    status = theService->retrieve("MySpace", work);
    TS_ASSERT( status.isFailure() );
    status = theService->remove("ttttt");
    TS_ASSERT( status.isFailure() );
	}

	void testRetrieve()
	{
    AnalysisDataService *theService = AnalysisDataService::Instance();
    Workspace *work;
    theService->add("MySpace", work);
    Workspace *workBack;
    StatusCode status = theService->retrieve("MySpace", workBack);
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT_EQUALS(work, workBack);

	}

};

#endif /*ANALYSISDATASERVICETEST_H_*/
