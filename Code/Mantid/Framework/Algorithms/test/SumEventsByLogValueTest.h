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

  void test_init()
  {
    SumEventsByLogValue alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
  }

  void test_non_existent_log_fails()
  {
    auto alg = setupAlg("notthere");
    TS_ASSERT( ! alg->execute() );
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

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
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

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), 1 );
    TS_ASSERT_EQUALS( outWS->readX(0)[0], 1.0 );
    TS_ASSERT_EQUALS( outWS->readY(0)[0], 300.0 );
    TS_ASSERT_EQUALS( outWS->readE(0)[0], std::sqrt(300.0) );
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
