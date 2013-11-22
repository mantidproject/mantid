#ifndef MANTID_WORKFLOWALGORITHMS_MUONLOADCORRECTEDTEST_H_
#define MANTID_WORKFLOWALGORITHMS_MUONLOADCORRECTEDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidWorkflowAlgorithms/MuonLoadCorrected.h"

using Mantid::WorkflowAlgorithms::MuonLoadCorrected;

using namespace Mantid::Kernel;
using namespace Mantid::API;

class MuonLoadCorrectedTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonLoadCorrectedTest *createSuite() { return new MuonLoadCorrectedTest(); }
  static void destroySuite( MuonLoadCorrectedTest *suite ) { delete suite; }

  // Name of the output workspace.
  const std::string g_outWSName; 

  MuonLoadCorrectedTest() : g_outWSName("MuonLoadCorrectedTest_OutputWS")
  {}

  ~MuonLoadCorrectedTest() 
  {
    // Remove workspace from the data service if exists
    if ( AnalysisDataService::Instance().doesExist(g_outWSName) )
      AnalysisDataService::Instance().remove(g_outWSName);
  }

  void test_init()
  {
    MuonLoadCorrected alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_properties()
  {
    MuonLoadCorrected alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT( alg.existsProperty("Filename") )
    TS_ASSERT( alg.existsProperty("DTCType") )
    TS_ASSERT( alg.existsProperty("DTCFile") )
    TS_ASSERT( alg.existsProperty("OutputWorkspace") )
  }
  
  void test_noCorrection()
  {
    MuonLoadCorrected alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", "MUSR00015189") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("DTCType", "None") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", g_outWSName) );

    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(g_outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    TSM_ASSERT( "You have forgot to check the results", 0);
  }
};


#endif /* MANTID_WORKFLOWALGORITHMS_MUONLOADCORRECTEDTEST_H_ */
