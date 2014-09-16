#ifndef MANTID_SINQ_POLDICREATEPEAKSFROMCELLTEST_H_
#define MANTID_SINQ_POLDICREATEPEAKSFROMCELLTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiCreatePeaksFromCell.h"

using Mantid::SINQ::PoldiCreatePeaksFromCell;
using namespace Mantid::API;

class PoldiCreatePeaksFromCellTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiCreatePeaksFromCellTest *createSuite() { return new PoldiCreatePeaksFromCellTest(); }
  static void destroySuite( PoldiCreatePeaksFromCellTest *suite ) { delete suite; }


  void test_Init()
  {
    PoldiCreatePeaksFromCell alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("PoldiCreatePeaksFromCellTest_OutputWS");

    PoldiCreatePeaksFromCell alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PointGroup", "m-3m (Cubic)") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("LatticeCentering", "Primitive"))
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service. TODO: Change to your desired type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    // TODO: Check the results

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_SINQ_POLDICREATEPEAKSFROMCELLTEST_H_ */
