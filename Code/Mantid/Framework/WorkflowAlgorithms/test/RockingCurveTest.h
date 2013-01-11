#ifndef MANTID_WORKFLOWALGORITHMS_ROCKINGCURVETEST_H_
#define MANTID_WORKFLOWALGORITHMS_ROCKINGCURVETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidWorkflowAlgorithms/RockingCurve.h"
#include "MantidAlgorithms/FilterByXValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::WorkflowAlgorithms::RockingCurve;
using Mantid::DataObjects::EventWorkspace_sptr;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class RockingCurveTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RockingCurveTest *createSuite() { return new RockingCurveTest(); }
  static void destroySuite( RockingCurveTest *suite ) { delete suite; }

  // Just a simple test on a very small workspace - leave more extensive testing for system tests
  void test_simple_case()
  {
    EventWorkspace_sptr ws = WorkspaceCreationHelper::CreateEventWorkspace2(3,1);
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    auto scan_index = new TimeSeriesProperty<int>("scan_index");
    scan_index->addValue("2010-01-01T00:00:00",0);
    scan_index->addValue("2010-01-01T00:00:30",1);
    // TODO: 'Close' the log, but I need to think about what happens if it isn't closed
    scan_index->addValue("2010-01-01T00:01:40",0);
    ws->mutableRun().addProperty(scan_index);
    auto prop = new TimeSeriesProperty<double>("sample_property");
    // This log goes from 1->5 half way through the scan_index=1 period (so average will be 3)
    prop->addValue("2010-01-01T00:00:00",1.0);
    prop->addValue("2010-01-01T00:01:05",5.0);
    ws->mutableRun().addProperty(prop);

    // Create a workspace to mask out one of the spectra
    MatrixWorkspace_sptr mask = WorkspaceFactory::Instance().create("MaskWorkspace",3,1,1);
    mask->dataY(1)[0] = 1;

    RockingCurve alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", ws) );
    const std::string outWSName("outTable");
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("MaskWorkspace", mask) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("XMin", 40.0) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("XMax", 90.0) );
    TS_ASSERT( alg.execute() );
    
    // Retrieve the output table workspace from the ADS.
    Mantid::API::ITableWorkspace_sptr table;
    TS_ASSERT_THROWS_NOTHING( table = AnalysisDataService::Instance().retrieveWS<Mantid::API::ITableWorkspace>(outWSName) );
    TS_ASSERT( table );
    if (!table) return;
    
    TS_ASSERT_EQUALS( table->rowCount(), 1 )
    TS_ASSERT_EQUALS( table->columnCount(), 5 )
    TS_ASSERT_EQUALS( table->getColumnNames()[0], "scan_index" );
    TS_ASSERT_EQUALS( table->Int(0,0), 1 )
    TS_ASSERT_EQUALS( table->getColumnNames()[1], "Counts" );
    // The original workspace has 600 events.
    // The scan_index=1 period covers 70 out of 100s -> so 420 events remain
    // The masking removes 1 of 3 spectra -> leaving 280
    // The XMin/XMax range covers 50s out of the remaining 70s TOF range
    //   (note that there's a correlation between pulse time & TOF) -> so 200 are left at the end
    TS_ASSERT_EQUALS( table->Int(0,1), 200 )
    TS_ASSERT_EQUALS( table->getColumnNames()[2], "Time" );
    TS_ASSERT_EQUALS( table->Double(0,2), 70.0);
    TS_ASSERT_EQUALS( table->getColumnNames()[3], "proton_charge" );
    // The cell in the proton_charge column will be empty
    TS_ASSERT_EQUALS( table->getColumnNames()[4], "sample_property" );
    TS_ASSERT_EQUALS( table->Double(0,4), 3.0 );

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
};


#endif /* MANTID_WORKFLOWALGORITHMS_ROCKINGCURVETEST_H_ */
