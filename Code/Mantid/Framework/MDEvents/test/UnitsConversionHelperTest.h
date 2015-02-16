#ifndef CONVERT2_MDEVENTS_UNITS_CONVERSION_TEST_H_
#define CONVERT2_MDEVENTS_UNITS_CONVERSION_TEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/UnitFactory.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/Progress.h"

#include "MantidMDEvents/UnitsConversionHelper.h"
#include "MantidMDEvents/MDWSDescription.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;


class UnitsConversionHelperTest : public CxxTest::TestSuite
{
  Mantid::API::MatrixWorkspace_sptr ws2D;
  Mantid::DataObjects::TableWorkspace_sptr  detLoc;

public:
  static UnitsConversionHelperTest *createSuite() {
    return new UnitsConversionHelperTest(); 
  }
  static void destroySuite(UnitsConversionHelperTest  * suite) { delete suite; }    

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
    WSD.m_PreprDetTable = detLoc;


    // initialize peculiar conversion from ws units to DeltaE_inWavenumber
    TS_ASSERT_THROWS_NOTHING(Conv.initialize(WSD,"DeltaE_inWavenumber"));

    const MantidVec& X        = ws2D->readX(0);
    size_t n_bins = X.size()-1;
    for(size_t i=0;i<n_bins;i++)
    {
      TS_ASSERT_DELTA(X[i]*8.06554465,Conv.convertUnits(X[i]),1.e-4);
    }

    auto range = Conv.getConversionRange(0,10);
    TS_ASSERT_EQUALS(0,range.first);
    TS_ASSERT_EQUALS(3,range.second);

    range = Conv.getConversionRange(-10,3);
    TS_ASSERT_EQUALS(-10,range.first);
    TS_ASSERT_EQUALS(3,range.second);


    range = Conv.getConversionRange(-100000,2);
    TS_ASSERT_EQUALS(-100000,range.first);
    TS_ASSERT_EQUALS(2,range.second);

    range = Conv.getConversionRange(0,-100000);
    TS_ASSERT_EQUALS(-100000,range.first);
    TS_ASSERT_EQUALS(0,range.second);

  }
  void testConvertToTofInelasticWS()
  {
    UnitsConversionHelper Conv;
    MDWSDescription WSD;

    // ws description currently needs min/max to be set properly
    std::vector<double> min(2,-10),max(2,10);
    WSD.setMinMax(min,max);

    WSD.buildFromMatrixWS(ws2D,"|Q|","Direct");
    WSD.m_PreprDetTable = detLoc;

    // initalize Convert to TOF
    TS_ASSERT_THROWS_NOTHING(Conv.initialize(WSD,"TOF"));
    double t_1 = Conv.convertUnits(3.);
    double t_2 = Conv.convertUnits(10);
    double t_3 = Conv.convertUnits(-10);
    double t_4 = Conv.convertUnits(-100);
    double t_lim = Conv.convertUnits(-DBL_MAX);

    const MantidVec& X        = ws2D->readX(0);
    MantidVec E_storage(X.size());
    TS_ASSERT_THROWS_NOTHING(Conv.updateConversion(0));

    size_t n_bins = X.size();
    std::vector<double> TOFS(n_bins);
    for(size_t i=0;i<n_bins;i++)
    {
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
    WSD.m_PreprDetTable = detLoc;

    //initialize Convert back;
    TS_ASSERT_THROWS_NOTHING(Conv.initialize(WSD,"DeltaE"));
    TS_ASSERT_THROWS_NOTHING(Conv.updateConversion(0));

    for(size_t i=0;i<n_bins;i++)
    {
      TS_ASSERT_DELTA(E_storage[i],Conv.convertUnits(TOFS[i]),1.e-5);
    }

    auto range = Conv.getConversionRange(-1000000000,1000000000);
    TS_ASSERT_DELTA(t_lim,range.first,1.e-8);
    TS_ASSERT_EQUALS(1000000000,range.second);

    range = Conv.getConversionRange(t_1,t_2);
    TS_ASSERT_EQUALS(3,Conv.convertUnits(range.first));
    TS_ASSERT_EQUALS(3,Conv.convertUnits(range.second));


    range = Conv.getConversionRange(t_3,t_4);
    TS_ASSERT_DELTA(-100,Conv.convertUnits(range.first),1.e-6);
    TS_ASSERT_DELTA(-10,Conv.convertUnits(range.second),1.e-6);

    range = Conv.getConversionRange(t_2,t_3);
    TS_ASSERT_DELTA(-10,Conv.convertUnits(range.first),1.e-6);
    TS_ASSERT_DELTA(3,Conv.convertUnits(range.second),1.e-6);


  }

  void testConvertViaTOFElastic()
  {

    // Modify input workspace to be elastic workspace
    const MantidVec& X        = ws2D->readX(0);
    MantidVec E_storage(X.size());
    size_t n_bins = X.size();
    for(size_t i=0;i<n_bins;i++)
    {
      E_storage[i]=-0.1+0.1*static_cast<double>(i);
    }

    NumericAxis *pAxis0 = new NumericAxis(n_bins-1); 
    pAxis0->setUnit("Energy");
    ws2D->replaceAxis(0,pAxis0);    


    //---------------------------------------------------------------------------------
    UnitsConversionHelper Conv;
    MDWSDescription WSD;
    // ws description currently needs min/max to be set properly
    std::vector<double> min(3,-10),max(3,10);
    WSD.setMinMax(min,max);

    WSD.buildFromMatrixWS(ws2D,"Q3D","Elastic");
    WSD.m_PreprDetTable = detLoc;



    // initalize Convert from energy to momentum, forcing convert wia TOF
    TS_ASSERT_THROWS_NOTHING(Conv.initialize(WSD,"Momentum",true));


    std::vector<double> Momentums(n_bins);
    for(size_t i=0;i<n_bins;i++)
    {
      Momentums[i]     =Conv.convertUnits(E_storage[i]);
    }

    auto range = Conv.getConversionRange(-10,10);
    TS_ASSERT_DELTA(0,range.first,1.e-8);
    TS_ASSERT_DELTA(10,range.second,1.e-8);

 
    range = Conv.getConversionRange(10000,1);
    TS_ASSERT_DELTA(1,range.first,1.e-8);
    TS_ASSERT_DELTA(10000,range.second,1.e-8);


    // initalize Convert from momentum to energy, forcing convert wia TOF
    pAxis0 = new NumericAxis(n_bins-1); 
    pAxis0->setUnit("Momentum");
    ws2D->replaceAxis(0,pAxis0);    
    WSD.buildFromMatrixWS(ws2D,"Q3D","Elastic");

    TS_ASSERT_THROWS_NOTHING(Conv.initialize(WSD,"Energy",true));
    // note comparison from 1 as negative energies were not converted to momentum/back
    for(size_t i=1;i<n_bins;i++)
    {
      TS_ASSERT_DELTA(E_storage[i],Conv.convertUnits(Momentums[i]) ,1.e-8);
    }
    // this test may indicate problem -- not if this problem ever find in reality
    range = Conv.getConversionRange(-10,10);
    TS_ASSERT_DELTA(0,range.first,1.e-8);
    TS_ASSERT_DELTA(10,range.second,1.e-8);


    range = Conv.getConversionRange(1.e-10,10);
    TS_ASSERT_DELTA(0,range.first,1.e-8);
    TS_ASSERT_DELTA(10,range.second,1.e-8);


  }


  UnitsConversionHelperTest()
  {

    API::FrameworkManager::Instance();

    std::vector<double> L2(5,5);
    std::vector<double> polar(5,(30./180.)*M_PI);
    polar[0]=0;
    std::vector<double> azimutal(5,0);
    azimutal[1]=(45./180.)*M_PI;
    azimutal[2]=(90./180.)*M_PI;
    azimutal[3]=(135./180.)*M_PI;
    azimutal[4]=(180./180.)*M_PI;

    int numBins=10;
    ws2D =WorkspaceCreationHelper::createProcessedInelasticWS(L2, polar, azimutal,numBins,-1,3,3);

    detLoc = WorkspaceCreationHelper::buildPreprocessedDetectorsWorkspace(ws2D);


  }
};
#endif
