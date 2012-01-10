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
 
    pProg =  std::auto_ptr<API::Progress >(new API::Progress(dynamic_cast<ConvertToMDEvents *>(this),0.0,1.0,4));

    TS_ASSERT_THROWS_NOTHING(processDetectorsPositions(ws2D,det_loc,ConvertToMDEvents::getLogger(),pProg.get()));
}



ConvertToMDEventsUnitsConvTest (){

    

   std::vector<double> L2(5,5);
   std::vector<double> polar(5,(30./180.)*3.1415926);
   polar[0]=0;
   std::vector<double> azimutal(5,0);
   azimutal[1]=(45./180.)*3.1415936;
   azimutal[2]=(90./180.)*3.1415936;
   azimutal[3]=(135./180.)*3.1415936;
   azimutal[4]=(180./180.)*3.1415936;

   int numBins=10;
   ws2D =WorkspaceCreationHelper::createProcessedInelasticWS(L2, polar, azimutal,numBins,-1,3,4);


}
};
#endif
