#ifndef ANALYSISDATASERVICETEST_H_
#define ANALYSISDATASERVICETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

class AnalysisDataServiceTest : public CxxTest::TestSuite
{
public:

  void testAdd()
  {
    Workspace_sptr space;
    TS_ASSERT_THROWS_NOTHING( AnalysisDataService::Instance().add("MySpace",space));
    Workspace_sptr space2;
    TS_ASSERT_THROWS( AnalysisDataService::Instance().add("MySpace",space2),std::runtime_error);
    //clean up the ADS for other tests
    AnalysisDataService::Instance().remove("MySpace");
  }

  void testAddOrReplace()
  {
    Workspace_sptr space;
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add("MySpaceAddOrReplace",space));
    TS_ASSERT_THROWS(AnalysisDataService::Instance().add("MySpaceAddOrReplace",space),std::runtime_error);
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace("MySpaceAddOrReplace",space));
    //clean up the ADS for other tests
    AnalysisDataService::Instance().remove("MySpaceAddOrReplace");
  }

  void testRemove()
  {
    Workspace_sptr space;
    AnalysisDataService::Instance().add("MySpace",space);
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove("MySpace"));
    TS_ASSERT_THROWS(AnalysisDataService::Instance().retrieve("MySpace"),std::runtime_error);    
    // Remove should not throw but give a warning in the log file, changed by LCC 05/2008
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove("ttttt"));
  }

  void testRetrieve()
  {
    Workspace_sptr work;
    AnalysisDataService::Instance().add("MySpace", work);
    Workspace_sptr workBack;
    TS_ASSERT_THROWS_NOTHING(workBack = AnalysisDataService::Instance().retrieve("MySpace"));
    TS_ASSERT_EQUALS(work, workBack);
    //clean up the ADS for other tests
    AnalysisDataService::Instance().remove("MySpace");
  }

};

#endif /*ANALYSISDATASERVICETEST_H_*/
