#ifndef MANTID_ALGORITHMS_CONVERTAXESTOREALSPACETEST_H_
#define MANTID_ALGORITHMS_CONVERTAXESTOREALSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/ConvertAxesToRealSpace.h"

using Mantid::Algorithms::ConvertAxesToRealSpace;
using namespace Mantid::API;

class ConvertAxesToRealSpaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertAxesToRealSpaceTest *createSuite() { return new ConvertAxesToRealSpaceTest(); }
  static void destroySuite( ConvertAxesToRealSpaceTest *suite ) { delete suite; }


  void test_Init()
  {
    ConvertAxesToRealSpace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec_y_theta()
  {
    std::string baseWSName("ConvertAxesToRealSpaceTest_exec_y_theta");
    MatrixWorkspace_sptr ws = do_algorithm_run(baseWSName,"y", "theta", 50,10);
    if (!ws) return;
  }  

  void test_exec_x_y()
  {
    std::string baseWSName("ConvertAxesToRealSpaceTest_exec_x_y");
    do_algorithm_run(baseWSName,"x", "y", 60,20);
  }

  void test_exec_z_2theta()
  {
    std::string baseWSName("ConvertAxesToRealSpaceTest_exec_z_2theta");
    do_algorithm_run(baseWSName,"z", "2theta", 6,2);
  }

  void test_exec_r_signed2theta()
  {
    std::string baseWSName("ConvertAxesToRealSpaceTest_exec_phi_signed2theta");
    do_algorithm_run(baseWSName,"phi", "signed2theta", 100,200);
  }
  
  MatrixWorkspace_sptr do_algorithm_run(std::string baseWSName, std::string verticalAxis, std::string horizontalAxis, 
    int nHBins, int nVBins)
  {
    std::string inWSName(baseWSName+"_InputWS");
    std::string outWSName(baseWSName+"_OutputWS");
    
    auto testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(3, 2, false);
    AnalysisDataService::Instance().addOrReplace(inWSName, testWS);
 
    Mantid::Algorithms::ConvertAxesToRealSpace conv;

    conv.initialize();

    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("InputWorkspace",inWSName) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("OutputWorkspace",outWSName) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("VerticalAxis", verticalAxis) );
    TS_ASSERT_THROWS_NOTHING( conv.setPropertyValue("HorizontalAxis", horizontalAxis) );
    TS_ASSERT_THROWS_NOTHING( conv.setProperty("NumberVerticalBins", nVBins) );
    TS_ASSERT_THROWS_NOTHING( conv.setProperty("NumberHorizontalBins", nHBins) );

    TS_ASSERT_THROWS_NOTHING( conv.execute() );
    TS_ASSERT( conv.isExecuted() );
    
    //remove input workspace
    AnalysisDataService::Instance().remove(inWSName);

    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return ws;

    //check basics
    TS_ASSERT_EQUALS(nHBins,ws->blocksize());
    TS_ASSERT_EQUALS(nVBins,ws->getNumberHistograms());
    TS_ASSERT_EQUALS(verticalAxis,ws->getAxis(1)->unit()->caption());
    TS_ASSERT_EQUALS(horizontalAxis,ws->getAxis(0)->unit()->caption());

    // Remove output workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);

    return ws;
  }


};


#endif /* MANTID_ALGORITHMS_CONVERTAXESTOREALSPACETEST_H_ */