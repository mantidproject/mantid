#ifndef CONVERT2_MDEVENTS_UNITS_CONVERSION_TEST_H_
#define CONVERT2_MDEVENTS_UNITS_CONVERSION_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/NumericAxis.h"
#include "MantidMDAlgorithms/ConvertToMDEventsUnitsConv.h"
#include "MantidMDAlgorithms/ConvToMDPreprocDetectors.h"
#include "MantidMDAlgorithms/ConvertToMDEventsParams.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/Progress.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::MDAlgorithms::ConvertToMD;




class ConvertToMDEventsUnitsConvTest : public CxxTest::TestSuite
{
   Mantid::API::MatrixWorkspace_sptr ws2D;
   ConvToMDPreprocDetectors det_loc;

public:
static ConvertToMDEventsUnitsConvTest *createSuite() {
    return new ConvertToMDEventsUnitsConvTest(); 
}
static void destroySuite(ConvertToMDEventsUnitsConvTest  * suite) { delete suite; }    

void testSpecialConversionTOF()
{
    double factor,power;

    const Kernel::Unit_sptr pThisUnit=Kernel::UnitFactory::Instance().create("Wavelength");
    TS_ASSERT(!pThisUnit->quickConversion("MomentumTransfer",factor,power));
}

void testTOFConversionFails()
{ 

    Kernel::Unit_sptr pSourceWSUnit     = Kernel::UnitFactory::Instance().create("Wavelength");
    Kernel::Unit_sptr pWSUnit           = Kernel::UnitFactory::Instance().create("MomentumTransfer");
    double delta;
    double L1(10),L2(10),TwoTheta(0.1),efix(10);
    int emode(0);
    TS_ASSERT_THROWS_NOTHING(pWSUnit->initialize(L1,L2,TwoTheta,emode,efix,delta));
    TS_ASSERT_THROWS_NOTHING(pSourceWSUnit->initialize(L1,L2,TwoTheta,emode,efix,delta));
     
    double X0(5);
    double tof(0) ,k_tr(0);
    TS_ASSERT_THROWS_NOTHING(tof  = pSourceWSUnit->singleToTOF(X0));
    TS_ASSERT_THROWS_NOTHING(k_tr = pWSUnit->singleFromTOF(tof));
}



void testConvertFastFromInelasticWS()
{
    UnitsConverter<ConvFast,Histogram> Conv;

    TS_ASSERT_THROWS_NOTHING(Conv.setUpConversion(det_loc,"DeltaE","DeltaE_inWavenumber"));

     const MantidVec& X        = ws2D->readX(0);
     size_t n_bins = X.size()-1;
     for(size_t i=0;i<n_bins;i++){
         TS_ASSERT_DELTA(0.5*(X[i]+X[i+1])*8.06554465,Conv.getXConverted(X,i),1.e-4);
     }

}
void testConvertToTofInelasticWS()
{
    // Convert to TOF
    UnitsConverter<ConvByTOF,Centered> Conv;
    // set diret conversion mode
    det_loc.setEmode(1);
    TS_ASSERT_THROWS_NOTHING(Conv.setUpConversion(det_loc,"DeltaE","TOF"));


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
     UnitsConverter<ConvFromTOF,Centered> ConvBack;
     std::string uintFrom = ws2D->getAxis(0)->unit()->unitID();
     TS_ASSERT_THROWS_NOTHING(ConvBack.setUpConversion(det_loc,uintFrom,"DeltaE"));
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

   det_loc.buildFakeDetectorsPositions(ws2D);
   det_loc.setEfix(10);
   det_loc.setEmode(1);
  
   Kernel::UnitFactory::Instance().create("TOF");
   Kernel::UnitFactory::Instance().create("Energy");
   Kernel::UnitFactory::Instance().create("DeltaE");
   Kernel::UnitFactory::Instance().create("DeltaE_inWavenumber");
   Kernel::UnitFactory::Instance().create("Momentum");


}
};
#endif
