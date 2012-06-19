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
#include "MantidDataObjects/TableWorkspace.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class QueryMDWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QueryMDWorkspaceTest *createSuite() { return new QueryMDWorkspaceTest(); }
  static void destroySuite( QueryMDWorkspaceTest *suite ) { delete suite; }


  void checkInputs(std::string strNormalisation)
  {
    MDEventWorkspace3Lean::sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, -10.0, 20.0, 3);
    QueryMDWorkspace query;
    query.initialize();    
    query.setRethrows(true);
    query.setProperty("InputWorkspace", in_ws);
    query.setProperty("OutputWorkspace", "QueryWS");
    query.setProperty("Normalisation", "none");
    TSM_ASSERT_EQUALS("Invalid property setup", true, query.validateProperties());
  }

  void testDefaultInputs()
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
    std::string defaultNormalisation = query.getProperty("Normalisation");
    TSM_ASSERT_EQUALS("Wrong default normalisation", "none", defaultNormalisation);
  }

  void testCheckInputsWithNoNormalisation()
  {
    checkInputs("none");
  }

  void testCheckInputsWithVolumeNormalisation()
  {
    checkInputs("volume");
  }

  void testCheckInputsWithNumberOfEventsNormalisation()
  {
    checkInputs("number of events");
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

  void testDifferentNormalisation()
  {
    MDEventWorkspace3Lean::sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, -10.0, 20.0, 3);
    boost::shared_ptr<IMDIterator> it(in_ws->createIterator());

    QueryMDWorkspace A;
    A.initialize();
    A.setProperty("InputWorkspace", in_ws);
    A.setPropertyValue("OutputWorkspace", "QueryWS_A");
    A.setPropertyValue("Normalisation", "none"); //Not normalising
    A.execute();

    QueryMDWorkspace B;
    B.initialize();
    B.setProperty("InputWorkspace", in_ws);
    B.setPropertyValue("OutputWorkspace", "QueryWS_B");
    B.setPropertyValue("Normalisation", "number of events"); //Normalising by n events
    B.execute();

    AnalysisDataServiceImpl& ADS = AnalysisDataService::Instance();

    TableWorkspace_sptr queryA = boost::dynamic_pointer_cast<TableWorkspace>(ADS.retrieve("QueryWS_A"));
    TableWorkspace_sptr queryB = boost::dynamic_pointer_cast<TableWorkspace>(ADS.retrieve("QueryWS_B"));
    
    TS_ASSERT_EQUALS(queryA->rowCount(), queryB->rowCount());

    for(size_t i = 0; i < queryA->rowCount(); ++i)
    {
      it->next();
      TSM_ASSERT("The iterator should be valid over the range of table rows it was used to create.", it->valid());

      double signalNotNormalised = queryA->cell<double>(i, 0);
      double signalNormalisedByNumEvents = queryB->cell<double>(i, 0);
      double errorNotNormalised = queryA->cell<double>(i, 1);
      double errorNormalisedByNumEvents = queryB->cell<double>(i, 1);
      const size_t nEvents = it->getNumEvents();

      //Compare each signal and error result.
      TS_ASSERT_DELTA(signalNotNormalised, signalNormalisedByNumEvents*nEvents, 0.0001);
      TS_ASSERT_DELTA(errorNotNormalised, errorNormalisedByNumEvents*nEvents, 0.0001);
    }
    
    ADS.remove("QueryWS_A");
    ADS.remove("QueryWS_B");
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
    size_t expectedCount = 3 + in_ws->getNumDims(); //3 fixed columns are Signal, Error, nEvents 
    TSM_ASSERT_EQUALS("Four columns expected", expectedCount, table->columnCount());
    TSM_ASSERT_EQUALS("Wrong number of rows", 1000, table->rowCount());
  }

  void testNumberOfColumnsDependsOnDimensionality()
  {
    MDEventWorkspace2Lean::sptr in_ws = MDEventsTestHelper::makeMDEW<2>(10, -10.0, 20.0, 3);
    QueryMDWorkspace query;
    query.initialize();
    query.setProperty("InputWorkspace", in_ws);
    query.setPropertyValue("OutputWorkspace", "QueryWS");
    query.execute();

    TS_ASSERT(AnalysisDataService::Instance().doesExist("QueryWS"));

    ITableWorkspace_sptr table = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("QueryWS");

    TSM_ASSERT("Workspace output is not an ITableWorkspace", table !=NULL);
    size_t expectedCount = 3 + in_ws->getNumDims(); //3 fixed columns are Signal, Error, nEvents 
    TSM_ASSERT_EQUALS("Five columns expected", expectedCount, table->columnCount());
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
    size_t expectedCount = 3 + in_ws->getNumDims(); //3 fixed columns are Signal, Error, nEvents 
    TSM_ASSERT_EQUALS("Six columns expected", expectedCount, table->columnCount());
    TSM_ASSERT_EQUALS("Wrong number of rows", 3, table->rowCount());
  }


};


#endif /* MANTID_MDEVENTS_QUERYMDWORKSPACETEST_H_ */
