#ifndef MANTIDMD_CONVERT2MD_TEST_PERFORM_H_
#define MANTIDMD_CONVERT2MD_TEST_PERFORM_H_

#include <ctime>
#include <iomanip>
#include <iostream>

// this preprocessor definition disables "ADD TO MD WORKSPACE" operations and should be enabled if this is necessary. (for debug purposes only!)

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/CPUTimer.h"

#include "MantidAPI/TextAxis.h"
#include "MantidMDAlgorithms/ConvertToMD.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"

#include "MantidMDEvents/ConvToMDSelector.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDEvents;
using namespace Mantid::MDAlgorithms;


class ConvertToMDTestPerformance : public CxxTest::TestSuite
{
    Kernel::CPUTimer Clock;
    time_t start,end;

    size_t numHist;
    Kernel::Matrix<double> Rot;   

   Mantid::API::MatrixWorkspace_sptr inWs2D;
   Mantid::API::MatrixWorkspace_sptr inWsEv;

   WorkspaceCreationHelper::MockAlgorithm reporter;

   boost::shared_ptr<ConvToMDBase> pConvMethods;
   ConvToMDPreprocDet DetLoc_events;
   ConvToMDPreprocDet DetLoc_histo;
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

    WSD.setDetectors(DetLoc_events);
    WSD.m_RotMatrix = Rot;

    // create new target MD workspace
    pTargWS->releaseWorkspace();   
    pTargWS->createEmptyMDWS(WSD);

    ConvToMDSelector AlgoSelector;
    pConvMethods = AlgoSelector.convSelector(inWsEv);
    TS_ASSERT_THROWS_NOTHING(pConvMethods->initialize(WSD,pTargWS));

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

    WSD.setDetectors(DetLoc_events);
    WSD.m_RotMatrix = Rot;
    // create new target MD workspace
    pTargWS->releaseWorkspace();   
    pTargWS->createEmptyMDWS(WSD);


    ConvToMDSelector AlgoSelector;
    pConvMethods = AlgoSelector.convSelector(inWsEv);
    pConvMethods->initialize(WSD,pTargWS);

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

    WSD.setDetectors(DetLoc_histo);
    WSD.m_RotMatrix = Rot;
    // create new target MD workspace
    pTargWS->releaseWorkspace();   
    pTargWS->createEmptyMDWS(WSD);


    pTargWS->createEmptyMDWS(WSD);

    ConvToMDSelector AlgoSelector;
    pConvMethods = AlgoSelector.convSelector(inWs2D);
    pConvMethods->initialize(WSD,pTargWS);

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

    WSD.setDetectors(DetLoc_histo);
    WSD.m_RotMatrix = Rot;
    // create new target MD workspace
    pTargWS->releaseWorkspace();   
    pTargWS->createEmptyMDWS(WSD);


    pTargWS->createEmptyMDWS(WSD);

    ConvToMDSelector AlgoSelector;
    pConvMethods = AlgoSelector.convSelector(inWs2D);
    pConvMethods->initialize(WSD,pTargWS);

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

   inWs2D = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(int(numHist), int(nEvents)));
   // add workspace energy
   inWs2D->mutableRun().addProperty("Ei",12.,"meV",true);


   pMockAlgorithm = std::auto_ptr<WorkspaceCreationHelper::MockAlgorithm>(new WorkspaceCreationHelper::MockAlgorithm(numHist));
   DetLoc_histo.processDetectorsPositions(inWs2D,pMockAlgorithm->getLogger(),pMockAlgorithm->getProgress());
   DetLoc_events.processDetectorsPositions(inWsEv,pMockAlgorithm->getLogger(),pMockAlgorithm->getProgress());

   pTargWS = boost::shared_ptr<MDEventWSWrapper>(new MDEventWSWrapper());

   Rot.setRandom(100);
   Rot.toRotation();


}

};

#endif