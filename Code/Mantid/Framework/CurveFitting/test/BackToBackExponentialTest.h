#ifndef BACKTOBACKEXPONENTIALTEST_H_
#define BACKTOBACKEXPONENTIALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/BackToBackExponential.h"
#include "MantidCurveFitting/LinearBackground.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/CompositeFunctionMW.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

// Algorithm to force Simplex to run
class SimplexBackToBackExponential : public BackToBackExponential
{
public:
  virtual ~SimplexBackToBackExponential() {}

protected:
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData)
  {
    throw Exception::NotImplementedError("No derivative function provided");
  }
};



class BackToBackExponentialTest : public CxxTest::TestSuite
{
public:

  void getHRP38692_WI2(Mantid::MantidVec& x, Mantid::MantidVec& y, Mantid::MantidVec& e)
  {

    x[0] = 79280.000;
    x[1] = 79284.562;
    x[2] = 79292.437;
    x[3] = 79300.312;
    x[4] = 79308.187;
    x[5] = 79316.062;
    x[6] = 79323.937;
    x[7] = 79331.812;
    x[8] = 79339.687;
    x[9] = 79347.625;
    x[10] = 79355.625;
    x[11] = 79363.625;
    x[12] = 79371.625;
    x[13] = 79379.625;
    x[14] = 79387.625;
    x[15] = 79395.625;
    x[16] = 79403.625;
    x[17] = 79411.625;
    x[18] = 79419.625;
    x[19] = 79427.625;
    x[20] = 79435.625;
    x[21] = 79443.625;
    x[22] = 79451.625;
    x[23] = 79459.625;
    x[24] = 79467.625;
    x[25] = 79475.625;
    x[26] = 79483.625;
    x[27] = 79491.625;
    x[28] = 79499.625;
    x[29] = 79507.625;
    x[30] = 79515.625;
    x[31] = 79523.625;
    x[32] = 79531.625;
    x[33] = 79539.625;
    x[34] = 79547.625;
    x[35] = 79555.625;
    x[36] = 79563.625;
    x[37] = 79571.625;
    x[38] = 79579.625;
    x[39] = 79587.625;
    x[40] = 79595.625;
    x[41] = 79603.625;
    x[42] = 79611.625;
    x[43] = 79619.625;
    x[44] = 79627.625;
    x[45] = 79635.625;
    x[46] = 79643.625;
    x[47] = 79651.625;
    x[48] = 79659.625;
    x[49] = 79667.625;
    x[50] = 79675.625;
    x[51] = 79683.625;
    x[52] = 79691.625;
    x[53] = 79699.625;
    x[54] = 79707.625;
    x[55] = 79715.625;
    x[56] = 79723.625;
    x[57] = 79731.625;
    x[58] = 79739.625;
    x[59] = 79747.625;
    x[60] = 79755.625;

    y[0] =  7  ;
    y[1] =  7  ;
    y[2] =  8  ;
    y[3] =  4  ;
    y[4] =  9  ;
    y[5] =  4  ;
    y[6] =  10 ;
    y[7] =  10 ;
    y[8] =  5  ;
    y[9] =  8  ;
    y[10] = 7  ;
    y[11] = 10 ;
    y[12] = 18 ;
    y[13] = 30 ;
    y[14] = 71 ;
    y[15] = 105;
    y[16] = 167;
    y[17] = 266;
    y[18] = 271;
    y[19] = 239;
    y[20] = 221;
    y[21] = 179;
    y[22] = 133;
    y[23] = 126;
    y[24] = 88 ;
    y[25] = 85 ;
    y[26] = 52 ;
    y[27] = 37 ;
    y[28] = 51 ;
    y[29] = 32 ;
    y[30] = 31 ;
    y[31] = 17 ;
    y[32] = 21 ;
    y[33] = 15 ;
    y[34] = 13 ;
    y[35] = 12 ;
    y[36] = 12 ;
    y[37] = 10 ;
    y[38] = 7  ;
    y[39] = 5  ;
    y[40] = 9  ;
    y[41] = 6  ;
    y[42] = 8  ;
    y[43] = 8  ;
    y[44] = 3  ;
    y[45] = 10 ;
    y[46] = 4  ;
    y[47] = 4  ;
    y[48] = 5  ;
    y[49] = 5  ;
    y[50] = 7  ;
    y[51] = 2  ;
    y[52] = 0  ;
    y[53] = 4  ;
    y[54] = 2  ;
    y[55] = 1  ;
    y[56] = 4  ;
    y[57] = 5  ;
    y[58] = 4  ;
    y[59] = 6  ;
    y[60] = 2  ;    

    for (int i = 0; i<= 60; i++)
      e[i] = sqrt(y[i]); 
  }


  void testAgainstMockData()
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "IkedaCarpenterPV1D_GaussMockData";
    int histogramNumber = 1;
    int timechannels = 61;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    Mantid::MantidVec& x = ws2D->dataX(0);
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getHRP38692_WI2(x, y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));



    alg2.setPropertyValue("InputWorkspace",wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");

    // create function you want to fit against
    CompositeFunctionMW *fnWithBk = new CompositeFunctionMW();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->setParameter("A0",6.0);
    bk->setParameter("A1",0.0);
    bk->removeActive(1);    

    BackToBackExponential* fn = new BackToBackExponential();
    fn->initialize();

    fn->setParameter("I",237.0);
    fn->setParameter("A",0.4);
    fn->setParameter("B",0.03);
    fn->setParameter("X0",79400.0);
    fn->setParameter("S",8.0);

    fnWithBk->addFunction(fn);
    fnWithBk->addFunction(bk);

    //alg2.setFunction(fnWithBk);
    alg2.setPropertyValue("Function",*fnWithBk);

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )    
    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect


    std::string minimizer = alg2.getProperty("Minimizer");
    TS_ASSERT( minimizer.compare("Levenberg-Marquardt") == 0 );

    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 1.713,0.01);

    IFitFunction *out = FunctionFactory::Instance().createInitialized(alg2.getPropertyValue("Function")); 
    CompositeFunctionMW *pk = dynamic_cast<CompositeFunctionMW *>(out);

    TS_ASSERT_DELTA( out->getParameter("f0.I"), 230.67 ,0.2);
    TS_ASSERT_DELTA( out->getParameter("f0.A"), 0.2522 ,0.01);
    TS_ASSERT_DELTA( out->getParameter("f0.B"), 0.0293 ,0.001);
    TS_ASSERT_DELTA( out->getParameter("f0.X0"), 79408.00 ,0.1);
    TS_ASSERT_DELTA( out->getParameter("f0.S"), 15.561 ,0.2);

    TS_ASSERT_DELTA( out->getParameter("f1.A0"), 6.8327 ,0.2);

    const double* xx = &ws2D->readX(0)[0];
    double *yy = new double[timechannels]; 
    pk->function(yy, xx, timechannels);

    // note that fitting a none-totally optimized IC to a Gaussian peak so 
    // not a perfect fit - but pretty ok result
    TS_ASSERT_DELTA( yy[9], 4.1138 ,0.1);
    TS_ASSERT_DELTA( yy[10], 4.6679 ,0.1);
    TS_ASSERT_DELTA( yy[11], 6.8471 ,0.1);
    TS_ASSERT_DELTA( yy[12], 13.7936, 0.1);
    TS_ASSERT_DELTA( yy[13], 31.4458 ,0.1);
    TS_ASSERT_DELTA( yy[14], 66.6687 ,0.1);
    TS_ASSERT_DELTA( yy[15], 120.9065 ,0.1);
    TS_ASSERT_DELTA( yy[16], 183.5634 ,0.1);
    TS_ASSERT_DELTA( yy[17], 234.3267 ,0.1);
    TS_ASSERT_DELTA( yy[18], 256.1503, 0.1);
    TS_ASSERT_DELTA( yy[19], 246.6795 ,0.1);
    TS_ASSERT_DELTA( yy[20], 216.7392 ,0.1);
    TS_ASSERT_DELTA( yy[21], 180.0203 ,0.1);
    TS_ASSERT_DELTA( yy[22], 145.4474, 0.1);
    TS_ASSERT_DELTA( yy[23], 116.3506 ,0.1);
    TS_ASSERT_DELTA( yy[24], 92.9199 ,0.1);

    delete[] yy;

    AnalysisDataService::Instance().remove(wsName);
  }

};

#endif /*BACKTOBACKEXPONENTIALTEST_H_*/
