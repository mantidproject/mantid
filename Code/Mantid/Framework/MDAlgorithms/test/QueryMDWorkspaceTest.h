#ifndef MANTID_MDEVENTS_QUERYMDWORKSPACETEST_H_
#define MANTID_MDEVENTS_QUERYMDWORKSPACETEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidMDAlgorithms/QueryMDWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidDataObjects/TableWorkspace.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

class QueryMDWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QueryMDWorkspaceTest *createSuite() { return new QueryMDWorkspaceTest(); }
  static void destroySuite( QueryMDWorkspaceTest *suite ) { delete suite; }

  QueryMDWorkspaceTest()
  {
    FrameworkManager::Instance();
  }

  void checkInputs(std::string strNormalisation)
  {
    MDEventWorkspace3Lean::sptr in_ws = MDEventsTestHelper::makeMDEW<3>(10, -10.0, 20.0, 3);
    QueryMDWorkspace query;
    query.initialize();    
    query.setRethrows(true);
    query.setProperty("InputWorkspace", in_ws);
    query.setProperty("OutputWorkspace", "QueryWS");
    query.setProperty("Normalisation", strNormalisation);
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
      TS_ASSERT_DELTA(signalNotNormalised, signalNormalisedByNumEvents*double(nEvents), 0.0001);
      TS_ASSERT_DELTA(errorNotNormalised, errorNormalisedByNumEvents*double(nEvents), 0.0001);
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
    TSM_ASSERT_EQUALS("Five columns expected", expectedCount, table->columnCount());
    TSM_ASSERT_EQUALS("Wrong number of rows", 3, table->rowCount());
  }

  IMDWorkspace_sptr createSlice() {

      auto in_ws = MDEventsTestHelper::makeMDEW<2>(2, -10.0, 10, 3);

      // Create a line slice at 45 degrees to the original workspace.
      IAlgorithm_sptr binMDAlg = AlgorithmManager::Instance().create("BinMD");
      binMDAlg->setRethrows(true);
      binMDAlg->initialize();
      binMDAlg->setChild(true);
      binMDAlg->setProperty("InputWorkspace", in_ws);
      binMDAlg->setProperty("AxisAligned", false);
      binMDAlg->setPropertyValue("BasisVector0", "X,units,0.7071,0.7071"); // cos 45 to in_ws x-axis (consistent with a 45 degree anti-clockwise rotation)
      binMDAlg->setPropertyValue("BasisVector1", "Y,units,-0.7071,0.7071"); // cos 45 to in_ws y-axis (consistent with a 45 degree anti-clockwise rotation)
      binMDAlg->setPropertyValue("OutputExtents", "0,28.284,-1,1"); // 0 to sqrt((-10-10)^2 + (-10-10)^2), -1 to 1 (in new coordinate axes)
      binMDAlg->setPropertyValue("OutputBins", "10,1");
      binMDAlg->setPropertyValue("OutputWorkspace", "temp");
      binMDAlg->execute();
      Workspace_sptr temp = binMDAlg->getProperty("OutputWorkspace");
      auto slice = boost::dynamic_pointer_cast<IMDWorkspace>(temp);
      return slice;
  }

  void testOnSlice()
  {

    IMDWorkspace_sptr slice = createSlice();

    QueryMDWorkspace query;
    query.setRethrows(true);
    query.setChild(true);
    query.initialize();
    query.setProperty("InputWorkspace", slice);
    query.setPropertyValue("OutputWorkspace", "QueryWS");
    query.execute();
    ITableWorkspace_sptr table =  query.getProperty("OutputWorkspace");

    TSM_ASSERT("Workspace output is not an ITableWorkspace", table !=NULL);
    size_t expectedCount = 3 + 2; //3 fixed columns are Signal, Error, nEvents and then data is 2D
    TSM_ASSERT_EQUALS("Six columns expected", expectedCount, table->columnCount());
    TSM_ASSERT_EQUALS("Wrong number of rows", 10, table->rowCount());

    /*
     Note that what we do in the following is to check that the y and x coordinates are the same. They will ONLY be the same in the
     original coordinate system owing to the way that they have been rotated. If we were displaying the results in the new coordinate system
     then y == 0 and x would increment from 0 to sqrt((-10-10)^2 + (-10-10)^2).
     */
    for(size_t i =0; i < table->rowCount(); ++i)
    {
      auto xColumn = table->getColumn(3);
      auto yColumn = table->getColumn(4);
      double x = xColumn->toDouble(i);
      double y = yColumn->toDouble(i);
      std::stringstream messageBuffer;
      messageBuffer << "X and Y should be equal at row index: " << i;
      TSM_ASSERT_DELTA(messageBuffer.str(), x, y, 1e-3);
    }

  }

  void testOnSlice_without_transform_to_original()
  {
      IMDWorkspace_sptr slice = createSlice();

      QueryMDWorkspace query;
      query.setRethrows(true);
      query.setChild(true);
      query.initialize();
      query.setProperty("TransformCoordsToOriginal", false); // DO NOT, use the original workspace coordinates.
      query.setProperty("InputWorkspace", slice);
      query.setPropertyValue("OutputWorkspace", "QueryWS");
      query.execute();
      ITableWorkspace_sptr table =  query.getProperty("OutputWorkspace");

      TSM_ASSERT("Workspace output is not an ITableWorkspace", table !=NULL);
      size_t expectedCount = 3 + 2; //3 fixed columns are Signal, Error, nEvents and then data is 2D
      TSM_ASSERT_EQUALS("Six columns expected", expectedCount, table->columnCount());
      TSM_ASSERT_EQUALS("Wrong number of rows", 10, table->rowCount());

      /*
       Since we were displaying the results in the new coordinate system
       then y == 0 and x would increment from 0 to sqrt((-10-10)^2 + (-10-10)^2).

       Note that what we do in the following is to check that the y and x coordinates are NOT the same. They will ONLY be the same in the
       original coordinate system owing to the way that they have been rotated.
       */
      const double xMax = std::sqrt( 20 * 20 * 2);
      const double xMin = 0;

      auto xColumn = table->getColumn(3);
      auto yColumn = table->getColumn(4);


      TS_ASSERT_EQUALS(0, yColumn->toDouble(0));// Always zero
      TS_ASSERT_EQUALS(0, yColumn->toDouble(table->rowCount()-1));


      const double binHalfWidth = 1.5;
      TSM_ASSERT_DELTA("From zero", xMin, xColumn->toDouble(0), binHalfWidth /*account for bin widths*/);
      TSM_ASSERT_DELTA("To max", xMax, xColumn->toDouble(table->rowCount()-1), binHalfWidth /*account for bin widths*/);


  }


};


#endif /* MANTID_MDEVENTS_QUERYMDWORKSPACETEST_H_ */
