#ifndef MANTID_ALGORITHMS_CREATESAMPLEWORKSPACETEST_H_
#define MANTID_ALGORITHMS_CREATESAMPLEWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"

using Mantid::Algorithms::CreateSampleWorkspace;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects; 

class CreateSampleWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateSampleWorkspaceTest *createSuite() { return new CreateSampleWorkspaceTest(); }
  static void destroySuite( CreateSampleWorkspaceTest *suite ) { delete suite; }

  CreateSampleWorkspaceTest() {
    FrameworkManager::Instance();
  }

  void test_Init()
  {
    CreateSampleWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  MatrixWorkspace_sptr createSampleWorkspace(std::string outWSName, std::string wsType = "", std::string function = "",
     std::string userFunction = "", int numBanks = 2, int bankPixelWidth = 10, int numEvents = 1000, bool isRandom = false, 
     std::string xUnit = "TOF", double xMin = 0.0, double xMax = 20000.0, double binWidth = 200.0)
  {
  
    CreateSampleWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );
    alg.setPropertyValue("OutputWorkspace", outWSName);
    if (!wsType.empty()) TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("WorkspaceType", wsType) );
    if (!function.empty()) TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Function", function) );
    if (!userFunction.empty()) TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("UserDefinedFunction", userFunction) );
    if (numBanks != 2) TS_ASSERT_THROWS_NOTHING( alg.setProperty("NumBanks", numBanks) );
    if (bankPixelWidth != 10) TS_ASSERT_THROWS_NOTHING( alg.setProperty("BankPixelWidth", bankPixelWidth) );
    if (numEvents != 1000) TS_ASSERT_THROWS_NOTHING( alg.setProperty("NumEvents", numEvents) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("Random", isRandom) );
    if (xUnit!="TOF") TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("XUnit", xUnit) );
    if (xMin != 0.0) TS_ASSERT_THROWS_NOTHING( alg.setProperty("XMin", xMin) );
    if (xMax != 20000.0) TS_ASSERT_THROWS_NOTHING( alg.setProperty("XMax", xMax) );
    if (binWidth != 200.0) TS_ASSERT_THROWS_NOTHING( alg.setProperty("BinWidth", binWidth) );
    
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws);
    
    //check the basics
    int numBins = static_cast<int>((xMax-xMin)/binWidth);
    int numHist = numBanks * bankPixelWidth * bankPixelWidth;
    TS_ASSERT_EQUALS(ws->getNumberHistograms(),numHist);
    TS_ASSERT_EQUALS(ws->blocksize(),numBins);
    
    TS_ASSERT_EQUALS(ws->getAxis(0)->unit()->unitID(), xUnit);
    TS_ASSERT_EQUALS(ws->readX(0)[0], xMin);
    if (ws->blocksize()==static_cast<size_t>(numBins))
    {
      TS_ASSERT_DELTA(ws->readX(0)[numBins], xMax,binWidth);
    }

    if (wsType == "Event")
    {
      EventWorkspace_sptr ews = boost::dynamic_pointer_cast<EventWorkspace>(ws);
      TS_ASSERT(ews);
    }

    return ws;
  }
  
  void test_histogram_defaults()
  {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_OutputWS");
  
    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws = createSampleWorkspace(outWSName);
    if (!ws) return;
    TS_ASSERT_DELTA(ws->readY(0)[20], 0.3,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[40], 0.3,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[50], 10.3,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[60], 0.3,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[80], 0.3,0.0001);
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  void test_event_defaults()
  {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_OutputWS_event");
  
    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws = createSampleWorkspace(outWSName, "Event");
    if (!ws) return;
    TS_ASSERT_DELTA(ws->readY(0)[20], 30,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[40], 30,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[50], 1030,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[60], 30,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[80], 30,0.0001);
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }  

  void test_event_MoreBanksMoreDetectorsLessEvents()
  {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_OutputWS_MoreBanksMoreDetectors");
  
    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws = createSampleWorkspace(outWSName, "Event","","",4,30,100);
    if (!ws) return;
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_histo_Multiple_Peaks()
  {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_OutputWS_Multiple_Peaks");
  
    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws = createSampleWorkspace(outWSName, "Histogram","Multiple Peaks");
    if (!ws) return;
    TS_ASSERT_DELTA(ws->readY(0)[20], 0.3,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[40], 0.3,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[60], 8.3,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[80], 0.3,0.0001);
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  } 

  void test_event_Flat_background()
  {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_OutputWS_Flat_background");
  
    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws = createSampleWorkspace(outWSName, "Event","Flat background");
    if (!ws) return;
    TS_ASSERT_DELTA(ws->readY(0)[20], 10.0,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[40], 10.0,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[60], 10.0,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[80], 10.0,0.0001);
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
    
  void test_event_Exp_Decay()
  {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_OutputWS_Exp_Decay");
  
    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws = createSampleWorkspace(outWSName, "Event","Exp Decay");
    if (!ws) return;
    TS_ASSERT_DELTA(ws->readY(0)[20], 3.0,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[40], 0.0,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[60], 0.0,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[80], 0.0,0.0001);
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }     

  void test_event_User_Defined()
  {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_OutputWS_User_Defined");
    std::string myFunc = "name=LinearBackground, A0=0.5;name=Gaussian, PeakCentre=10000, Height=50, Sigma=0.5;name=Gaussian, PeakCentre=1000, Height=80, Sigma=0.5";
    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws = createSampleWorkspace(outWSName, "Histogram","User Defined",myFunc);
    if (!ws) return;
    TS_ASSERT_DELTA(ws->readY(0)[5], 80.5,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[20], 0.5,0.0001);
    TS_ASSERT_DELTA(ws->readY(0)[50], 50.5,0.0001);
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_histogram_random()
  {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_Hist_random");
  
    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws = createSampleWorkspace(outWSName);
    if (!ws) return;
    TS_ASSERT_DELTA(ws->readY(0)[20], 0.3,0.5);
    TS_ASSERT_DELTA(ws->readY(0)[40], 0.3,0.5);
    TS_ASSERT_DELTA(ws->readY(0)[50], 10.3,0.5);
    TS_ASSERT_DELTA(ws->readY(0)[60], 0.3,0.5);
    TS_ASSERT_DELTA(ws->readY(0)[80], 0.3,0.5);
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  void test_event_random()
  {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_event_random");
  
    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws = createSampleWorkspace(outWSName,"Event");
    if (!ws) return;
    TS_ASSERT_DELTA(ws->readY(0)[20], 30,50);
    TS_ASSERT_DELTA(ws->readY(0)[40], 30,50);
    TS_ASSERT_DELTA(ws->readY(0)[50], 1030,50);
    TS_ASSERT_DELTA(ws->readY(0)[60], 30,50);
    TS_ASSERT_DELTA(ws->readY(0)[80], 3,50);
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_units()
  {
    // Name of the output workspace.
    std::string outWSName("CreateSampleWorkspaceTest_units");
  
    /* Equivalent of this python command:
      ws=CreateSampleWorkspace(WorkspaceType="Event",Function="One Peak",
      NumBanks=1,BankPixelWidth=2,NumEvents=50,Random=True,
      XUnit="dSpacing",XMin=0, XMax=8, BinWidth=0.1)
    */
    MatrixWorkspace_sptr ws = createSampleWorkspace(outWSName,"Event","One Peak","",1,2,50,true,"dSpacing",0,8,0.1);
    if (!ws) return;
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);

    ws = createSampleWorkspace(outWSName,"Event","One Peak","",1,2,50,true,"Wavelength",0,8,0.1);
    if (!ws) return;
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);

    ws = createSampleWorkspace(outWSName,"Event","One Peak","",1,2,50,true,"Energy",100,1000,10);
    if (!ws) return;
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);

    ws = createSampleWorkspace(outWSName,"Event","One Peak","",1,2,50,true,"QSquared",0,800,10);
    if (!ws) return;
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  void test_failure_due_to_bad_bin_width()
  {
    /* Equivalent of this python command:
      mono_ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10000,XUnit='DeltaE',XMin=-5,XMax=15)
    */
    std::string outWSName = "CreateSampleWorkspaceTest_test_failure_due_to_bad_bin_width";
    CreateSampleWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );
    alg.setPropertyValue("OutputWorkspace", outWSName);
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("NumBanks", 1) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("BankPixelWidth", 4) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("NumEvents", 10000) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("XUnit", "DeltaE") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("XMin", -5.0) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("XMax", 15.0) );
    //leave the default bin width of 200, which is inappropriate
    
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service. TODO: Change to your desired type
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws);
    //just one bin
    TS_ASSERT_EQUALS(ws->blocksize(),1);

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

};


#endif /* MANTID_ALGORITHMS_CREATESAMPLEWORKSPACETEST_H_ */
