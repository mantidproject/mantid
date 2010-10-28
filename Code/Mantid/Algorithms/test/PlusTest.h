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
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceOpOverloads.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class PlusTest : public CxxTest::TestSuite
{

public:
  //Constructor
  PlusTest()
  {
  }


  void testInit()
  {
    Plus alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    //Setting properties to input workspaces that don't exist throws
    TS_ASSERT_THROWS( alg.setPropertyValue("LHSWorkspace","test_in21"), std::invalid_argument );
    TS_ASSERT_THROWS( alg.setPropertyValue("RHSWorkspace","test_in22"), std::invalid_argument );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace","test_out2") );
  }
  
  void testExec1D1D()
  {
    int sizex = 10;
    // Register the workspace in the data service

    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    AnalysisDataService::Instance().add("test_in11", work_in1);
    AnalysisDataService::Instance().add("test_in12", work_in2);

    Plus alg;

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

  void testExec1D1DRand()
  {
    int sizex = 10;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceRand(sizex);
    AnalysisDataService::Instance().add("test_in11", work_in1);
    AnalysisDataService::Instance().add("test_in12", work_in2);

    Plus alg;

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

  void testExec2D2D()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Plus alg;

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

  
  void testExec2D2DHist()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey,true);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey,true);

    Plus alg;

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
    TS_ASSERT_EQUALS(work_out1->dataX(0).size(),work_in1->dataX(0).size());
    TS_ASSERT_DELTA(work_out1->dataX(0)[work_out1->dataX(0).size()-1],1, 0.00001);
    TS_ASSERT_EQUALS(work_out1->dataY(0).size(),work_in1->dataY(0).size());
    TS_ASSERT_EQUALS(work_out1->dataE(0).size(),work_in1->dataE(0).size());


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

    Plus alg;

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

  void testExec1DRand2D()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceRand(sizex);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Plus alg;

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

  void testExec1DRand2DVertical()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create1DWorkspaceRand(sizex);
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Plus alg;

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

  void testExec2D1D()
  {
    int sizex = 5,sizey=300;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create1DWorkspaceFib(sizex);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Plus alg;

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

  void testExec2D1DVertical()
  {
    int sizex = 10,sizey=20;
    // Register the workspace in the data service
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace123(1,sizey);
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    Plus alg;

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

    checkData(work_in1, work_in2, work_out1);

    AnalysisDataService::Instance().remove(wsName1);
    AnalysisDataService::Instance().remove(wsName2);
    AnalysisDataService::Instance().remove(wsNameOut);    
  }
  
  void testExec2D2DbyOperatorOverload()
  {
    int sizex = 10,sizey=20;
    MatrixWorkspace_sptr work_in1 = WorkspaceCreationHelper::Create2DWorkspace123(sizex,sizey);
    MatrixWorkspace_sptr work_in2 = WorkspaceCreationHelper::Create2DWorkspace154(sizex,sizey);

    MatrixWorkspace_sptr work_out1 = work_in2 + work_in1;

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

    Plus alg;

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

    Plus alg;

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
    a += 5;
    TS_ASSERT_EQUALS(a->readY(0)[0],8);
    TS_ASSERT_EQUALS(a,b);
    a += c;
    TS_ASSERT_EQUALS(a->readY(0)[0],10);
    TS_ASSERT_EQUALS(a,b);
  }

  void testRunAddition()
  {
    MatrixWorkspace_sptr a = WorkspaceCreationHelper::CreateWorkspaceSingleValue(3);
    a->mutableRun().setProtonCharge(10.);
    MatrixWorkspace_sptr b = WorkspaceCreationHelper::CreateWorkspaceSingleValue(2);
    b->mutableRun().setProtonCharge(5.);

    AnalysisDataService::Instance().add("a", a);
    AnalysisDataService::Instance().add("b", b);

    Plus alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(
      alg.setPropertyValue("LHSWorkspace","a");
      alg.setPropertyValue("RHSWorkspace","b");    
      alg.setPropertyValue("OutputWorkspace","c");
    )
    alg.execute();

    MatrixWorkspace_sptr work_out1;
    TS_ASSERT_THROWS_NOTHING(work_out1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("c")));

    TS_ASSERT_DELTA(work_out1->run().getProtonCharge(), 15.0, 1e-8);
   
    AnalysisDataService::Instance().remove("a");
    AnalysisDataService::Instance().remove("b");
    AnalysisDataService::Instance().remove("c");
  }
  


  void EventSetup()
  {
    AnalysisDataService::Instance().addOrReplace("ev1", boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateEventWorkspace(3,10,100, 0.0, 1.0, 3))); // 100 ev
    AnalysisDataService::Instance().addOrReplace("ev2", boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateEventWorkspace(3,10,100, 0.0, 1.0, 2))); //200 ev
    AnalysisDataService::Instance().addOrReplace("ev3", boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateEventWorkspace(3,10,100, 0.0, 1.0, 2, 100))); //200 events per spectrum, but the spectra are at different pixel ids
    //Make one with weird units
    MatrixWorkspace_sptr ev4 = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateEventWorkspace(3,10,100, 0.0, 1.0, 2, 100));
    ev4->setYUnit("Microfurlongs per Megafortnights");
    AnalysisDataService::Instance().addOrReplace("ev4_weird_units",ev4);
    //Different # of spectra
    AnalysisDataService::Instance().addOrReplace("ev5", boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateEventWorkspace(5,10,100, 0.0, 1.0, 2, 100))); //200 events per spectrum, but the spectra are at different pixel ids
    //a 2d workspace with the value 2 in each bin
    AnalysisDataService::Instance().addOrReplace("in2D", WorkspaceCreationHelper::Create2DWorkspaceBinned(3, 10, 0.0, 1.0));

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
  }

  //------------------------------------------------------------------------------------------------
  void testEventWorkspaces_IncompatibleUnits_Fail()
  {
    EventSetup();
    Plus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","ev1");
    alg.setPropertyValue("RHSWorkspace","ev4_weird_units");
    alg.setPropertyValue("OutputWorkspace", "evOUT");
    alg.execute();
    TS_ASSERT( !alg.isExecuted() );
    EventTeardown();
  }

  //------------------------------------------------------------------------------------------------
  void testEventWorkspaces_addingInPlace()
  {
    EventSetup();

    std::string in1_name("ev1");
    std::string in2_name("ev2");
    std::string out_name("ev1");

    EventWorkspace_sptr in1, in2, out;
    TS_ASSERT_THROWS_NOTHING(in1 = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(in1_name)));
    TS_ASSERT_THROWS_NOTHING(in2 = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(in2_name)));
    int numEvents1 = in1->getNumberEvents();
    int numEvents2 = in2->getNumberEvents();

    //Tests that the workspace is okay at first
    TS_ASSERT_EQUALS( in1->blocksize(), 10);
    for (int wi=0; wi < 3; wi++)
      for (int i=0; i<in1->blocksize(); i++)
        TS_ASSERT_EQUALS( in1->readY(wi)[i], 1);

    Plus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",in1_name);
    alg.setPropertyValue("RHSWorkspace",in2_name);
    alg.setPropertyValue("OutputWorkspace",out_name);
    alg.execute();

    out = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(out_name));
    int numEventsOut = out->getNumberEvents();

    //Correct in output
    TS_ASSERT_EQUALS( out->getNumberEvents(), numEvents1+numEvents2);
    //10 bins copied
    TS_ASSERT_EQUALS( out->blocksize(), 10);
    for (int wi=0; wi < 3; wi++)
      for (int i=0; i<out->blocksize(); i++)
        TS_ASSERT_EQUALS( out->readY(wi)[i], 3);

    //But they were added in #1
    TS_ASSERT_EQUALS( in1->getNumberEvents(), numEvents1+numEvents2);
    TS_ASSERT_EQUALS( in1, out);
    TS_ASSERT_DIFFERS( in2, out);

    EventTeardown();
  }

  //------------------------------------------------------------------------------------------------
  void testEventWorkspaces_differentOutputWorkspace()
  {
    EventSetup();

    std::string in1_name("ev1");
    std::string in2_name("ev2");
    std::string out_name("evOUT");

    EventWorkspace_sptr in1, in2, out;
    TS_ASSERT_THROWS_NOTHING(in1 = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(in1_name)));
    TS_ASSERT_THROWS_NOTHING(in2 = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(in2_name)));
    int numEvents1 = in1->getNumberEvents();
    int numEvents2 = in2->getNumberEvents();

    Plus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",in1_name);
    alg.setPropertyValue("RHSWorkspace",in2_name);
    alg.setPropertyValue("OutputWorkspace",out_name);
    alg.execute();

    out = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(out_name));
    //Ya, its an event workspace
    TS_ASSERT(out);
    int numEventsOut = out->getNumberEvents();

    //Correct in output
    TS_ASSERT_EQUALS( out->getNumberEvents(), numEvents1+numEvents2);
    //10 bins copied
    TS_ASSERT_EQUALS( out->blocksize(), 10);

    for (int wi=0; wi < 3; wi++)
      for (int i=0; i<out->blocksize(); i++)
        TS_ASSERT_EQUALS( out->readY(wi)[i], 3);

    //But they were added in #1
    TS_ASSERT_DIFFERS( in1, out);
    TS_ASSERT_DIFFERS( in2, out);

    EventTeardown();
  }



  //------------------------------------------------------------------------------------------------
  void testEventWorkspaces_differentOutputWorkspace_with_a_singlebin()
  {
    EventSetup();

    std::string in1_name("ev1");
    std::string in2_name("ev2");
    std::string out_name("evOUT");

    EventWorkspace_sptr in1, in2, out;
    TS_ASSERT_THROWS_NOTHING(in1 = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(in1_name)));
    TS_ASSERT_THROWS_NOTHING(in2 = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(in2_name)));
    int numEvents1 = in1->getNumberEvents();
    int numEvents2 = in2->getNumberEvents();

    //------- set to a single bin ------------
    MantidVecPtr x1;
    MantidVec& xRef = x1.access();
    xRef.push_back(0);
    xRef.push_back(1e5); //One large bin
    in1->setAllX(x1);
    in2->setAllX(x1);

    Plus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",in1_name);
    alg.setPropertyValue("RHSWorkspace",in2_name);
    alg.setPropertyValue("OutputWorkspace",out_name);
    alg.execute();

    out = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(out_name));
    //Ya, its an event workspace
    TS_ASSERT(out);
    int numEventsOut = out->getNumberEvents();

    //Correct in output
    TS_ASSERT_EQUALS( out->getNumberEvents(), numEvents1+numEvents2);
    //1 bin copied
    TS_ASSERT_EQUALS( out->blocksize(), 1);

    for (int wi=0; wi < 3; wi++)
      TS_ASSERT_EQUALS( out->readY(wi)[0], (numEvents1+numEvents2)/3);

    //But they were added in #1
    TS_ASSERT_DIFFERS( in1, out);
    TS_ASSERT_DIFFERS( in2, out);

    EventTeardown();
  }

  //------------------------------------------------------------------------------------------------
  void testEventWorkspaces_differentOutputAndDifferentPixelIDs()
  {
    EventSetup();

    std::string in1_name("ev1");
    std::string in2_name("ev3");
    std::string out_name("evOUT");

    EventWorkspace_sptr in1, in2, out;
    TS_ASSERT_THROWS_NOTHING(in1 = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(in1_name)));
    TS_ASSERT_THROWS_NOTHING(in2 = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(in2_name)));
    int numEvents1 = in1->getNumberEvents();
    int numEvents2 = in2->getNumberEvents();

    IndexToIndexMap *rhs_map = in2->getWorkspaceIndexToDetectorIDMap();
    //First pixel id of rhs is 100
    TS_ASSERT_EQUALS( (*rhs_map)[0], 100);

    Plus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace",in1_name);
    alg.setPropertyValue("RHSWorkspace",in2_name);
    alg.setPropertyValue("OutputWorkspace",out_name);
    alg.execute();

    out = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(out_name));
    //Ya, its an event workspace
    TS_ASSERT(out);
    int numEventsOut = out->getNumberEvents();

    //Correct in output
    TS_ASSERT_EQUALS( out->getNumberEvents(), numEvents1+numEvents2);
    //Still the same # of histograms
    TS_ASSERT_EQUALS( out->getNumberHistograms(), 3);
    //10 bins copied
    TS_ASSERT_EQUALS( out->blocksize(), 10);

    //1 event per pixel for the first 3 histograms (pixels 0-2)
    for (int wi=0; wi < 3; wi++)
      for (int i=0; i < out->blocksize(); i++)
        TS_ASSERT_EQUALS( out->readY(wi)[i], 3);

    //But two detector IDs in each one
    for (int i=0; i<3; i++)
    {
      std::vector<int> detList = out->spectraMap().getDetectors(i);
      TS_ASSERT_EQUALS( detList[0], 0+i );
      TS_ASSERT_EQUALS( detList[1], 100+i );
    }

    //But they were added in #1
    TS_ASSERT_DIFFERS( in1, out);
    TS_ASSERT_DIFFERS( in2, out);

    EventTeardown();
  }

  //------------------------------------------------------------------------------------------------
  void testEventWorkspaces_addingInPlace_But_DifferentPixelIDs()
  {
    EventSetup();
    Plus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","ev1");
    alg.setPropertyValue("RHSWorkspace","ev3");
    alg.setPropertyValue("OutputWorkspace","ev1");
    alg.execute();
    //Succeeds despite detector id mismatch
    TS_ASSERT( alg.isExecuted() );

    EventTeardown();
  }


  //------------------------------------------------------------------------------------------------
  void testEventWorkspaces_Event_Plus_2D_differentOutput()
  {
    EventSetup();
    Plus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","ev1");
    alg.setPropertyValue("RHSWorkspace","in2D");
    alg.setPropertyValue("OutputWorkspace","out2D");
    alg.execute();

    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_sptr out = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("out2D"));
    //It's not an EventWorkspace
    TS_ASSERT( ! (boost::dynamic_pointer_cast<EventWorkspace>(out)) );

    //Should be 3 counts per bin
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 3);
    for (int wi=0; wi < 3; wi++)
      for (int i=0; i < out->blocksize(); i++)
        TS_ASSERT_EQUALS( out->readY(wi)[i], 3);

    EventTeardown();
  }


  //------------------------------------------------------------------------------------------------
  void testEventWorkspaces_Event_Plus_2D_addingInPlace_to_2D_Succeeds()
  {
    //This test result may change with ProxyWorkspace

    EventSetup();
    Plus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","ev1");
    alg.setPropertyValue("RHSWorkspace","in2D");
    alg.setPropertyValue("OutputWorkspace","in2D");
    alg.execute();

    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_sptr out = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("in2D"));
    //It's not an EventWorkspace
    TS_ASSERT( ! (boost::dynamic_pointer_cast<EventWorkspace>(out)) );

    //Should be 3 counts per bin
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 3);
    for (int wi=0; wi < 3; wi++)
      for (int i=0; i < out->blocksize(); i++)
        TS_ASSERT_EQUALS( out->readY(wi)[i], 3);

    EventTeardown();
  }

  //------------------------------------------------------------------------------------------------
  void testEventWorkspaces_Event_Plus_2D_addingInPlace_to_Event_Fails()
  {
    //Can't add into an EventWorkspace
    //This test result may change with ProxyWorkspace

    EventSetup();
    Plus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","ev1");
    alg.setPropertyValue("RHSWorkspace","in2D");
    alg.setPropertyValue("OutputWorkspace","ev1");
    alg.execute();

    TS_ASSERT( ! alg.isExecuted() );

    EventTeardown();
  }

  //------------------------------------------------------------------------------------------------
  void testEventWorkspaces_Event_DifferentSizesFail()
  {
    EventSetup();
    Plus alg;
    alg.initialize();
    alg.setPropertyValue("LHSWorkspace","ev1");
    alg.setPropertyValue("RHSWorkspace","ev5");
    alg.setPropertyValue("OutputWorkspace","evOUT");
    alg.execute();
    TS_ASSERT( ! alg.isExecuted() );
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
      double sig2 = work_in2->dataY(ws2Index/work_in1->blocksize())[ws2Index%work_in2->blocksize()];
      double sig3 = work_out1->dataY(i/work_in1->blocksize())[i%work_in1->blocksize()];
      TS_ASSERT_DELTA(work_in1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()],
        work_out1->dataX(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
      
      TS_ASSERT_DELTA(sig1 + sig2, sig3, 0.0001);
      double err1 = work_in1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()];
      double err2 = work_in2->dataE(ws2Index/work_in2->blocksize())[ws2Index%work_in2->blocksize()];
      double err3(sqrt((err1*err1) + (err2*err2)));     
      TS_ASSERT_DELTA(err3, work_out1->dataE(i/work_in1->blocksize())[i%work_in1->blocksize()], 0.0001);
  }
  
};

#endif /*PLUSTEST_H_*/
