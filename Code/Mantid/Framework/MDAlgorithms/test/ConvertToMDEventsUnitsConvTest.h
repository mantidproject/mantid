#ifndef CONVERT2_MDEVENTS_UNITS_CONVERSION_TEST_H_
#define CONVERT2_MDEVENTS_UNITS_CONVERSION_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/ConvertToMDEventsUnitsConv.h"
#include "MantidMDAlgorithms/ConvertToMDEventsDetInfo.h"
#include "MantidMDAlgorithms/ConvertToMDEvents.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidMDAlgorithms/NotMD.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;



class ConvertToMDEventsUnitsConvTest : public CxxTest::TestSuite, public ConvertToMDEvents
{
   Mantid::API::MatrixWorkspace_sptr ws2D;
//   static Mantid::Kernel::Logger &g_log;
   std::auto_ptr<API::Progress > pProg;
   PreprocessedDetectors det_loc;

public:
static ConvertToMDEventsUnitsConvTest *createSuite() {
    return new ConvertToMDEventsUnitsConvTest(); 
    //g_log = Kernel::Logger::get("MD-Algorithms-Tests");
}
static void destroySuite(ConvertToMDEventsUnitsConvTest  * suite) { delete suite; }    

void test_nothing()
{
 //   std::auto_ptr<NotMD> pD = std::auto_ptr<NotMD>(new NotMD()); 
    pProg =  std::auto_ptr<API::Progress >(new API::Progress(dynamic_cast<ConvertToMDEvents *>(this),0.0,1.0,4));

    TS_ASSERT_THROWS_NOTHING(processDetectorsPositions(ws2D,det_loc,ConvertToMDEvents::getLogger(),pProg.get()));
}



ConvertToMDEventsUnitsConvTest (){

    
   int numHist=10;
   
   ws2D =WorkspaceCreationHelper::createProcessedWorkspaceWithCylComplexInstrument(4,10,true);

 


  
 //  wsEv->setInstrument( ComponentCreationHelper::createTestInstrumentCylindrical(numHist) );
 //  // any inelastic units or unit conversion using TOF needs Ei to be present among properties. 
 ////  wsEv->mutableRun().addProperty("Ei",13.,"meV",true);

}
};
#endif
