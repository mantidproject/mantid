#ifndef IKEDACARPENTERPVTEST_H_
#define IKEDACARPENTERPVTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/IkedaCarpenterPV.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/LinearBackground.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidKernel/Exception.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidNexus/LoadNeXus.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/cow_ptr.h"
#include <limits>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::NeXus;

class IkedaCarpenterPVTest : public CxxTest::TestSuite
{
public:

  IkedaCarpenterPVTest()
  {
    ConfigService::Instance().setString("curvefitting.peakRadius","100");
  }

  void getMockData(Mantid::MantidVec& y, Mantid::MantidVec& e)
  {
    y[0]  =    0.0000;
    y[1]  =    0.0003;
    y[2]  =    0.0028;
    y[3]  =    0.0223;
    y[4]  =    0.1405;
    y[5]  =    0.6996;
    y[6]  =    2.7608;
    y[7]  =    8.6586;
    y[8]  =   21.6529;
    y[9]  =   43.3558;
    y[10] =   69.8781;
    y[11] =   91.2856;
    y[12] =   97.5646;
    y[13] =   86.4481;
    y[14] =   64.7703;
    y[15] =   42.3348;
    y[16] =   25.3762;
    y[17] =   15.0102;
    y[18] =    9.4932;
    y[19] =    6.7037;
    y[20] =    5.2081;
    y[21] =    4.2780;
    y[22] =    3.6037;
    y[23] =    3.0653;
    y[24] =    2.6163;
    y[25] =    2.2355;
    y[26] =    1.9109;
    y[27] =    1.6335;
    y[28] =    1.3965;
    y[29] =    1.1938;
    y[30] =    1.0206;

    e[0]  =      0.0056;
    e[1]  =      0.0176;
    e[2]  =      0.0539;
    e[3]  =      0.1504;
    e[4]  =      0.3759;
    e[5]  =      0.8374;
    e[6]  =      1.6626;
    e[7]  =      2.9435;
    e[8]  =      4.6543;
    e[9]  =      6.5855;
    e[10] =      8.3603;
    e[11] =      9.5553;
    e[12] =      9.8785;
    e[13] =      9.2987;
    e[14] =      8.0490;
    e[15] =      6.5075;
    e[16] =      5.0385;
    e[17] =      3.8753;
    e[18] =      3.0821;
    e[19] =      2.5902;
    e[20] =      2.2831;
    e[21] =      2.0693;
    e[22] =      1.8993;
    e[23] =      1.7518;
    e[24] =      1.6185;
    e[25] =      1.4962;
    e[26] =      1.3833;
    e[27] =      1.2791;
    e[28] =      1.1827;
    e[29] =      1.0936;
    e[30] =      1.0112;
  }

  // here tries to fit an IC peak to a Gaussian mock data peak
  void testAgainstMockData()
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "IkedaCarpenterPV1D_GaussMockData";
    int histogramNumber = 1;
    int timechannels = 31;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < timechannels; i++) ws2D->dataX(0)[i] = i*5;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));


    // Set general Fit parameters
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","150");

    // set up fitting function and pass to Fit
    IkedaCarpenterPV* icpv = new IkedaCarpenterPV();  
    icpv->initialize();

    icpv->setParameter("I",1000);
    icpv->tie("Alpha0", "1.597107");
    icpv->tie("Alpha1", "1.496805");
    icpv->tie("Beta0", "31.891718");
    icpv->tie("Kappa", "46.025921");
    //icpv->tie("SigmaSquared", "100.0");
    icpv->setParameter("X0",45.0);
    //icpv->tie("Gamma", "1.0");

    alg2.setPropertyValue("Function",*icpv);

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )    
    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 13.13,1);

    IFitFunction *out = FunctionFactory::Instance().createInitialized(alg2.getPropertyValue("Function")); 
    IPeakFunction *pk = dynamic_cast<IPeakFunction *>(out); 

    TS_ASSERT_DELTA( pk->height(), 13.99 ,1);
    TS_ASSERT_DELTA( pk->centre(), 48.229 ,1);
    TS_ASSERT_DELTA( pk->width(), 0.4816 ,0.01);
    TS_ASSERT_DELTA( out->getParameter("I"), 374.93, 1);
    TS_ASSERT_DELTA( out->getParameter("Alpha0"), 1.597107 ,0.0001);
    TS_ASSERT_DELTA( out->getParameter("Alpha1"), 1.496805 ,0.001);
    TS_ASSERT_DELTA( out->getParameter("Beta0"), 31.891718 ,0.0001);
    TS_ASSERT_DELTA( out->getParameter("Kappa"), 46.025921 ,0.0001);
    TS_ASSERT_DELTA( out->getParameter("SigmaSquared"), 0.0338 ,0.001);
    TS_ASSERT_DELTA( out->getParameter("Gamma"), 0.0484 ,0.01);
    TS_ASSERT_DELTA( out->getParameter("X0"), 48.229 ,0.1);

   
    // could set workspace here but makes no difference since
    // regardless m_wavelength set to zero in IC code 
    //pk->setMatrixWorkspace(ws2D, 0,-1,-1);

    const double* x = &ws2D->readX(0)[0];
    double *yy = new double[timechannels]; 
    pk->function(yy, x, timechannels);

    // note that fitting a none-totally optimized IC to a Gaussian peak so 
    // not a perfect fit - but pretty ok result
    TS_ASSERT_DELTA( yy[9], 1.22099 ,0.1);
    TS_ASSERT_DELTA( yy[10], 90.7193 ,4);
    TS_ASSERT_DELTA( yy[11], 93.1314 ,4);
    TS_ASSERT_DELTA( yy[12], 41.1798, 2);
    TS_ASSERT_DELTA( yy[13], 15.0869 ,1);
    TS_ASSERT_DELTA( yy[14], 5.55355 ,1);

    delete[] yy;

    AnalysisDataService::Instance().remove(wsName);
  }


};

#endif /*IKEDACARPENTERPVTEST_H_*/
