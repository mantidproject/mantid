#ifndef MANTIDMD_CONVERT2MD_TEST_PERFORM_H_
#define MANTIDMD_CONVERT2MD_TEST_PERFORM_H_

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidAPI/TextAxis.h"
#include "MantidMDAlgorithms/ConvertToMDEvents.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDEvents;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDAlgorithms::ConvertToMD;

class ConvertToMDEventsTestPerformance : public CxxTest::TestSuite
{
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

    det_loc.setEmode(WSD.emode);
    det_loc.setL1(10);
    det_loc.setEfix(WSD.Ei);


}

void test_EventNoUnitsConv()
{
    pTargWS->createEmptyMDWS(WSD);

    pConvMethods = std::auto_ptr<IConvertToMDEventsWS>(new ConvertToMDEventsWS<EventWSType,Q3D,Direct,ConvertNo,CrystType>());
    pConvMethods->setUPConversion(inWsEv,det_loc,WSD,pTargWS);        

    pMockAlgorithm->resetProgress(numHist);
    TS_ASSERT_THROWS_NOTHING(pConvMethods->runConversion(pMockAlgorithm->getProgress()));
}


ConvertToMDEventsTestPerformance():
WSD(4)
{
   numHist=100*100;
   inWsEv = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateRandomEventWorkspace(1000, numHist, 0.1));
   inWsEv->setInstrument( ComponentCreationHelper::createTestInstrumentCylindrical(numHist) );

   
   pMockAlgorithm = std::auto_ptr<WorkspaceCreationHelper::MockAlgorithm>(new WorkspaceCreationHelper::MockAlgorithm(numHist));
   det_loc.processDetectorsPositions(inWsEv,pMockAlgorithm->getLogger(),pMockAlgorithm->getProgress());


   pTargWS = boost::shared_ptr<MDEventWSWrapper>(new MDEventWSWrapper());

}

};

#endif