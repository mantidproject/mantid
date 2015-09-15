#ifndef MANTID_WORKFLOWALGORITHMS_MUONCALCULATEASYMMETRYTEST_H_
#define MANTID_WORKFLOWALGORITHMS_MUONCALCULATEASYMMETRYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidWorkflowAlgorithms/MuonCalculateAsymmetry.h"

using Mantid::WorkflowAlgorithms::MuonCalculateAsymmetry;

using namespace Mantid::Kernel;
using namespace Mantid::API;

class MuonCalculateAsymmetryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonCalculateAsymmetryTest *createSuite() { return new MuonCalculateAsymmetryTest(); }
  static void destroySuite( MuonCalculateAsymmetryTest *suite ) { delete suite; }

  MuonCalculateAsymmetryTest()
  {
    // To make sure everything is loaded
    FrameworkManager::Instance();
  }

  void test_Init()
  {
    MuonCalculateAsymmetry alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_groupCounts_singlePeriod()
  {
    // Name of the output workspace.
    const std::string outWSName = outputWorkspaceName("GroupCounts_SinglePeriod");

    MatrixWorkspace_sptr inWS = createWorkspace();
  
    MuonCalculateAsymmetry alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FirstPeriodWorkspace", inWS) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("OutputType", "GroupCounts") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("GroupIndex", 1) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(ws);

    if (ws)
    {
      TS_ASSERT_EQUALS( ws->getNumberHistograms(), 1 ); 
      TS_ASSERT_EQUALS( ws->blocksize(), 3 );

      TS_ASSERT_EQUALS( ws->readY(0)[0], 4 );
      TS_ASSERT_EQUALS( ws->readY(0)[1], 5 );
      TS_ASSERT_EQUALS( ws->readY(0)[2], 6 );

      TS_ASSERT_EQUALS( ws->readX(0)[0], 1 );
      TS_ASSERT_EQUALS( ws->readX(0)[1], 2 );
      TS_ASSERT_EQUALS( ws->readX(0)[2], 3 );

      TS_ASSERT_DELTA( ws->readE(0)[0], 0.4, 0.01 );
      TS_ASSERT_DELTA( ws->readE(0)[1], 0.5, 0.01 );
      TS_ASSERT_DELTA( ws->readE(0)[2], 0.6, 0.01 );
    }
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_groupCounts_twoPeriods_plus()
  {
    // Name of the output workspace.
    const std::string outWSName = outputWorkspaceName("GroupCounts_TwoPeriods_Plus");

    MatrixWorkspace_sptr inWSFirst = createWorkspace();
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
  
    MuonCalculateAsymmetry alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FirstPeriodWorkspace", inWSFirst) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("SecondPeriodWorkspace", inWSSecond) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("PeriodOperation", "+") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("OutputType", "GroupCounts") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("GroupIndex", 1) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(ws);

    if (ws)
    {
      TS_ASSERT_EQUALS( ws->getNumberHistograms(), 1 ); 
      TS_ASSERT_EQUALS( ws->blocksize(), 3 );

      TS_ASSERT_EQUALS( ws->readY(0)[0], 8 );
      TS_ASSERT_EQUALS( ws->readY(0)[1], 10 );
      TS_ASSERT_EQUALS( ws->readY(0)[2], 12 );

      TS_ASSERT_EQUALS( ws->readX(0)[0], 1 );
      TS_ASSERT_EQUALS( ws->readX(0)[1], 2 );
      TS_ASSERT_EQUALS( ws->readX(0)[2], 3 );

      TS_ASSERT_DELTA( ws->readE(0)[0], 0.566, 0.001 );
      TS_ASSERT_DELTA( ws->readE(0)[1], 0.707, 0.001 );
      TS_ASSERT_DELTA( ws->readE(0)[2], 0.849, 0.001 );
    }
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_groupCounts_twoPeriod_minus()
  {
    // Name of the output workspace.
    const std::string outWSName = outputWorkspaceName("GroupCounts_TwoPeriods_Minus");

    MatrixWorkspace_sptr inWSFirst = createWorkspace(3);
    MatrixWorkspace_sptr inWSSecond = createWorkspace();
  
    MuonCalculateAsymmetry alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FirstPeriodWorkspace", inWSFirst) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("SecondPeriodWorkspace", inWSSecond) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("PeriodOperation", "-") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("OutputType", "GroupCounts") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("GroupIndex", 1) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(ws);

    if (ws)
    {
      TS_ASSERT_EQUALS( ws->getNumberHistograms(), 1 ); 
      TS_ASSERT_EQUALS( ws->blocksize(), 3 );

      TS_ASSERT_EQUALS( ws->readY(0)[0], 3 );
      TS_ASSERT_EQUALS( ws->readY(0)[1], 3 );
      TS_ASSERT_EQUALS( ws->readY(0)[2], 3 );

      TS_ASSERT_EQUALS( ws->readX(0)[0], 1 );
      TS_ASSERT_EQUALS( ws->readX(0)[1], 2 );
      TS_ASSERT_EQUALS( ws->readX(0)[2], 3 );

      TS_ASSERT_DELTA( ws->readE(0)[0], 0.806, 0.001 );
      TS_ASSERT_DELTA( ws->readE(0)[1], 0.943, 0.001 );
      TS_ASSERT_DELTA( ws->readE(0)[2], 1.082, 0.001 );
    }
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_groupAsymmetry()
  {
    // Name of the output workspace.
    const std::string outWSName = outputWorkspaceName("GroupAsymmetry");

    MatrixWorkspace_sptr inWS = createWorkspace();
  
    MuonCalculateAsymmetry alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FirstPeriodWorkspace", inWS) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("OutputType", "GroupAsymmetry") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("GroupIndex", 2) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(ws);

    if (ws)
    {
      TS_ASSERT_EQUALS( ws->getNumberHistograms(), 1 ); 
      TS_ASSERT_EQUALS( ws->blocksize(), 3 );

      TS_ASSERT_DELTA( ws->readY(0)[0], -0.247, 0.001 );
      TS_ASSERT_DELTA( ws->readY(0)[1], 0.356, 0.001 );
      TS_ASSERT_DELTA( ws->readY(0)[2], 1.405, 0.001 );

      TS_ASSERT_EQUALS( ws->readX(0)[0], 1 );
      TS_ASSERT_EQUALS( ws->readX(0)[1], 2 );
      TS_ASSERT_EQUALS( ws->readX(0)[2], 3 );

      TS_ASSERT_DELTA( ws->readE(0)[0], 0.075, 0.01 );
      TS_ASSERT_DELTA( ws->readE(0)[1], 0.136, 0.01 );
      TS_ASSERT_DELTA( ws->readE(0)[2], 0.240, 0.01 );
    }
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_pairAsymmetry()
  {
    // Name of the output workspace.
    const std::string outWSName = outputWorkspaceName("GroupAsymmetry");

    MatrixWorkspace_sptr inWS = createWorkspace();
  
    MuonCalculateAsymmetry alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("FirstPeriodWorkspace", inWS) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("OutputType", "PairAsymmetry") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("PairFirstIndex", 2) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("PairSecondIndex", 0) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Alpha", 0.5) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(ws);

    if (ws)
    {
      TS_ASSERT_EQUALS( ws->getNumberHistograms(), 1 ); 
      TS_ASSERT_EQUALS( ws->blocksize(), 3 );

      TS_ASSERT_DELTA( ws->readY(0)[0], 0.867, 0.001 );
      TS_ASSERT_DELTA( ws->readY(0)[1], 0.778, 0.001 );
      TS_ASSERT_DELTA( ws->readY(0)[2], 0.714, 0.001 );

      TS_ASSERT_EQUALS( ws->readX(0)[0], 1.5 );
      TS_ASSERT_EQUALS( ws->readX(0)[1], 2.5 );
      TS_ASSERT_EQUALS( ws->readX(0)[2], 3 );

      TS_ASSERT_DELTA( ws->readE(0)[0], 0.475, 0.01 );
      TS_ASSERT_DELTA( ws->readE(0)[1], 0.410, 0.01 );
      TS_ASSERT_DELTA( ws->readE(0)[2], 0.365, 0.01 );
    }
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
private:

  std::string outputWorkspaceName(std::string testName)
  {
    return "MuonCalculateAsymmetryTest_" + testName + "_OutputWS";
  }

  /**
   * Creates 3x3 workspace with values:
   *     1 2 3
   *     4 5 6
   *     7 8 9
   *
   * Delta is added to every value if specified.
   * 
   * Errors are the same values but divided by 10.
   * 
   * X values are 1 2 3 for all the histograms.
   */
  MatrixWorkspace_sptr createWorkspace(double delta = 0.0)
  {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::Create2DWorkspace(3,3);

    for ( size_t i = 0; i < ws->getNumberHistograms(); i++ )
    {
      for ( size_t j = 0; j < ws->blocksize(); j++ )
      {
        double v = static_cast<double>(i * ws->blocksize() + j) + 1.0 + delta;

        ws->dataY(i)[j] = v;
        ws->dataX(i)[j] = static_cast<double>(j + 1);
        ws->dataE(i)[j] = v * 0.1;
      }
    }

    return ws;
  }

};


#endif /* MANTID_WORKFLOWALGORITHMS_MUONCALCULATEASYMMETRYTEST_H_ */
