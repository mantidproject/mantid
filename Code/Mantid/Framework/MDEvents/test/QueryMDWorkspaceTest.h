#ifndef MANTID_MDEVENTS_QUERYMDWORKSPACETEST_H_
#define MANTID_MDEVENTS_QUERYMDWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include <boost/shared_ptr.hpp>
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidMDEvents/QueryMDWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class QueryMDWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QueryMDWorkspaceTest *createSuite() { return new QueryMDWorkspaceTest(); }
  static void destroySuite( QueryMDWorkspaceTest *suite ) { delete suite; }


  void testCheckInputs()
  {
    MDEventWorkspace3Lean::sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, -10.0, 20.0, 3);
    QueryMDWorkspace query;
    query.initialize();
    query.setProperty("InputWorkspace", in_ws);
    query.setProperty("OutputWorkspace", "QueryWS");
    TSM_ASSERT_EQUALS("Invalid property setup", true, query.validateProperties());
    TSM_ASSERT("Should limit rows by default", query.getProperty("LimitRows"));
    const int expectedRowLimit = 100000;
    const int actualRowLimit = query.getProperty("MaximumRows");
    TSM_ASSERT_EQUALS("Wrong default number of rows", expectedRowLimit, actualRowLimit);
  }

  void testExecution()
  {
    MDEventWorkspace3Lean::sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, -10.0, 20.0, 3);
    QueryMDWorkspace query;
    query.initialize();
    query.setProperty("InputWorkspace", in_ws);
    query.setPropertyValue("OutputWorkspace", "QueryWS");
    query.execute();
    TSM_ASSERT("Did not execute", query.isExecuted());
  }

  void testExecution_BoxData()
  {
    MDEventWorkspace3Lean::sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, -10.0, 20.0, 3);
    QueryMDWorkspace query;
    query.initialize();
    query.setProperty("InputWorkspace", in_ws);
    query.setPropertyValue("OutputWorkspace", "QueryWS");
    query.setPropertyValue("BoxDataTable", "QueryWS_box");
    query.execute();
    TSM_ASSERT("Did not execute", query.isExecuted());
  }

  void testTableGenerated()
  {
    MDEventWorkspace3Lean::sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, -10.0, 20.0, 3);
    QueryMDWorkspace query;
    query.initialize();
    query.setProperty("InputWorkspace", in_ws);
    query.setPropertyValue("OutputWorkspace", "QueryWS");
    query.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("QueryWS"));

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("QueryWS");

    TSM_ASSERT("Workspace output is not an ITableWorkspace", table !=NULL);
    TSM_ASSERT_EQUALS("Four columns expected", 4, table->columnCount());
    TSM_ASSERT_EQUALS("Wrong number of rows", 1000, table->rowCount());
  }

  void testLimitRows()
  {
    MDEventWorkspace3Lean::sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, -10.0, 20.0, 3);
    QueryMDWorkspace query;
    query.initialize();
    query.setProperty("InputWorkspace", in_ws);
    query.setPropertyValue("OutputWorkspace", "QueryWS");
    query.setProperty("LimitRows", true);
    query.setProperty("MaximumRows", 3);
    query.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("QueryWS"));

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("QueryWS");

    TSM_ASSERT("Workspace output is not an ITableWorkspace", table !=NULL);
    TSM_ASSERT_EQUALS("Four columns expected", 4, table->columnCount());
    TSM_ASSERT_EQUALS("Wrong number of rows", 3, table->rowCount());
  }


};


#endif /* MANTID_MDEVENTS_QUERYMDWORKSPACETEST_H_ */
