#ifndef CONVERT2_MDEVENTS_METHODS_TEST_H_
#define CONVERT2_MDEVENTS_METHODS_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/ConvertToMDEventsUnitsConv.h"
#include "MantidMDAlgorithms/ConvertToMDEventsDetInfo.h"
#include "MantidMDAlgorithms/ConvertToMDEvents.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/Progress.h"
#include "MantidMDAlgorithms/ConvertToMDEventsHistoWS.h"
#include "MantidMDAlgorithms/ConvertToMDEventsEventWS.h"



using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::Geometry;



class ConvertToMDEventsMethodsTest : public CxxTest::TestSuite, public ConvertToMDEvents
{
   Mantid::API::MatrixWorkspace_sptr ws2D;

   std::auto_ptr<API::Progress > pProg;   
   PreprocessedDetectors det_loc;

public:
static ConvertToMDEventsMethodsTest *createSuite() {
    return new ConvertToMDEventsMethodsTest(); 
    //g_log = Kernel::Logger::get("MD-Algorithms-Tests");
}
static void destroySuite(ConvertToMDEventsMethodsTest  * suite) { delete suite; }    

void test_TwoTransfMethods()
{

    ConvertToMDEvensHistoWS<Q3D,Direct,ConvertNo> HistoConv;
    
    MDEvents::MDWSDescription TestWS(4);

    TestWS.Ei   = *(dynamic_cast<Kernel::PropertyWithValue<double>  *>(ws2D->run().getProperty("Ei")));
    TestWS.emode= MDAlgorithms::Direct;
    TestWS.dim_min.assign(4,-3);
    TestWS.dim_max.assign(4,3);
    TestWS.dim_names.assign(4,"Momentum");
    TestWS.dim_names[3]="DeltaE";
    TestWS.rotMatrix.assign(9,0);
    TestWS.rotMatrix[0]=1;
    TestWS.rotMatrix[4]=1;
    TestWS.rotMatrix[8]=1;

     boost::shared_ptr<MDEvents::MDEventWSWrapper> pOutMDWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());
     pOutMDWSWrapper->createEmptyMDWS(TestWS);


    TS_ASSERT_THROWS_NOTHING(HistoConv.setUPConversion(ws2D,det_loc,TestWS,pOutMDWSWrapper));

    pProg =  std::auto_ptr<API::Progress >(new API::Progress(dynamic_cast<ConvertToMDEvents *>(this),0.0,1.0,4));

}



ConvertToMDEventsMethodsTest (){    

   std::vector<double> L2(5,5);
   std::vector<double> polar(5,(30./180.)*3.1415926);
   polar[0]=0;
   std::vector<double> azimutal(5,0);
   azimutal[1]=(45./180.)*3.1415936;
   azimutal[2]=(90./180.)*3.1415936;
   azimutal[3]=(135./180.)*3.1415936;
   azimutal[4]=(180./180.)*3.1415936;

   int numBins=10;
   ws2D =WorkspaceCreationHelper::createProcessedInelasticWS(L2, polar, azimutal,numBins,-1,3,3);

   Kernel::UnitFactory::Instance().create("TOF");
   Kernel::UnitFactory::Instance().create("Energy");
   Kernel::UnitFactory::Instance().create("DeltaE");
   Kernel::UnitFactory::Instance().create("Momentum");

   // set up workpspaces and preprocess detectors
    pProg =  std::auto_ptr<API::Progress >(new API::Progress(dynamic_cast<ConvertToMDEvents *>(this),0.0,1.0,4));

    processDetectorsPositions(ws2D,det_loc,ConvertToMDEvents::getLogger(),pProg.get());
    //pConvMethods = std::auto_ptr<ConvertToMDEventsCoordTestHelper>(new ConvertToMDEventsCoordTestHelper());
    //pConvMethods->setUPConversion(ws2D,det_loc);


}

};



#endif