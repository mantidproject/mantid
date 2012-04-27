#ifndef MANTIDMD_CONVERT2MD_TEST_PERFORM_H_
#define MANTIDMD_CONVERT2MD_TEST_PERFORM_H_

#include <ctime>
#include <iomanip>
#include <iostream>


#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/CPUTimer.h"

#include "MantidAPI/TextAxis.h"
#include "MantidMDAlgorithms/ConvertToMDEvents.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDEvents;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDAlgorithms::ConvertToMD;

class ConvertToMDEventsTestPerformance : public CxxTest::TestSuite
{
    Kernel::CPUTimer Clock;
     time_t start,end;

    size_t numHist;
    MDWSDescription WSD;

   Mantid::API::MatrixWorkspace_sptr inWs2D;
   Mantid::API::MatrixWorkspace_sptr inWsEv;

   WorkspaceCreationHelper::MockAlgorithm reporter;
   std::auto_ptr<IConvertToMDEventsWS> pConvMethods;
   ConvToMDPreprocDetectors det_loc;
   // pointer to mock algorithm to work with progress bar
   std::auto_ptr<WorkspaceCreationHelper::MockAlgorithm> pMockAlgorithm;

    boost::shared_ptr<MDEvents::MDEventWSWrapper> pTargWS;

public:
static ConvertToMDEventsTestPerformance *createSuite() { return new ConvertToMDEventsTestPerformance(); }
static void destroySuite(ConvertToMDEventsTestPerformance * suite) { delete suite; }    

void setUp()
{
 
    WSD.emode = 2;
    WSD.Ei    = 10;

    Kernel::Matrix<double> Rot(3,3);
    Rot.setRandom(100);
    Rot.toRotation();

    size_t nDim = WSD.nDims;
    WSD.rotMatrix = Rot;
    WSD.dimMin.assign(nDim,-1.e+32);
    WSD.dimMax.assign(nDim, 1.e+32);

    det_loc.setEmode(WSD.emode);
    det_loc.setL1(10);
    det_loc.setEfix(WSD.Ei);

    pTargWS->releaseWorkspace();
    pTargWS->createEmptyMDWS(WSD);
}

void test_EventNoUnitsConv()
{
    pConvMethods = std::auto_ptr<IConvertToMDEventsWS>(new ConvertToMDEventsWS<EventWSType,Q3D,Indir,ConvertNo,CrystType>());
    pConvMethods->setUPConversion(inWsEv,det_loc,WSD,pTargWS);        

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
  
    pConvMethods = std::auto_ptr<IConvertToMDEventsWS>(new ConvertToMDEventsWS<EventWSType,Q3D,Indir,ConvFromTOF,CrystType>());
    pConvMethods->setUPConversion(inWsEv,det_loc,WSD,pTargWS);        

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
  
    pConvMethods = std::auto_ptr<IConvertToMDEventsWS>(new ConvertToMDEventsWS<Ws2DHistoType,Q3D,Indir,ConvFromTOF,CrystType>());
    pConvMethods->setUPConversion(inWsEv,det_loc,WSD,pTargWS);        

    pMockAlgorithm->resetProgress(numHist);
    //Clock.elapsedCPU();
    std::time (&start);
    TS_ASSERT_THROWS_NOTHING(pConvMethods->runConversion(pMockAlgorithm->getProgress()));
    std::time (&end);
    double sec = std::difftime (end,start);
    //float sec = Clock.elapsedCPU();
    TS_WARN("Time to complete: <Ws2DHistoType,Q3D,Indir,ConvFromTOF,CrystType>: "+boost::lexical_cast<std::string>(sec)+" sec");
}

void test_HistoNoUnitsConv()
{
    pConvMethods = std::auto_ptr<IConvertToMDEventsWS>(new ConvertToMDEventsWS<Ws2DHistoType,Q3D,Indir,ConvertNo,CrystType>());
    pConvMethods->setUPConversion(inWsEv,det_loc,WSD,pTargWS);        

    pMockAlgorithm->resetProgress(numHist);
    //Clock.elapsedCPU();
    std::time (&start);
    TS_ASSERT_THROWS_NOTHING(pConvMethods->runConversion(pMockAlgorithm->getProgress()));
    std::time (&end);
    double sec = std::difftime (end,start);
    TS_WARN("Time to complete: <Ws2DHistoType,Q3D,Indir,ConvertNo,CrystType>: "+boost::lexical_cast<std::string>(sec)+" sec");
}


ConvertToMDEventsTestPerformance():
WSD(4)
{
   numHist=100*100;
   size_t nEvents=1000;
   inWsEv = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateRandomEventWorkspace(nEvents, numHist, 0.1));
   inWsEv->setInstrument( ComponentCreationHelper::createTestInstrumentCylindrical(int(numHist)) );

   inWs2D = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(int(numHist), int(nEvents)));

   pMockAlgorithm = std::auto_ptr<WorkspaceCreationHelper::MockAlgorithm>(new WorkspaceCreationHelper::MockAlgorithm(numHist));
   det_loc.processDetectorsPositions(inWs2D,pMockAlgorithm->getLogger(),pMockAlgorithm->getProgress());

   pTargWS = boost::shared_ptr<MDEventWSWrapper>(new MDEventWSWrapper());

}

};

#endif