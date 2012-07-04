#ifndef CONVERT2_MDEVENTS_UNITS_CONVERSION_TEST_H_
#define CONVERT2_MDEVENTS_UNITS_CONVERSION_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/UnitFactory.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidMDEvents/ConvToMDPreprocDet.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/Progress.h"

#include "MantidMDEvents/UnitsConversionHelper.h"
#include "MantidMDEvents/MDWSDescription.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;



class ConvertToMDUnitsConvTest : public CxxTest::TestSuite
{
   Mantid::API::MatrixWorkspace_sptr ws2D;
   ConvToMDPreprocDet det_loc;

public:
static ConvertToMDUnitsConvTest *createSuite() {
    return new ConvertToMDUnitsConvTest(); 
}
static void destroySuite(ConvertToMDUnitsConvTest  * suite) { delete suite; }    

void testSpecialConversionTOF()
{
    double factor,power;

    const Kernel::Unit_sptr pThisUnit=Kernel::UnitFactory::Instance().create("Wavelength");
    TS_ASSERT(!pThisUnit->quickConversion("MomentumTransfer",factor,power));
}

void testTOFConversionRuns()
{ 

    Kernel::Unit_sptr pSourceWSUnit     = Kernel::UnitFactory::Instance().create("Wavelength");
    Kernel::Unit_sptr pWSUnit           = Kernel::UnitFactory::Instance().create("MomentumTransfer");
    double delta;
    double L1(10),L2(10),TwoTheta(0.1),efix(10);
    int emode(0);
    TS_ASSERT_THROWS_NOTHING(pWSUnit->initialize(L1,L2,TwoTheta,emode,efix,delta));
    TS_ASSERT_THROWS_NOTHING(pSourceWSUnit->initialize(L1,L2,TwoTheta,emode,efix,delta));
     
    double X0(5);
    double tof(0);
    TS_ASSERT_THROWS_NOTHING(tof  = pSourceWSUnit->singleToTOF(X0));
    TS_ASSERT_THROWS_NOTHING(pWSUnit->singleFromTOF(tof));
}



void testConvertFastFromInelasticWS()
{
    UnitsConversionHelper Conv;
    MDWSDescription WSD;

    // ws description currently needs min/max to be set properly
    std::vector<double> min(2,-10),max(2,10);
    WSD.setMinMax(min,max);

    WSD.buildFromMatrixWS(ws2D,"|Q|","Direct");
    WSD.setDetectors(det_loc);


    // initialize peculiar conversion from ws units to DeltaE_inWavenumber
    TS_ASSERT_THROWS_NOTHING(Conv.initialize(WSD,"DeltaE_inWavenumber"));

     const MantidVec& X        = ws2D->readX(0);
     size_t n_bins = X.size()-1;
     for(size_t i=0;i<n_bins;i++){
         TS_ASSERT_DELTA(X[i]*8.06554465,Conv.convertUnits(X[i]),1.e-4);
     }

}
void testConvertToTofInelasticWS()
{
    UnitsConversionHelper Conv;
    MDWSDescription WSD;

    // ws description currently needs min/max to be set properly
    std::vector<double> min(2,-10),max(2,10);
    WSD.setMinMax(min,max);

    WSD.buildFromMatrixWS(ws2D,"|Q|","Direct");
    WSD.setDetectors(det_loc);

    // initalize Convert to TOF
    TS_ASSERT_THROWS_NOTHING(Conv.initialize(WSD,"TOF"));


     const MantidVec& X        = ws2D->readX(0);
     MantidVec E_storage(X.size());
     TS_ASSERT_THROWS_NOTHING(Conv.updateConversion(0));

     size_t n_bins = X.size();
     std::vector<double> TOFS(n_bins);
     for(size_t i=0;i<n_bins;i++){
         E_storage[i]=X[i];
         TOFS[i]     =Conv.convertUnits(X[i]);
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

     // initialize matrix ws description, to the same number of dimensions as before
     WSD.buildFromMatrixWS(ws2D,"|Q|","Direct");

     //initialize Convert back;
     TS_ASSERT_THROWS_NOTHING(Conv.initialize(WSD,"DeltaE"));
     TS_ASSERT_THROWS_NOTHING(Conv.updateConversion(0));

     for(size_t i=0;i<n_bins;i++){
         TS_ASSERT_DELTA(E_storage[i],Conv.convertUnits(TOFS[i]),1.e-5);
     }
}


ConvertToMDUnitsConvTest()
{
    
   API::FrameworkManager::Instance();

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
  

}
};
#endif
