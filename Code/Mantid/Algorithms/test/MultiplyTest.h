#ifndef MULTIPLYTEST_H_
#define MULTIPLYTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "WorkspaceCreationHelper.hh"
#include "MantidAlgorithms/Multiply.h"
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

class MultiplyTest : public CxxTest::TestSuite
{
public:

  void testInit()
  {
    Multiply alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    //Setting properties to input workspaces that don't exist throws
    TS_ASSERT_THROWS( alg.setPropertyValue("LHSWorkspace","test_in21"), std::invalid_argument );
    TS_ASSERT_THROWS( alg.setPropertyValue("RHSWorkspace","test_in22"), std::invalid_argument );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace","test_out2") );
  }

  void testExec1D1D()
  {
    int sizex = 5;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    AnalysisDataService::Instance().add("test_in11", work_in1);
    AnalysisDataService::Instance().add("test_in12", work_in2);

    Multiply alg;

    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(
      alg.setPropertyValue("LHSWorkspace","test_in11");
      alg.setPropertyValue("RHSWorkspace","test_in12");    
      alg.setPropertyValue("OutputWorkspace","test_out1");
    )
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
    int sizex = 2,sizey=4;
    // Register the workspace in the data service
    Workspace2D_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);
    Workspace2D_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Multiply alg;

    AnalysisDataService::Instance().add("test_in21", work_in1);
    AnalysisDataService::Instance().add("test_in22", work_in2);
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","test_in21");
    alg.setPropertyValue("RHSWorkspace","test_in22");    
    alg.setPropertyValue("OutputWorkspace","test_out2");
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
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Multiply alg;

    std::string wsName1 = "test_in1D2D21";
    std::string wsName2 = "test_in1D2D22";
    std::string wsNameOut = "test_out1D2D";
    AnalysisDataService::Instance().add(wsName1, work_in1);
    AnalysisDataService::Instance().add(wsName2, work_in2);
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",wsName1);
    alg.setPropertyValue("RHSWorkspace",wsName2);    
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove(wsName1);
    AnalysisDataService::Instance().remove(wsName2);
    AnalysisDataService::Instance().remove(wsNameOut);
   
  }

  void testExec2D1D()
  {
    int sizex = 5,sizey=300;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Multiply alg;

    std::string wsName1 = "test_in2D1D21";
    std::string wsName2 = "test_in2D1D22";
    std::string wsNameOut = "test_out2D1D";
    AnalysisDataService::Instance().add(wsName1, work_in1);
    AnalysisDataService::Instance().add(wsName2, work_in2);
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",wsName1);
    alg.setPropertyValue("RHSWorkspace",wsName2);    
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));

    checkData(work_in2, work_in1, work_out1);

    AnalysisDataService::Instance().remove(wsName1);
    AnalysisDataService::Instance().remove(wsName2);
    AnalysisDataService::Instance().remove(wsNameOut);
   
  }  

  void testExec1DRand2D()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceRand(sizex);
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Multiply alg;

    std::string wsName1 = "test_in1D2Dv1";
    std::string wsName2 = "test_in1D2Dv2";
    std::string wsNameOut = "test_out1D2Dv";
    AnalysisDataService::Instance().add(wsName1, work_in1);
    AnalysisDataService::Instance().add(wsName2, work_in2);
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",wsName1);
    alg.setPropertyValue("RHSWorkspace",wsName2);    
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove(wsName1);
    AnalysisDataService::Instance().remove(wsName2);
    AnalysisDataService::Instance().remove(wsNameOut);
  }

  void testExec2D1DVertical()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(1,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Multiply alg;

    std::string wsName1 = "test_in2D1Dv1";
    std::string wsName2 = "test_in2D1Dv2";
    std::string wsNameOut = "test_out2D1Dv";
    AnalysisDataService::Instance().add(wsName1, work_in1);
    AnalysisDataService::Instance().add(wsName2, work_in2);
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",wsName1);
    alg.setPropertyValue("RHSWorkspace",wsName2);
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));

    checkData(work_in2, work_in1, work_out1);

    AnalysisDataService::Instance().remove(wsName1);
    AnalysisDataService::Instance().remove(wsName2);
    AnalysisDataService::Instance().remove(wsNameOut);    
  }
  
  void testExec2D2DbyOperatorOverload()
  {
    int sizex = 10,sizey=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    MatrixWorkspace_sptr work_out1 = work_in2 * work_in1;

    checkData(work_in1, work_in2, work_out1);
  }

    void testExec1DSingleValue()
  {
    int sizex = 10;
    // Register the workspace in the data service

    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2.2);
    AnalysisDataService::Instance().add("test_in11", work_in1);
    AnalysisDataService::Instance().add("test_in12", work_in2);

    Multiply alg;

    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","test_in11");
    alg.setPropertyValue("RHSWorkspace","test_in12");    
    alg.setPropertyValue("OutputWorkspace","test_out1");
    alg.execute();

    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out1")));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove("test_out1");
    AnalysisDataService::Instance().remove("test_in11");
    AnalysisDataService::Instance().remove("test_in12");

  } 
  
  void testExec2DSingleValue()
  {
    int sizex = 5,sizey=300;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::CreateWorkspaceSingleValue(4.455);

    Multiply alg;

    std::string wsName1 = "test_in2D1D21";
    std::string wsName2 = "test_in2D1D22";
    std::string wsNameOut = "test_out2D1D";
    AnalysisDataService::Instance().add(wsName1, work_in1);
    AnalysisDataService::Instance().add(wsName2, work_in2);
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",wsName1);
    alg.setPropertyValue("RHSWorkspace",wsName2);    
    alg.setPropertyValue("OutputWorkspace",wsNameOut);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT( alg.isExecuted() );
    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsNameOut)));

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove(wsName1);
    AnalysisDataService::Instance().remove(wsName2);
    AnalysisDataService::Instance().remove(wsNameOut);
   
  }

  void testCompoundAssignment()
  {
    MatrixWorkspace_sptr a = WorkspaceCreationHelper::CreateWorkspaceSingleValue(3);
    const Workspace_const_sptr b = a;
    MatrixWorkspace_sptr c = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2);
    a *= 5;
    TS_ASSERT_EQUALS(a->readY(0)[0],15);
    TS_ASSERT_EQUALS(a,b);
    a *= c;
    TS_ASSERT_EQUALS(a->readY(0)[0],30);
    TS_ASSERT_EQUALS(a,b);
  }
  










  // ==================================================================================================================

  void EventSetup()
  {
    // 3 pixels, 100 events, starting at 0.5 in steps of +1.0.
    AnalysisDataService::Instance().addOrReplace("ev1", boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceCreationHelper::CreateEventWorkspace(3, 10,100, 0.0, 1.0, 3)));

    // 3 pixels, 200 events, (two each) starting at 0.5 in steps of +1.0.
    AnalysisDataService::Instance().addOrReplace("ev2", boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceCreationHelper::CreateEventWorkspace(3, 10,100, 0.0, 1.0, 2)));

    //200 events per spectrum, but the spectra are at different pixel ids
    AnalysisDataService::Instance().addOrReplace("ev3", boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateEventWorkspace(3, 10,100, 0.0, 1.0, 2, 100)));
    //Make one with weird units
    MatrixWorkspace_sptr ev4 = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateEventWorkspace(3,10,100, 0.0, 1.0, 2, 100));
    ev4->setYUnit("Microfurlongs per Megafortnights");
    AnalysisDataService::Instance().addOrReplace("ev4_weird_units",ev4);
    //Different # of spectra
    AnalysisDataService::Instance().addOrReplace("ev5", boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateEventWorkspace(5,10,100, 0.0, 1.0, 2, 100))); //200 events per spectrum, but the spectra are at different pixel ids
    //a 2d workspace with the value 2 in each bin
    AnalysisDataService::Instance().addOrReplace("in2D", WorkspaceCreationHelper::Create2DWorkspaceBinned(3, 10, 0.0, 1.0));

    //A single value workspace with the value 3 +- 0.0
    MatrixWorkspace_sptr single_value = WorkspaceCreationHelper::CreateWorkspaceSingleValueWithError(3, 0.0);
    AnalysisDataService::Instance().add("three", single_value);


  }

  void EventTeardown()
  {
    AnalysisDataService::Instance().remove("ev1");
    AnalysisDataService::Instance().remove("ev2");
    AnalysisDataService::Instance().remove("ev3");
    AnalysisDataService::Instance().remove("ev4_weird_units");
    AnalysisDataService::Instance().remove("ev5");
    AnalysisDataService::Instance().remove("in2D");
    AnalysisDataService::Instance().remove("evOUT");
    AnalysisDataService::Instance().remove("out2D");
    AnalysisDataService::Instance().remove("three");
  }

  //-----------------------------------------------------------------------------------------------
  void testExecOneEvent_times_single_value()
  {
    EventWorkspace_sptr in1, in2, out;

    EventSetup();
    Multiply alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","ev1");
    alg.setPropertyValue("RHSWorkspace","three");
    alg.setPropertyValue("OutputWorkspace", "evOUT");
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    out = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("evOUT"));
    TS_ASSERT(out); // still Eventworkspace
    // Still has events, 300 total
    TS_ASSERT_EQUALS( out->getNumberEvents(), 300);
    for (int wi=0; wi < 3; wi++)
    {
      std::vector<WeightedEvent> & rwel = out->getEventListPtr(wi)->getWeightedEvents();
      TS_ASSERT_DELTA( rwel[0].weight(), 3.0, 1e-5); // weight is 3
      TS_ASSERT_DELTA( rwel[0].error(), 3.0, 1e-5); // error is 3

      for (int i=0; i<out->blocksize(); i++)
      {
        TS_ASSERT_EQUALS( out->readY(wi)[i], 3.0); // weight is 3
        TS_ASSERT_EQUALS( out->readE(wi)[i], 3.0); // error is also 3
      }
    }

    EventTeardown();
  }


  //-----------------------------------------------------------------------------------------------
  void testExecTwoEvents_times_single_value()
  {
    EventWorkspace_sptr in1, in2, out;

    EventSetup();
    Multiply alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","ev2");
    alg.setPropertyValue("RHSWorkspace","three");
    alg.setPropertyValue("OutputWorkspace", "evOUT");
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    out = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("evOUT"));
    TS_ASSERT(out); // still Eventworkspace
    for (int wi=0; wi < 3; wi++)
    {
      std::vector<WeightedEvent> & rwel = out->getEventListPtr(wi)->getWeightedEvents();
      TS_ASSERT_DELTA( rwel[0].weight(), 3.0, 1e-5); // weight is 3
      TS_ASSERT_DELTA( rwel[0].error(), 3.0, 1e-5); // error is 3

      for (int i=0; i<out->blocksize(); i++)
      {
        TS_ASSERT_DELTA( out->readY(wi)[i], 6.0, 1e-6); // two events, so 6
        TS_ASSERT_DELTA( out->readE(wi)[i], sqrt(2.0) * 3.0, 1e-6); // relative error is 2 / sqrt(2), since there are two events
      }
    }

    EventTeardown();
  }




  //-----------------------------------------------------------------------------------------------
  void testExecOneEvent_times_histogram()
  {
    EventWorkspace_sptr in1, in2, out;
    EventSetup();

    Multiply alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","ev1");
    alg.setPropertyValue("RHSWorkspace","in2D");
    alg.setPropertyValue("OutputWorkspace", "evOUT");
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    out = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("evOUT"));
    TS_ASSERT(out); // still Eventworkspace
    TS_ASSERT_EQUALS( out->getNumberEvents(), 300); // Still has events, 300 total
    for (int wi=0; wi < 3; wi++)
    {
      // The histogram was 2 +- sqrt(2) at all bins...

      std::vector<WeightedEvent> & rwel = out->getEventListPtr(wi)->getWeightedEvents();
      TS_ASSERT_DELTA( rwel[0].weight(), 2.0, 1e-5); // weight is twice
      TS_ASSERT_DELTA( rwel[0].errorSquared(), 2.0+1, 1e-5); //error is sqrt(3)

      for (int i=0; i<out->blocksize(); i++)
      {
        TS_ASSERT_EQUALS( out->readY(wi)[i], 2.0);
        TS_ASSERT_EQUALS( out->readE(wi)[i], sqrt(3.0));
      }
    }

    EventTeardown();
  }



  //-----------------------------------------------------------------------------------------------
  void testExecOneEvent_times_TwoEvents()
  {
    EventWorkspace_sptr in1, in2, out;
    EventSetup();

    Multiply alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","ev1");
    alg.setPropertyValue("RHSWorkspace","ev2");
    alg.setPropertyValue("OutputWorkspace", "evOUT");
    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    out = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("evOUT"));
    TS_ASSERT(out); // still Eventworkspace
    TS_ASSERT_EQUALS( out->getNumberEvents(), 300); // Still has events, 300 total
    for (int wi=0; wi < 3; wi++)
    {
      // The histogram was 2 +- sqrt(2) at all bins...

      std::vector<WeightedEvent> & rwel = out->getEventListPtr(wi)->getWeightedEvents();
      TS_ASSERT_DELTA( rwel[0].weight(), 2.0, 1e-5); // weight is twice
      TS_ASSERT_DELTA( rwel[0].errorSquared(), 2.0+1, 1e-5); //error is sqrt(3)

      for (int i=0; i<out->blocksize(); i++)
      {
        TS_ASSERT_EQUALS( out->readY(wi)[i], 2.0);
        TS_ASSERT_EQUALS( out->readE(wi)[i], sqrt(3.0));
      }
    }

    EventTeardown();
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
    int ws2LoopCount;
    if (work_in2->size() > 0)
    {
      ws2LoopCount = work_in1->size()/work_in2->size();
    }
    ws2LoopCount = (ws2LoopCount==0) ? 1 : ws2LoopCount;

    for (int i = 0; i < work_out1->size(); i++)
    {
      int ws2Index = i;
    
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

  void checkDataItem (MatrixWorkspace_sptr work_in1,  MatrixWorkspace_sptr work_in2, MatrixWorkspace_sptr work_out1, int i, int ws2Index)
  {
      double sig1 = work_in1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double sig2 = work_in2->dataY(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()];
      double sig3 = work_out1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      TS_ASSERT_DELTA(work_in1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()],
        work_out1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
      TS_ASSERT_DELTA(sig1 * sig2, sig3, 0.0001);
      double err1 = work_in1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double err2 = work_in2->dataE(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()]; 
      // (Sa/a)2 + (Sb/b)2 = (Sc/c)2 
      //  So after taking proportions, squaring, summing, 
      //  and taking the square root, you get a proportional error to the product c. Multiply that proportional error by c to get the actual standard deviation Sc. 
      double err3(sig3 * sqrt(((err1/sig1)*(err1/sig1)) + ((err2/sig2)*(err2/sig2))));     
      TS_ASSERT_DELTA(err3, work_out1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
  }

};

#endif /*MULTIPLYTEST_H_*/
