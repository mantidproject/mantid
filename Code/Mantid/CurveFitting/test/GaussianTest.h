#ifndef GAUSSIANTEST_H_
#define GAUSSIANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Gaussian.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::CurveFitting::Gaussian;
using Mantid::CurveFitting::Fit;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;


class GaussianTest : public CxxTest::TestSuite
{
public:

  void getMockData(Mantid::MantidVec& y, Mantid::MantidVec& e)
  {
    y[0] =   3.56811123;
    y[1] =   3.25921675;
    y[2] =   2.69444562;
    y[3] =   3.05054488;
    y[4] =   2.86077216;
    y[5] =   2.29916480;
    y[6] =   2.57468876;
    y[7] =   3.65843827;
    y[8] =  15.31622763;
    y[9] =  56.57989073;
    y[10] = 101.20662386;
    y[11] =  76.30364797;
    y[12] =  31.54892552;
    y[13] =   8.09166673;
    y[14] =   3.20615343;
    y[15] =   2.95246554;
    y[16] =   2.75421444;
    y[17] =   3.70180447;
    y[18] =   2.77832668;
    y[19] =   2.29507565;

    for (int i = 0; i <=19; i++)
      y[i] -= 2.8765;

    e[0] =   1.72776328;
    e[1] =   1.74157482;
    e[2] =   1.73451042;
    e[3] =   1.73348562;
    e[4] =   1.74405622;
    e[5] =   1.72626701;
    e[6] =   1.75911386;
    e[7] =   2.11866496;
    e[8] =   4.07631054;
    e[9] =   7.65159052;
    e[10] =  10.09984173;
    e[11] =   8.95849024;
    e[12] =   5.42231173;
    e[13] =   2.64064858;
    e[14] =   1.81697576;
    e[15] =   1.72347732;
    e[16] =   1.73406310;
    e[17] =   1.73116711;
    e[18] =   1.71790285;
    e[19] =   1.72734254;
  }

  void testAgainstMockData()
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "GaussMockData";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 20; i++) ws2D->dataX(0)[i] = i+1;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // set up gaussian fitting function
    Gaussian* gaus = new Gaussian();
    gaus->init();
    gaus->setCentre(11.2);
    gaus->setHeight(100.7);
    gaus->setWidth(2.2);

    //void setFunction(API::IFunction* fun);
    alg2.setFunction(gaus);


    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","1");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","20");

    // execute fit
   TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )

    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.0717,0.0001);


    TS_ASSERT_DELTA( gaus->height(), 97.804 ,0.001);
    TS_ASSERT_DELTA( gaus->centre(), 11.2356 ,0.0001);
    TS_ASSERT_DELTA( gaus->width(), 2.6237 ,0.0001);

  }

};

#endif /*GAUSSIANTEST_H_*/
