#ifndef MANTID_MD_CONVERT2_Q_NDANY_TEST_H_
#define MANTID_MD_CONVERT2_Q_NDANY_TEST_H_

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidAPI/TextAxis.h"
#include "MantidMDAlgorithms/ConvertToMD.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidMDEvents/ConvToMDSelector.h"
#include "MantidMDAlgorithms/PreprocessDetectorsToMD.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDEvents;

class Convert2AnyTestHelper: public ConvertToMD
{
public:
    Convert2AnyTestHelper(){};
    TableWorkspace_const_sptr preprocessDetectorsPositions( Mantid::API::MatrixWorkspace_const_sptr InWS2D,const std::string dEModeRequested="Direct",bool updateMasks=false)
    {
      return ConvertToMD::preprocessDetectorsPositions(InWS2D,dEModeRequested,updateMasks,std::string(this->getProperty("PreprocDetectorsWS")));
    }
    void setSourceWS(Mantid::API::MatrixWorkspace_sptr InWS2D)
    {
      m_InWS2D = InWS2D;
    }

};
// helper function to provide list of names to test:
std::vector<std::string> dim_availible()
{
    std::string dns_ws[]={"DeltaE","T","alpha","beta","gamma"};
    std::vector<std::string> data_names_in_WS;
    for(size_t i=0;i<5;i++){
        data_names_in_WS.push_back(dns_ws[i]);
    }
    return data_names_in_WS;
}
//
class ConvertToMDTest : public CxxTest::TestSuite
{
 std::auto_ptr<Convert2AnyTestHelper> pAlg;
public:
static ConvertToMDTest *createSuite() { return new ConvertToMDTest(); }
static void destroySuite(ConvertToMDTest * suite) { delete suite; }    

typedef std::vector<std::string> PropertyAllowedValues;

void testInit(){

    TS_ASSERT_THROWS_NOTHING( pAlg->initialize() )
    TS_ASSERT( pAlg->isInitialized() )

    TSM_ASSERT_EQUALS("algorithm should have 21 properties",21,(size_t)(pAlg->getProperties().size()));
}


void testSetUpThrow()
{
    //TODO: check if wrong WS throws (should on validator)

     // get ws from the DS    
     Mantid::API::MatrixWorkspace_sptr ws2D = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
     // give it to algorithm
    TSM_ASSERT_THROWS_NOTHING("the initial ws is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    // target ws fine
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransferND"));
    // unknown Q-dimension trows
    TS_ASSERT_THROWS(pAlg->setPropertyValue("QDimensions", "unknownQ"),std::invalid_argument);
    // correct Q-dimension fine
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "|Q|"));
    // additional dimensions requested -- fine
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OtherDimensions", "DeltaE,omega"));

}

void testExecNoQ()
{

     Mantid::API::MatrixWorkspace_sptr ws2D = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
     API::NumericAxis *pAxis = new API::NumericAxis(3);
     pAxis->setUnit("dSpacing");

     ws2D->replaceAxis(0,pAxis);

    pAlg->setPropertyValue("InputWorkspace","testWSProcessed");
    pAlg->setPropertyValue("OutputWorkspace","WS3DNoQ");
    pAlg->setPropertyValue("PreprocDetectorsWS","");
    pAlg->setPropertyValue("QDimensions","CopyToMD");
    pAlg->setPropertyValue("OtherDimensions","phi,chi");
//    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "NoDE"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode","Elastic")); // dE mode will be ignored
    //
    pAlg->setPropertyValue("MinValues","-10,0,-10");
    pAlg->setPropertyValue("MaxValues"," 10,20,40");
    pAlg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    checkHistogramsHaveBeenStored("WS3DNoQ");

    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>("WS3DNoQ");
    TS_ASSERT_EQUALS(Mantid::API::None, outWS->getSpecialCoordinateSystem());

    AnalysisDataService::Instance().remove("WS3DNoQ");
}

void testExecModQ()
{

     Mantid::API::MatrixWorkspace_sptr ws2D = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
     API::NumericAxis *pAxis = new API::NumericAxis(3);
     pAxis->setUnit("dSpacing");

     ws2D->replaceAxis(0,pAxis);

    pAlg->setPropertyValue("OutputWorkspace","WS3DmodQ");
    pAlg->setPropertyValue("InputWorkspace","testWSProcessed");
    pAlg->setPropertyValue("QDimensions","|Q|");
    pAlg->setPropertyValue("PreprocDetectorsWS","");
    pAlg->setPropertyValue("OtherDimensions","phi,chi");
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Elastic"));
    //
    pAlg->setPropertyValue("MinValues","-10,0,-10");
    pAlg->setPropertyValue("MaxValues"," 10,20,40");
    pAlg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(pAlg->execute());
    checkHistogramsHaveBeenStored("WS3DmodQ",7000,6489.5591101441796,7300.7539989122024);

    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>("WS3DmodQ");
    TS_ASSERT_EQUALS(Mantid::API::None, outWS->getSpecialCoordinateSystem());

    AnalysisDataService::Instance().remove("WS3DmodQ");
}

void testExecQ3D()
{
     Mantid::API::MatrixWorkspace_sptr ws2D = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("testWSProcessed");
     API::NumericAxis *pAxis = new API::NumericAxis(3);
     pAxis->setUnit("DeltaE");

     ws2D->replaceAxis(0,pAxis);

    pAlg->setPropertyValue("OutputWorkspace","WS5DQ3D");
    pAlg->setPropertyValue("InputWorkspace","testWSProcessed");
    pAlg->setPropertyValue("OtherDimensions","phi,chi");
    pAlg->setPropertyValue("PreprocDetectorsWS","");
     
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "Q3D"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Direct"));
    pAlg->setPropertyValue("MinValues","-10,-10,-10,  0,-10,-10");
    pAlg->setPropertyValue("MaxValues"," 10, 10, 10, 20, 40, 20");

    pAlg->setRethrows(false);
    pAlg->execute();
    TSM_ASSERT("Should finish successfully",pAlg->isExecuted());
    checkHistogramsHaveBeenStored("WS5DQ3D");

    auto outWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>("WS5DQ3D");
    TS_ASSERT_EQUALS(Mantid::API::HKL, outWS->getSpecialCoordinateSystem());

    AnalysisDataService::Instance().remove("WS5DQ3D");
}

//DO NOT DISABLE THIS TEST
void testAlgorithmProperties()
{
  /*
  The Create MD Workspace GUI runs this algorithm internally.
  If property names and property allowed values here change, that interface will break.

  This unit test is designed to flag up changes here. If property values and names here do need to be changed, 
  1) They must also be updated in CreateMDWorkspaceAlgDialog.cpp. 
  2) It should then be confirmed that that the Create MD Workspace custom interface still works!
  3) Finally this unit test should be updated so that the tests pass.
  */

  ConvertToMD alg;
  alg.initialize();

  Mantid::Kernel::Property *QDimProperty;
  TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", QDimProperty = alg.getProperty("QDimensions"));
  TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", QDimProperty = alg.getProperty("dEAnalysisMode"));
  TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", QDimProperty = alg.getProperty("OtherDimensions"));
  TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", QDimProperty = alg.getProperty("MinValues"));
  TSM_ASSERT_THROWS_NOTHING("Property name has changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", QDimProperty = alg.getProperty("MaxValues"));

  QDimProperty =alg.getProperty("QDimensions");
  PropertyAllowedValues QDimValues = QDimProperty->allowedValues();
  TSM_ASSERT_EQUALS("QDimensions property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", 3, QDimValues.size());
  TSM_ASSERT("QDimensions property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!",  findValue( QDimValues,"CopyToMD") );
  TSM_ASSERT("QDimensions property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", findValue( QDimValues, "|Q|") );
  TSM_ASSERT("QDimensions property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", findValue( QDimValues, "Q3D") );

  Mantid::Kernel::Property *dEAnalysisMode =alg.getProperty("dEAnalysisMode");
  PropertyAllowedValues dEAnalysisModeValues = dEAnalysisMode->allowedValues();
  TSM_ASSERT_EQUALS("QDimensions property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", 3, dEAnalysisModeValues.size());
//  TSM_ASSERT("dEAnalysisMode property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!",  findValue( dEAnalysisModeValues, "NoDE") );
  TSM_ASSERT("dEAnalysisMode property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", findValue( dEAnalysisModeValues, "Direct") );
  TSM_ASSERT("dEAnalysisMode property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", findValue( dEAnalysisModeValues, "Indirect") );
  TSM_ASSERT("dEAnalysisMode property values have changed. This has broken Create MD Workspace GUI. Fix CreateMDWorkspaceGUI!", findValue( dEAnalysisModeValues, "Elastic") );
}


ConvertToMDTest(){
     pAlg = std::auto_ptr<Convert2AnyTestHelper>(new Convert2AnyTestHelper());
     Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);
    // rotate the crystal by twenty degrees back;
     ws2D->mutableRun().mutableGoniometer().setRotationAngle(0,20);
     // add workspace energy
     ws2D->mutableRun().addProperty("Ei",13.,"meV",true);

     AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);
}
~ConvertToMDTest()
{
  AnalysisDataService::Instance().remove("testWSProcessed");
}
private:

  void checkHistogramsHaveBeenStored(const std::string & wsName, double val = 0.34, double bin_min=0.3, double bin_max=0.4)
  {
    IMDEventWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(wsName);
    uint16_t nexpts = outputWS->getNumExperimentInfo();
    for(uint16_t i = 0; i < nexpts; ++i)
    {
      ExperimentInfo_const_sptr expt = outputWS->getExperimentInfo(i);
      std::pair<double,double> bin = expt->run().histogramBinBoundaries(val);
      TS_ASSERT_DELTA(bin.first, bin_min, 1e-8);
      TS_ASSERT_DELTA(bin.second, bin_max, 1e-8);
    }
  }

  bool findValue(const PropertyAllowedValues& container, const std::string& value)
  {
    return std::find( container.begin(), container.end(), value) != container.end();
  }
};

//-------------------------------------------------------------------------------------------------
// Performance Test
//-------------------------------------------------------------------------------------------------

class ConvertToMDTestPerformance : public CxxTest::TestSuite
{
    //Kernel::CPUTimer Clock;
    time_t start,end;

    size_t numHist;
    Kernel::Matrix<double> Rot;   

   Mantid::API::MatrixWorkspace_sptr inWs2D;
   Mantid::API::MatrixWorkspace_sptr inWsEv;

   WorkspaceCreationHelper::MockAlgorithm reporter;

   boost::shared_ptr<ConvToMDBase>  pConvMethods;
   DataObjects::TableWorkspace_sptr pDetLoc_events;
   DataObjects::TableWorkspace_sptr pDetLoc_histo;
   // pointer to mock algorithm to work with progress bar
   std::auto_ptr<WorkspaceCreationHelper::MockAlgorithm> pMockAlgorithm;

    boost::shared_ptr<MDEvents::MDEventWSWrapper> pTargWS;

public:
static ConvertToMDTestPerformance *createSuite() { return new ConvertToMDTestPerformance(); }
static void destroySuite(ConvertToMDTestPerformance * suite) { delete suite; }    



void test_EventNoUnitsConv()
{
   

    NumericAxis *pAxis0 = new NumericAxis(2); 
    pAxis0->setUnit("DeltaE");
    inWsEv->replaceAxis(0,pAxis0);

    MDWSDescription WSD;
    std::vector<double> min(4,-1e+30),max(4,1e+30);
    WSD.setMinMax(min,max);

    WSD.buildFromMatrixWS(inWsEv,"Q3D","Indirect");

    WSD.m_PreprDetTable =pDetLoc_events;
    WSD.m_RotMatrix = Rot;
    // this one comes from ticket #6852 and would not exist in clear branch.
    WSD.addProperty("RUN_INDEX",static_cast<uint16_t>(10),true);

    // create new target MD workspace
    pTargWS->releaseWorkspace();   
    pTargWS->createEmptyMDWS(WSD);

    ConvToMDSelector AlgoSelector;
    pConvMethods = AlgoSelector.convSelector(inWsEv,pConvMethods);
    TS_ASSERT_THROWS_NOTHING(pConvMethods->initialize(WSD,pTargWS,false));

    pMockAlgorithm->resetProgress(numHist);
    //Clock.elapsedCPU();
    std::time (&start);
    TS_ASSERT_THROWS_NOTHING(pConvMethods->runConversion(pMockAlgorithm->getProgress()));
    std::time (&end);
    double sec = std::difftime (end,start);
    TS_WARN("Time to complete: <EventWSType,Q3D,Indir,ConvertNo,CrystType>: "+boost::lexical_cast<std::string>(sec)+" sec");
}

void test_EventFromTOFConv()
{
   

    NumericAxis *pAxis0 = new NumericAxis(2); 
    pAxis0->setUnit("TOF");
    inWsEv->replaceAxis(0,pAxis0);
  
    MDWSDescription WSD;
    std::vector<double> min(4,-1e+30),max(4,1e+30);
    WSD.setMinMax(min,max);
    WSD.buildFromMatrixWS(inWsEv,"Q3D","Indirect");

    WSD.m_PreprDetTable =pDetLoc_events;
    WSD.m_RotMatrix = Rot;
    // this one comes from ticket #6852 and would not exist in clear branch.
    WSD.addProperty("RUN_INDEX",static_cast<uint16_t>(10),true);

    // create new target MD workspace
    pTargWS->releaseWorkspace();   
    pTargWS->createEmptyMDWS(WSD);


    ConvToMDSelector AlgoSelector;
    pConvMethods = AlgoSelector.convSelector(inWsEv,pConvMethods);
    pConvMethods->initialize(WSD,pTargWS,false);

    pMockAlgorithm->resetProgress(numHist);
    //Clock.elapsedCPU();
    std::time (&start);
    TS_ASSERT_THROWS_NOTHING(pConvMethods->runConversion(pMockAlgorithm->getProgress()));
    std::time (&end);
    double sec = std::difftime (end,start);
    //float sec = Clock.elapsedCPU();
    TS_WARN("Time to complete: <EventWSType,Q3D,Indir,ConvFromTOF,CrystType>: "+boost::lexical_cast<std::string>(sec)+" sec");
}
void test_HistoFromTOFConv()
{

    NumericAxis *pAxis0 = new NumericAxis(2); 
    pAxis0->setUnit("TOF");
    inWs2D->replaceAxis(0,pAxis0);

    MDWSDescription WSD;
    std::vector<double> min(4,-1e+30),max(4,1e+30);
    WSD.setMinMax(min,max);

    WSD.buildFromMatrixWS(inWs2D,"Q3D","Indirect");

    WSD.m_PreprDetTable =pDetLoc_histo;
    WSD.m_RotMatrix = Rot;
    // this one comes from ticket #6852 and would not exist in clear branch.
    WSD.addProperty("RUN_INDEX",static_cast<uint16_t>(10),true);

    // create new target MD workspace
    pTargWS->releaseWorkspace();   
    pTargWS->createEmptyMDWS(WSD);


    pTargWS->createEmptyMDWS(WSD);

    ConvToMDSelector AlgoSelector;
    pConvMethods = AlgoSelector.convSelector(inWs2D,pConvMethods);
    pConvMethods->initialize(WSD,pTargWS,false);

    pMockAlgorithm->resetProgress(numHist);
    //Clock.elapsedCPU();
    std::time (&start);
    TS_ASSERT_THROWS_NOTHING(pConvMethods->runConversion(pMockAlgorithm->getProgress()));
    std::time (&end);
    double sec = std::difftime (end,start);

    TS_WARN("Time to complete: <Ws2DHistoType,Q3D,Indir,ConvFromTOF,CrystType>: "+boost::lexical_cast<std::string>(sec)+" sec");
}

void test_HistoNoUnitsConv()
{

  

    NumericAxis *pAxis0 = new NumericAxis(2); 
    pAxis0->setUnit("DeltaE");
    inWs2D->replaceAxis(0,pAxis0);

    MDWSDescription WSD;
    std::vector<double> min(4,-1e+30),max(4,1e+30);
    WSD.setMinMax(min,max);

    WSD.buildFromMatrixWS(inWs2D,"Q3D","Indirect");

    WSD.m_PreprDetTable =pDetLoc_histo;
    WSD.m_RotMatrix = Rot;
    // this one comes from ticket #6852 and would not exist in clear branch.
    WSD.addProperty("RUN_INDEX",static_cast<uint16_t>(10),true);

    // create new target MD workspace
    pTargWS->releaseWorkspace();   
    pTargWS->createEmptyMDWS(WSD);


    pTargWS->createEmptyMDWS(WSD);

    ConvToMDSelector AlgoSelector;
    pConvMethods = AlgoSelector.convSelector(inWs2D,pConvMethods);
    pConvMethods->initialize(WSD,pTargWS,false);

    pMockAlgorithm->resetProgress(numHist);
    //Clock.elapsedCPU();
    std::time (&start);
    TS_ASSERT_THROWS_NOTHING(pConvMethods->runConversion(pMockAlgorithm->getProgress()));
    std::time (&end);
    double sec = std::difftime (end,start);

    TS_WARN("Time to complete: <Ws2DHistoType,Q3D,Indir,ConvertNo,CrystType>: "+boost::lexical_cast<std::string>(sec)+" sec");
}


ConvertToMDTestPerformance():
Rot(3,3)
{
   numHist=100*100;
   size_t nEvents=1000;
   inWsEv = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateRandomEventWorkspace(nEvents, numHist, 0.1));
   inWsEv->setInstrument( ComponentCreationHelper::createTestInstrumentCylindrical(int(numHist)) );
   inWsEv->mutableRun().addProperty("Ei",12.,"meV",true);
   API::AnalysisDataService::Instance().addOrReplace("TestEventWS",inWsEv);


   inWs2D = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(int(numHist), int(nEvents)));
   // add workspace energy
   inWs2D->mutableRun().addProperty("Ei",12.,"meV",true);
   API::AnalysisDataService::Instance().addOrReplace("TestMatrixWS",inWs2D);

    auto pAlg = std::auto_ptr<PreprocessDetectorsToMD>(new PreprocessDetectorsToMD());
    pAlg->initialize();

    pAlg->setPropertyValue("InputWorkspace","TestMatrixWS");
    pAlg->setPropertyValue("OutputWorkspace","PreprocessedDetectorsTable");

    pAlg->execute();
    if(!pAlg->isExecuted())throw(std::runtime_error("Can not preprocess histogram detectors to MD"));
 
    API::Workspace_sptr tWs =  API::AnalysisDataService::Instance().retrieve("PreprocessedDetectorsTable");
    pDetLoc_histo = boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(tWs);
    if(!pDetLoc_histo)throw(std::runtime_error("Can not obtain preprocessed histogram detectors "));

    pAlg->setPropertyValue("InputWorkspace","TestEventWS");
    pAlg->execute();
    if(!pAlg->isExecuted())throw(std::runtime_error("Can not preprocess events detectors to MD"));

    tWs =  API::AnalysisDataService::Instance().retrieve("PreprocessedDetectorsTable");
    pDetLoc_events = boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(tWs);
    if(!pDetLoc_events)throw(std::runtime_error("Can not obtain preprocessed events detectors "));

    pTargWS = boost::shared_ptr<MDEventWSWrapper>(new MDEventWSWrapper());

    Rot.setRandom(100);
    Rot.toRotation();

    // this will be used to display progress
    pMockAlgorithm = std::auto_ptr<WorkspaceCreationHelper::MockAlgorithm>(new WorkspaceCreationHelper::MockAlgorithm());
}

};

#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

