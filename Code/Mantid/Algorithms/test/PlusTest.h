#ifndef PLUSTEST_H_
#define PLUSTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/Plus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class PlusTest : public CxxTest::TestSuite
{
public:

  void testInit()
  {
    Plus alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(
    alg.setPropertyValue("InputWorkspace_1","test_in21");
    alg.setPropertyValue("InputWorkspace_2","test_in22");    
    alg.setPropertyValue("OutputWorkspace","test_out2");
    );

  }
  
  void testExec1D1D()
  {
    int sizex = 10;
    // Register the workspace in the data service
    AnalysisDataService* ADS = AnalysisDataService::Instance();
    Workspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    Workspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    ADS->add("test_in11", work_in1);
    ADS->add("test_in12", work_in2);

    Plus plus_alg;

    plus_alg.initialize();
    plus_alg.setPropertyValue("InputWorkspace_1","test_in11");
    plus_alg.setPropertyValue("InputWorkspace_2","test_in12");    
    plus_alg.setPropertyValue("OutputWorkspace","test_out1");
    plus_alg.execute();

    Workspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = ADS->retrieve("test_out1"));

    checkData(work_in1, work_in2, work_out1);

    ADS->remove("test_out1");
    ADS->remove("test_in11");
    ADS->remove("test_in12");

  }

  void testExec2D2D()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    AnalysisDataService* ADS = AnalysisDataService::Instance();
    Workspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);
    Workspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Plus plus_alg;

    ADS->add("test_in21", work_in1);
    ADS->add("test_in22", work_in2);
    plus_alg.initialize();
    plus_alg.setPropertyValue("InputWorkspace_1","test_in21");
    plus_alg.setPropertyValue("InputWorkspace_2","test_in22");    
    plus_alg.setPropertyValue("OutputWorkspace","test_out2");
    TS_ASSERT_THROWS_NOTHING(plus_alg.execute());
    TS_ASSERT( plus_alg.isExecuted() );
    Workspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = ADS->retrieve("test_out2"));

    checkData(work_in1, work_in2, work_out1);

    ADS->remove("test_in21");
    ADS->remove("test_in22");
    ADS->remove("test_out2");
   
  }

  
  void testExec1D2D()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    AnalysisDataService* ADS = AnalysisDataService::Instance();
    Workspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    Workspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Plus plus_alg;

    std::string wsName1 = "test_in1D2D21";
    std::string wsName2 = "test_in1D2D22";
    std::string wsNameOut = "test_out1D2D";
    ADS->add(wsName1, work_in1);
    ADS->add(wsName2, work_in2);
    plus_alg.initialize();
    plus_alg.setPropertyValue("InputWorkspace_1",wsName1);
    plus_alg.setPropertyValue("InputWorkspace_2",wsName2);    
    plus_alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(plus_alg.execute());
    TS_ASSERT( plus_alg.isExecuted() );
    Workspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = ADS->retrieve(wsNameOut));

    checkData(work_in1, work_in2, work_out1);

    ADS->remove(wsName1);
    ADS->remove(wsName2);
    ADS->remove(wsNameOut);
   
  }

  void testExec2D1D()
  {
    int sizex = 5,sizey=300;
    // Register the workspace in the data service
    AnalysisDataService* ADS = AnalysisDataService::Instance();
    Workspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    Workspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Plus plus_alg;

    std::string wsName1 = "test_in2D1D21";
    std::string wsName2 = "test_in2D1D22";
    std::string wsNameOut = "test_out2D1D";
    ADS->add(wsName1, work_in1);
    ADS->add(wsName2, work_in2);
    plus_alg.initialize();
    plus_alg.setPropertyValue("InputWorkspace_1",wsName1);
    plus_alg.setPropertyValue("InputWorkspace_2",wsName2);    
    plus_alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(plus_alg.execute());
    TS_ASSERT( plus_alg.isExecuted() );
    Workspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = ADS->retrieve(wsNameOut));

    checkData(work_in2, work_in1, work_out1);

    ADS->remove(wsName1);
    ADS->remove(wsName2);
    ADS->remove(wsNameOut);
   
  }

  void checkData( Workspace_sptr work_in1,  Workspace_sptr work_in2, Workspace_sptr work_out1)
  {
    int ws2LoopCount;
    if (work_in2->size() > 0)
    {
      ws2LoopCount = work_in1->size()/work_in2->size();
    }
    ws2LoopCount = (ws2LoopCount==0) ? 1 : ws2LoopCount;

    for (int i = 0; i < work_out1->size(); i++)
    {
      checkDataItem(work_in1,work_in2,work_out1,i,i/ws2LoopCount);
    }
  }

  void checkDataItem (Workspace_sptr work_in1,  Workspace_sptr work_in2, Workspace_sptr work_out1, int i, int ws2Index)
  {
      double sig1 = work_in1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double sig2 = work_in2->dataY(ws2Index/work_in1->blocksize())[ws2Index%work_in1->blocksize()];
      double sig3 = work_out1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      TS_ASSERT_DELTA(work_in1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()],
        work_out1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
      TS_ASSERT_DELTA(sig1 + sig2, sig3, 0.0001);
      double err1 = work_in1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double err2 = work_in2->dataE(ws2Index/work_in2->blocksize())[ws2Index%work_in1->blocksize()];
      double err3(sqrt((err1*err1) + (err2*err2)));     
      TS_ASSERT_DELTA(err3, work_out1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
  }
  
};

#endif /*PLUSTEST_H_*/
