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

   Mantid::API::MatrixWorkspace_sptr inWs2D;
   Mantid::API::MatrixWorkspace_sptr inWsEv;
//   static Mantid::Kernel::Logger &g_log;
   std::auto_ptr<API::Progress > pProg;
   std::auto_ptr<IConvertToMDEventsWS> pConvMethods;
   ConvToMDPreprocDetectors det_loc;

public:
static ConvertToMDEventsTestPerformance *createSuite() { return new ConvertToMDEventsTestPerformance(); }
static void destroySuite(ConvertToMDEventsTestPerformance * suite) { delete suite; }    

void test_EventNoUnitsConv()
{
    pConvMethods = std::auto_ptr<IConvertToMDEventsWS>(new ConvertToMDEventsWS<EventWSType,Q3D,Direct,ConvertNo,CrystType>());
}


ConvertToMDEventsTestPerformance()
{
   int numHist=100*100;
   Mantid::API::MatrixWorkspace_sptr inWsEv = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateRandomEventWorkspace(1000, numHist, 0.1));
   inWsEv->setInstrument( ComponentCreationHelper::createTestInstrumentCylindrical(numHist) );
 
}

};

#endif