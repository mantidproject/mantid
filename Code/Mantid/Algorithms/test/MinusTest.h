#ifndef MINUSTEST_H_
#define MINUSTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/Minus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/WorkspaceProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class MinusOpTest : public Algorithm
{
public:

  MinusOpTest() : Algorithm() {}
  virtual ~MinusOpTest() {}
  void init() 
  {
    declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace_1","",Direction::Input));
    declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace_2","",Direction::Input));
    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));   
  }
  void exec() 
  {
    Workspace_sptr in_work1 = getProperty("InputWorkspace_1");
    Workspace_sptr in_work2 = getProperty("InputWorkspace_2");

    Workspace_sptr out_work = in_work1 - in_work2;
    setProperty("OutputWorkspace",out_work);
  }
};

class MinusTest : public CxxTest::TestSuite
{
public:

  void testInit()
  {
    Minus alg;
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

    Minus alg;

    alg.initialize();
    alg.setPropertyValue("InputWorkspace_1","test_in11");
    alg.setPropertyValue("InputWorkspace_2","test_in12");    
    alg.setPropertyValue("OutputWorkspace","test_out1");
    alg.execute();

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
    Workspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);
    Workspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);

    Minus alg;

    ADS->add("test_in21", work_in1);
    ADS->add("test_in22", work_in2);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace_1","test_in21");
    alg.setPropertyValue("InputWorkspace_2","test_in22");    
    alg.setPropertyValue("OutputWorkspace","test_out2");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
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

    Minus alg;

    std::string wsName1 = "test_in1D2D21";
    std::string wsName2 = "test_in1D2D22";
    std::string wsNameOut = "test_out1D2D";
    ADS->add(wsName1, work_in1);
    ADS->add(wsName2, work_in2);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace_1",wsName1);
    alg.setPropertyValue("InputWorkspace_2",wsName2);    
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    Workspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = ADS->retrieve(wsNameOut));

    checkData(work_in1, work_in2, work_out1);

    ADS->remove(wsName1);
    ADS->remove(wsName2);
    ADS->remove(wsNameOut);
   
  }
  
  void testExec2D2DbyOperatorOverload()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    AnalysisDataService* ADS = AnalysisDataService::Instance();
    Workspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);
    Workspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    MinusOpTest alg;

    std::string wsNameIn1 = "testExec2D2DbyOperatorOverload_in21";
    std::string wsNameIn2 = "testExec2D2DbyOperatorOverload_in22";
    std::string wsNameOut = "testExec2D2DbyOperatorOverload_out";
    ADS->add(wsNameIn1, work_in1);
    ADS->add(wsNameIn2, work_in2);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace_1",wsNameIn1);
    alg.setPropertyValue("InputWorkspace_2",wsNameIn2);    
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    Workspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = ADS->retrieve(wsNameOut));

    checkData(work_in1, work_in2, work_out1);

    ADS->remove(wsNameIn1);
    ADS->remove(wsNameIn2);
    ADS->remove(wsNameOut);
  }

private:

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
      TS_ASSERT_DELTA(sig1 - sig2, sig3, 0.0001);
      double err1 = work_in1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double err2 = work_in2->dataE(ws2Index/work_in2->blocksize())[ws2Index%work_in1->blocksize()];
      double err3(sqrt((err1*err1) + (err2*err2)));     
      TS_ASSERT_DELTA(err3, work_out1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
  }

};

#endif /*MINUSTEST_H_*/
