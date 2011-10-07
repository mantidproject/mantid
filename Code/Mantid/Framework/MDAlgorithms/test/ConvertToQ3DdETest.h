#ifndef MANTID_MD_CONVERT2_QxyzDE_TEST_H_
#define MANTID_MD_CONVERT2_QxyzDE_TEST_H_

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDAlgorithms/ConvertToQ3DdE.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDEvents;

class ConvertToQ3DdETest : public CxxTest::TestSuite
{
 std::auto_ptr<ConvertToQ3DdE> pAlg;
public:
static ConvertToQ3DdETest *createSuite() { return new ConvertToQ3DdETest(); }
static void destroySuite(ConvertToQ3DdETest * suite) { delete suite; }    

void testInit(){

    TS_ASSERT_THROWS_NOTHING( pAlg->initialize() )
    TS_ASSERT( pAlg->isInitialized() )

    TSM_ASSERT_EQUALS("algortithm should have 6 propeties",6,(size_t)(pAlg->getProperties().size()));
}

void testExecThrow(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::CreateGroupedWorkspace2DWithRingsAndBoxes();

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS("the inital ws is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()),std::invalid_argument);
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));


}



void testWithExistingLatticeTrowsLowEnergy(){
    // create model processed workpsace with 10x10 cylindrical detectors, 10 energy levels and oriented lattice
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("EnergyInput", "2."));

    pAlg->execute();
    TSM_ASSERT("Should be not-successful as input energy was lower then obtained",!pAlg->isExecuted());

}
void testExecFailsOnNewWorkspaceNoLimits(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("EnergyInput", "12."));

    pAlg->execute();
    TSM_ASSERT("Should fail as no  min-max limits were specied ",!pAlg->isExecuted());

}
void testExecFailsOnNewWorkspaceNoMaxLimits(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("EnergyInput", "12."));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MinQdE_values", "-50.,-50.,-50,-2"));

    pAlg->execute();
    TSM_ASSERT("Should fail as no max limits were specied ",!pAlg->isExecuted());

}
void testExecFailsLimits_MinGeMax(){
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("EnergyInput", "12."));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MinQdE_values", "-50.,-50.,-50,-2"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MaxQdE_values", " 50., 50.,-50,-2"));

    pAlg->execute();
    TSM_ASSERT("Should fail as no max limits were specied ",!pAlg->isExecuted());

}



void testExecFine(){
    // create model processed workpsace with 10x10 cylindrical detectors, 10 energy levels and oriented lattice
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);

    AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("EnergyInput", "12."));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MinQdE_values", "-50.,-50.,-50,-2"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MaxQdE_values", " 50., 50., 50, 20"));

    pAlg->execute();
    TSM_ASSERT("Should be successful ",pAlg->isExecuted());

}
void testExecAndAdd(){
    // create model processed workpsace with 10x10 cylindrical detectors, 10 energy levels and oriented lattice
    Mantid::API::MatrixWorkspace_sptr ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(100,10,true);

  // rotate the crystal by twenty degrees back;
     ws2D->mutableRun().getGoniometer().setRotationAngle(0,20);
     // add workspace energy
     ws2D->mutableRun().addProperty("Ei",13.,"meV",true);
 //  

     AnalysisDataService::Instance().addOrReplace("testWSProcessed", ws2D);

 
    TSM_ASSERT_THROWS_NOTHING("the inital is not in the units of energy transfer",pAlg->setPropertyValue("InputWorkspace", ws2D->getName()));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("OutputWorkspace", "EnergyTransfer4DWS"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("EnergyInput", "12."));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MinQdE_values", "-50.,-50.,-50,-2"));
    TS_ASSERT_THROWS_NOTHING(pAlg->setPropertyValue("MaxQdE_values", " 50., 50., 50, 20"));


    pAlg->execute();
    TSM_ASSERT("Should be successful ",pAlg->isExecuted());
    // check if the algorith used correct energy
    TSM_ASSERT_EQUALS("The energy, used by the algorithm has not been reset from the workspace", "13",std::string(pAlg->getProperty("EnergyInput")));

}

//TODO: check the results;


ConvertToQ3DdETest(){
    pAlg = std::auto_ptr<ConvertToQ3DdE>(new ConvertToQ3DdE());
}

};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

