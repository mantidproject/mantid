#ifndef MANTID_ALGORITHMS_SUMEVENTSBYLOGVALUETEST_H_
#define MANTID_ALGORITHMS_SUMEVENTSBYLOGVALUETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SumEventsByLogValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::SumEventsByLogValue;
using Mantid::DataObjects::EventWorkspace_sptr;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class SumEventsByLogValueTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SumEventsByLogValueTest *createSuite() { return new SumEventsByLogValueTest(); }
  static void destroySuite( SumEventsByLogValueTest *suite ) { delete suite; }

  void test_validators()
  {
    SumEventsByLogValue alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );

    // InputWorkspace has to be an EventWorkspace
    TS_ASSERT_THROWS( alg.setProperty("InputWorkspace", WorkspaceCreationHelper::Create2DWorkspace(1,1)), std::invalid_argument );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", WorkspaceCreationHelper::CreateEventWorkspace()) );

    // LogName must not be empty
    TS_ASSERT_THROWS( alg.setProperty("LogName",""), std::invalid_argument );
  }

  void test_validateInputs()
  {
    // Create and event workspace. We don't care what data is in it.
    EventWorkspace_sptr ws = WorkspaceCreationHelper::CreateEventWorkspace();
    // Add a single-number log
    ws->mutableRun().addProperty("SingleValue",5);
    // Add a time-series property
    auto tsp = new TimeSeriesProperty<double>("TSP");
    tsp->addValue(DateAndTime::getCurrentTime(),9.9);
    ws->mutableRun().addLogData(tsp);
    // Add an EMPTY time-series property
    auto emptyTSP = new TimeSeriesProperty<int>("EmptyTSP");
    ws->mutableRun().addLogData(emptyTSP);

    SumEventsByLogValue alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace",ws) );

    // Check protest when non-existent log is set
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("LogName", "NotThere") );
    auto errorMap = alg.validateInputs();
    TS_ASSERT_EQUALS( errorMap.size(), 1);
    TS_ASSERT_EQUALS( errorMap.begin()->first, "LogName" );

    // Check protest when single-value log is set
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("LogName", "SingleValue") );
    errorMap = alg.validateInputs();
    TS_ASSERT_EQUALS( errorMap.size(), 1);
    TS_ASSERT_EQUALS( errorMap.begin()->first, "LogName" );

    // Check protest when empty tsp log given
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("LogName", "emptyTSP") );
    errorMap = alg.validateInputs();
    TS_ASSERT_EQUALS( errorMap.size(), 1);
    TS_ASSERT_EQUALS( errorMap.begin()->first, "LogName" );

    // Check it's happy when a non-empty tsp is given
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("LogName", "TSP") );
    errorMap = alg.validateInputs();
    TS_ASSERT( errorMap.empty() );
  }

  void test_text_property()
  {
    auto alg = setupAlg("textProp");
    TS_ASSERT( ! alg->execute() );
  }

  void test_double_property_fails_if_no_rebin_parameters()
  {
    auto alg = setupAlg("doubleProp");
    TS_ASSERT( ! alg->execute() );
  }

  void test_double_property()
  {
    auto alg = setupAlg("doubleProp");
    alg->setChild(true);
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("OutputBinning","2.5,1,3.5") );
    TS_ASSERT( alg->execute() );

    Workspace_const_sptr out = alg->getProperty("OutputWorkspace");
    MatrixWorkspace_const_sptr outWS = boost::dynamic_pointer_cast<const MatrixWorkspace>(out);
    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), 1 );
    TS_ASSERT_EQUALS( outWS->readY(0)[0], 300 );
  }

  void test_double_property_with_number_of_bins_only()
  {
    auto alg = setupAlg("doubleProp");
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("OutputBinning","3") );
    TS_ASSERT( alg->execute() );
  }

  void test_integer_property()
  {
    auto alg = setupAlg("integerProp");
    alg->setChild(true);
    TS_ASSERT( alg->execute() );

    Workspace_sptr out = alg->getProperty("OutputWorkspace");
    auto outWS = boost::dynamic_pointer_cast<ITableWorkspace>(out);
    TS_ASSERT_EQUALS( outWS->rowCount(), 1 );
    TS_ASSERT_EQUALS( outWS->columnCount(), 2 );
    TS_ASSERT_EQUALS( outWS->Int(0,0), 1 );
    TS_ASSERT_EQUALS( outWS->Int(0,1), 300 );
  }

private:
  IAlgorithm_sptr setupAlg(const std::string & logName)
  {
    IAlgorithm_sptr alg = boost::make_shared<SumEventsByLogValue>();
    alg->initialize();
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("InputWorkspace",createWorkspace()) );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("OutputWorkspace","outws") );
    TS_ASSERT_THROWS_NOTHING( alg->setProperty("LogName",logName) );

    return alg;
  }

  EventWorkspace_sptr createWorkspace()
  {
    EventWorkspace_sptr ws = WorkspaceCreationHelper::CreateEventWorkspace(3,1);
    Run & run = ws->mutableRun();

    // TODO: Use more complex properties (i.e. more that one value!)
    auto dblTSP = new TimeSeriesProperty<double>("doubleProp");
    dblTSP->addValue("2010-01-01T00:00:00", 3.0);
    run.addProperty(dblTSP);

    auto textTSP = new TimeSeriesProperty<std::string>("textProp");
    textTSP->addValue("2010-01-01T00:00:00", "ON");
    run.addProperty(textTSP);
    
    auto intTSP = new TimeSeriesProperty<int>("integerProp");
    intTSP->addValue("2010-01-01T00:00:00", 1);
    run.addProperty(intTSP);

    return ws;
  }

};


#endif /* MANTID_ALGORITHMS_SUMEVENTSBYLOGVALUETEST_H_ */
