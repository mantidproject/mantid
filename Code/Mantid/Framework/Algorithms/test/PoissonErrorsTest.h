#ifndef POISSONERRORSTEST_H_
#define POISSONERRORSTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/PoissonErrors.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceOpOverloads.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class PoissonErrorsTest : public CxxTest::TestSuite
{
public:

	PoissonErrorsTest()
	{
		inputProp1 = "InputWorkspace";
		inputProp2 = "CountsWorkspace";;
		outputProp = "OutputWorkspace";;
	}

  void testInit()
  {
    PoissonErrors alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    //Setting properties to input workspaces that don't exist throws
    TS_ASSERT_THROWS( alg.setPropertyValue(inputProp1,"test_in21"), std::invalid_argument );
    TS_ASSERT_THROWS( alg.setPropertyValue(inputProp2,"test_in22"), std::invalid_argument );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue(outputProp,"test_out2") );
  }

  void testExec1D1D()
  {
    int nBins = 10;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    AnalysisDataService::Instance().add("test_in11", work_in1);
    AnalysisDataService::Instance().add("test_in12", work_in2);

    PoissonErrors alg;

    alg.initialize();
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue(inputProp1,"test_in11") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue(inputProp2,"test_in12") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue(outputProp,"test_out1") );
    alg.execute();

    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out1")));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove("test_out1");
    AnalysisDataService::Instance().remove("test_in11");
    AnalysisDataService::Instance().remove("test_in12");

  }

  void testExec1D1DRand()
  {
    int nBins = 10;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceRand(nBins);
    AnalysisDataService::Instance().add("test_in11", work_in1);
    AnalysisDataService::Instance().add("test_in12", work_in2);

    PoissonErrors alg;

    alg.initialize();
    alg.setPropertyValue(inputProp1,"test_in11");
    alg.setPropertyValue(inputProp2,"test_in12");    
    alg.setPropertyValue(outputProp,"test_out1");
    alg.execute();

    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out1")));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove("test_out1");
    AnalysisDataService::Instance().remove("test_in11");
    AnalysisDataService::Instance().remove("test_in12");

  }


  void testExec2D2D()
  {
    int nHist = 10,nBins=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace123(nHist,nBins);

    PoissonErrors alg;

    AnalysisDataService::Instance().add("test_in21", work_in1);
    AnalysisDataService::Instance().add("test_in22", work_in2);
    alg.initialize();
    alg.setPropertyValue(inputProp1,"test_in21");
    alg.setPropertyValue(inputProp2,"test_in22");    
    alg.setPropertyValue(outputProp,"test_out2");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out2")));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove("test_in21");
    AnalysisDataService::Instance().remove("test_in22");
    AnalysisDataService::Instance().remove("test_out2");

  }

  void testExec1D2D()
  {
    int nHist = 10,nBins=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(nHist,nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);

    PoissonErrors alg;

    std::string wsName1 = "test_in1D2D21";
    std::string wsName2 = "test_in1D2D22";
    std::string wsNameOut = "test_out1D2D";
    AnalysisDataService::Instance().add(wsName1, work_in1);
    AnalysisDataService::Instance().add(wsName2, work_in2);
    alg.initialize();
    alg.setPropertyValue(inputProp1,wsName1);
    alg.setPropertyValue(inputProp2,wsName2);    
    alg.setPropertyValue(outputProp,wsNameOut);
		//this should fail
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( !alg.isExecuted() );
    
   
  }

  void testExec1DRand2DVertical()
  {
    int nHist = 10,nBins=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceRand(nBins);
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(nHist,nBins);

    PoissonErrors alg;

    std::string wsName1 = "test_in1D2Dv1";
    std::string wsName2 = "test_in1D2Dv2";
    std::string wsNameOut = "test_out1D2Dv";
    AnalysisDataService::Instance().add(wsName1, work_in1);
    AnalysisDataService::Instance().add(wsName2, work_in2);
    alg.initialize();
    alg.setPropertyValue(inputProp1,wsName1);
    alg.setPropertyValue(inputProp2,wsName2);    
    alg.setPropertyValue(outputProp,wsNameOut);
		//this should fail
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( !alg.isExecuted() );
  }
  

    void testExec1DSingleValue()
  {
    int nBins = 10;
    // Register the workspace in the data service

    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.2);
    AnalysisDataService::Instance().add("test_in11", work_in1);
    AnalysisDataService::Instance().add("test_in12", work_in2);

    PoissonErrors alg;

    alg.initialize();
    alg.setPropertyValue(inputProp1,"test_in11");
    alg.setPropertyValue(inputProp2,"test_in12");    
    alg.setPropertyValue(outputProp,"test_out1");
		//this should fail
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( !alg.isExecuted() );

    AnalysisDataService::Instance().remove("test_in11");
    AnalysisDataService::Instance().remove("test_in12");

  } 
  
  void testExec2DSingleValue()
  {
    int nBins=300;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(nBins);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(4.455);

    PoissonErrors alg;

    std::string wsName1 = "test_in2D1D21";
    std::string wsName2 = "test_in2D1D22";
    std::string wsNameOut = "test_out2D1D";
    AnalysisDataService::Instance().add(wsName1, work_in1);
    AnalysisDataService::Instance().add(wsName2, work_in2);
    alg.initialize();
    alg.setPropertyValue(inputProp1,wsName1);
    alg.setPropertyValue(inputProp2,wsName2);    
    alg.setPropertyValue(outputProp,wsNameOut);
		//this should fail
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( !alg.isExecuted() );

    AnalysisDataService::Instance().remove(wsName1);
    AnalysisDataService::Instance().remove(wsName2);
    AnalysisDataService::Instance().remove(wsNameOut);
   
  }

  
private:
  void checkData( MatrixWorkspace_sptr work_in1,  MatrixWorkspace_sptr work_in2, MatrixWorkspace_sptr work_out1)
  {
    //default to a horizontal loop orientation
    checkData(work_in1,work_in2,work_out1,0);
  }

  // loopOrientation 0=Horizontal, 1=Vertical
  void checkData( MatrixWorkspace_sptr work_in1,  MatrixWorkspace_sptr work_in2, MatrixWorkspace_sptr work_out1, int loopOrientation)
  {
    size_t ws2LoopCount;
    if (work_in2->size() > 0)
    {
      ws2LoopCount = work_in1->size()/work_in2->size();
    }
    ws2LoopCount = (ws2LoopCount==0) ? 1 : ws2LoopCount;

    for (size_t i = 0; i < work_out1->size(); i++)
    {
      size_t ws2Index = i;
    
      if (ws2LoopCount > 1)
      {
        if (loopOrientation == 0)
        {
          ws2Index = i%ws2LoopCount;
        }
        else
        {
          ws2Index = i/ws2LoopCount;
        }
      }
      checkDataItem(work_in1,work_in2,work_out1,i,ws2Index);
    }
  }

  void checkDataItem (MatrixWorkspace_sptr work_in1,  MatrixWorkspace_sptr work_in2, MatrixWorkspace_sptr work_out1, size_t i, size_t ws2Index)
  {
    //printf("I=%d\tws2Index=%d\n",i,ws2Index);
      double sig1 = work_in1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double sig2 = work_in2->dataY(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()];
      double sig2e = work_in2->dataE(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()];
      double sig3 = work_out1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      TS_ASSERT_DELTA(work_in1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()],
        work_out1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
      TS_ASSERT_DELTA(sig1, sig3, 0.0001);
			//double err1 = work_in1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()];
      //double err2 = work_in2->dataE(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()];
      double err3((sig2e/sig2)*sig3);     
      TS_ASSERT_DELTA(err3, work_out1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
  }

	private:
		std::string inputProp1;
		std::string inputProp2;
		std::string outputProp;


};

#endif /*POISSONERRORSTEST_H_*/
