#ifndef CONVERT2_MDEVENTS_UNITS_CONVERSION_TEST_H_
#define CONVERT2_MDEVENTS_UNITS_CONVERSION_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/ConvertToMDEventsUnitsConv.h"
#include "MantidMDAlgorithms/ConvertToMDEventsDetInfo.h"
#include "MantidMDAlgorithms/ConvertToMDEvents.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/Progress.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::Geometry;


class TestConvertToMDEventsMethods :public IConvertToMDEventsMethods
{
    size_t conversionChunk(size_t job_ID){UNUSED_ARG(job_ID);return 0;}
public:
    void setUPTestConversion(Mantid::API::MatrixWorkspace_sptr pWS2D, const PreprocessedDetectors &detLoc)
    {
        MDEvents::MDWSDescription TestWS(5);

        TestWS.Ei   = *(dynamic_cast<Kernel::PropertyWithValue<double>  *>(pWS2D->run().getProperty("Ei")));
        TestWS.emode= MDAlgorithms::Direct;

        boost::shared_ptr<MDEvents::MDEventWSWrapper> pOutMDWSWrapper = boost::shared_ptr<MDEvents::MDEventWSWrapper>(new MDEvents::MDEventWSWrapper());
        pOutMDWSWrapper->createEmptyMDWS(TestWS);

        IConvertToMDEventsMethods::setUPConversion(pWS2D,detLoc,TestWS,pOutMDWSWrapper);

    }
    /// method which starts the conversion procedure
    void runConversion(API::Progress *){}
 
};


class ConvertToMDEventsUnitsConvTest : public CxxTest::TestSuite, public ConvertToMDEvents
{
   Mantid::API::MatrixWorkspace_sptr ws2D;
//   static Mantid::Kernel::Logger &g_log;
   std::auto_ptr<API::Progress > pProg;
   std::auto_ptr<TestConvertToMDEventsMethods> pConvMethods;
   PreprocessedDetectors det_loc;

public:
static ConvertToMDEventsUnitsConvTest *createSuite() {
    return new ConvertToMDEventsUnitsConvTest(); 
    //g_log = Kernel::Logger::get("MD-Algorithms-Tests");
}
static void destroySuite(ConvertToMDEventsUnitsConvTest  * suite) { delete suite; }    

void testSetUp_and_PreprocessDetectors()
{
    pProg =  std::auto_ptr<API::Progress >(new API::Progress(dynamic_cast<ConvertToMDEvents *>(this),0.0,1.0,4));

    TS_ASSERT_THROWS_NOTHING(processDetectorsPositions(ws2D,det_loc,ConvertToMDEvents::getLogger(),pProg.get()));
    TS_ASSERT_THROWS_NOTHING(pConvMethods = std::auto_ptr<TestConvertToMDEventsMethods>(new TestConvertToMDEventsMethods()));
    TS_ASSERT_THROWS_NOTHING(pConvMethods->setUPTestConversion(ws2D,det_loc));

}

void testConvertFastFromInelasticWS()
{
    UNITS_CONVERSION<ConvFast,Histohram> Conv;
    TS_ASSERT_THROWS_NOTHING(Conv.setUpConversion(pConvMethods.get(),"DeltaE_inWavenumber"));

     const MantidVec& X        = ws2D->readX(0);
     size_t n_bins = X.size()-1;
     for(size_t i=0;i<n_bins;i++){
         TS_ASSERT_DELTA(0.5*(X[i]+X[i+1])*8.06554465,Conv.getXConverted(X,i),1.e-4);
     }

}
void testConvertToTofInelasticWS()
{
    // Convert to TOF
    UNITS_CONVERSION<ConvByTOF,Centered> Conv;
    TS_ASSERT_THROWS_NOTHING(Conv.setUpConversion(pConvMethods.get(),"TOF"));

     const MantidVec& X        = ws2D->readX(0);
     MantidVec E_storage(X.size());
     TS_ASSERT_THROWS_NOTHING(Conv.updateConversion(0));

     size_t n_bins = X.size();
     std::vector<double> TOFS(n_bins);
     for(size_t i=0;i<n_bins;i++){
         E_storage[i]=X[i];
         TOFS[i]     =Conv.getXConverted(X,i);
     }

     // Let WS know that it is in TOF now (one column)
     MantidVec& T = ws2D->dataX(0);

     NumericAxis *pAxis0 = new NumericAxis(n_bins-1); 
     for(size_t i=0; i < n_bins-1; i++){
            double Tm =0.5*(TOFS[i]+TOFS[i+1]);
            pAxis0->setValue(i,Tm);
            T[i]=TOFS[i];
     }
     T[n_bins-1]=TOFS[n_bins-1];

     pAxis0->setUnit("TOF");
     ws2D->replaceAxis(0,pAxis0);



     // Convert back;
     UNITS_CONVERSION<ConvFromTOF,Centered> ConvBack;

     TS_ASSERT_THROWS_NOTHING(ConvBack.setUpConversion(pConvMethods.get(),"DeltaE"));
     TS_ASSERT_THROWS_NOTHING(ConvBack.updateConversion(0));

     for(size_t i=0;i<n_bins;i++){
         TS_ASSERT_DELTA(E_storage[i],ConvBack.getXConverted(TOFS,i),1.e-5);
     }
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
   ws2D =WorkspaceCreationHelper::createProcessedInelasticWS(L2, polar, azimutal,numBins,-1,3,3);

   Kernel::UnitFactory::Instance().create("TOF");
   Kernel::UnitFactory::Instance().create("Energy");
   Kernel::UnitFactory::Instance().create("DeltaE");
   Kernel::UnitFactory::Instance().create("DeltaE_inWavenumber");
   Kernel::UnitFactory::Instance().create("Momentum");


}
};
#endif
