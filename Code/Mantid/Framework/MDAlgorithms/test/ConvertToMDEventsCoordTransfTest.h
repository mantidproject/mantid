#ifndef CONV2_MDEVENTS_COORD_TRANSF_TEST_H_
#define CONV2_MDEVENTS_COORD_TRANSF_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/ConvertToMDEventsUnitsConv.h"
#include "MantidMDAlgorithms/ConvertToMDEventsDetInfo.h"
#include "MantidMDAlgorithms/ConvertToMDEvents.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/Progress.h"
#include "MantidMDAlgorithms/ConvertToMDEventsCoordTransf.h"

//namespace ConvertToMDEventsTest
//{

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::Geometry;


class ConvertToMDEventsCoordTestHelper :public IConvertToMDEventsMethods
{
    size_t conversionChunk(size_t job_ID){UNUSED_ARG(job_ID);return 0;}
public:
    void setUPConversion(Mantid::API::MatrixWorkspace_sptr pWS2D, const PreprocessedDetectors &detLoc)
    {
        MDEvents::MDWSDescription TestWS(4);

        TestWS.Ei   = *(dynamic_cast<Kernel::PropertyWithValue<double>  *>(pWS2D->run().getProperty("Ei")));
        TestWS.emode= MDAlgorithms::Direct;
        TestWS.dim_min.assign(4,-3);
        TestWS.dim_max.assign(4,3);
        TestWS.dim_names[1]="phi";
        TestWS.dim_names[2]="chi";
        TestWS.dim_names[3]="omega";

        boost::shared_ptr<MDEvents::MDEventWSWrapper> pOutMDWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());
        pOutMDWSWrapper->createEmptyMDWS(TestWS);

        IConvertToMDEventsMethods::setUPConversion(pWS2D,detLoc,TestWS,pOutMDWSWrapper);

    }
    /// method which starts the conversion procedure
    void runConversion(API::Progress *){} 
};

//
class ConvertToMDEventsCoordTransfTest : public CxxTest::TestSuite, public ConvertToMDEvents
{
   Mantid::API::MatrixWorkspace_sptr ws2D;
//   static Mantid::Kernel::Logger &g_log;
   std::auto_ptr<API::Progress > pProg;
   std::auto_ptr<ConvertToMDEventsCoordTestHelper> pConvMethods;
   PreprocessedDetectors det_loc;

public:
static ConvertToMDEventsCoordTransfTest *createSuite() {
    return new ConvertToMDEventsCoordTransfTest(); 
    //g_log = Kernel::Logger::get("MD-Algorithms-Tests");
}
static void destroySuite(ConvertToMDEventsCoordTransfTest  * suite) { delete suite; }    

void test_CoordTransfNOQ()
{
    COORD_TRANSFORMER<NoQ,ANY_Mode,ConvertNo,Histohram> Copy;
    Copy.setUpTransf(pConvMethods.get());
    std::vector<coord_t> Coord(4);

    // copy one generic variable from WS axis (Y axis is not deined)
    TS_ASSERT_THROWS_NOTHING(Copy.calcGenericVariables(Coord,4));
    TS_ASSERT_THROWS_NOTHING(Copy.calcYDepCoordinates(Coord,0));
    const MantidVec& X        = ws2D->readX(0);
    size_t n_data = (X.size()-1);
    for(size_t i=0;i<n_data;i++){
        TS_ASSERT_THROWS_NOTHING(Copy.calcMatrixCoord(X,0,i,Coord));
        TS_ASSERT_DELTA(0.5*(X[i]+X[i+1]),Coord[0],1.e-5);  
    }


}



ConvertToMDEventsCoordTransfTest (){    

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
    pConvMethods = std::auto_ptr<ConvertToMDEventsCoordTestHelper>(new ConvertToMDEventsCoordTestHelper());
    pConvMethods->setUPConversion(ws2D,det_loc);


}

};

//} // end Namespace
#endif


