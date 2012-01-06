#ifndef MANTID_MD_CONVEVENTS2_Q_NDANY_TEST_H_
#define MANTID_MD_CONVEVENTS2_Q_NDANY_TEST_H_

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
using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDEvents;

class ConvertEvents2MDEvTestHelper: public ConvertToMDEvents
{
public:
    ConvertEvents2MDEvTestHelper(){};

};

//
class ConvertEventsToMDEventsTest : public CxxTest::TestSuite
{
 std::auto_ptr<ConvertEvents2MDEvTestHelper> pAlg;
public:
static ConvertEventsToMDEventsTest *createSuite() { return new ConvertEventsToMDEventsTest(); }
static void destroySuite(ConvertEventsToMDEventsTest * suite) { delete suite; }    



void testEventWS()
{
// set up algorithm
   TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("InputWorkspace","testEvWS"));
   TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace","testMDEvWorkspace"));
   TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OtherDimensions",""));
   TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("QDimensions", "QxQyQz"));
   pAlg->setPropertyValue("UsePreprocessedDetectors","0");
   TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("dEAnalysisMode", "Elastic"));
   pAlg->setPropertyValue("MinValues","-10,-10,-10");
   pAlg->setPropertyValue("MaxValues"," 10, 10, 10");

   pAlg->setRethrows(false);
   pAlg->execute();
   TSM_ASSERT("Shoud finish succesfully",pAlg->isExecuted());
   AnalysisDataService::Instance().remove("OutputWorkspace"); 


}

ConvertEventsToMDEventsTest(){
   pAlg = std::auto_ptr<ConvertEvents2MDEvTestHelper>(new ConvertEvents2MDEvTestHelper());
   pAlg->initialize();

   int numHist=10;
   Mantid::API::MatrixWorkspace_sptr wsEv = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::CreateRandomEventWorkspace(100, numHist, 0.1));
   wsEv->setInstrument( ComponentCreationHelper::createTestInstrumentCylindrical(numHist) );
   // any inelastic units or unit conversion using TOF needs Ei to be present among properties. 
 //  wsEv->mutableRun().addProperty("Ei",13.,"meV",true);

   AnalysisDataService::Instance().addOrReplace("testEvWS", wsEv);

}

};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

