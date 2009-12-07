#ifndef GAUSSIANTEST_H_
#define GAUSSIANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Gaussian.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/LinearBackground.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

// Algorithm to force Gaussian1D to be run by simplex algorithm
class SimplexGaussian : public Gaussian
{
public:
  virtual ~SimplexGaussian() {}

protected:
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData)
  {
    throw Exception::NotImplementedError("No derivative function provided");
  }
};


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


  // here we have an example where an upper constraint on Sigma <= 100 makes
  // the Gaussian fit below success. The starting value of Sigma is here 300. 
  // Note that the fit is equally successful if we had no constraint on Sigma
  // and used a starting of Sigma = 100.
  void testAgainstHRPD_DatasetWithConstraints()
  {
    // load dataset to test against
    std::string inputFile = "../../../../Test/Data/HRP38692.RAW";
    LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", inputFile);
    std::string outputSpace = "MAR_Dataset";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    loader.execute();


    Fit alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set which spectrum to fit against and initial starting values
    alg.setPropertyValue("InputWorkspace",outputSpace);
    alg.setPropertyValue("WorkspaceIndex","2");
    alg.setPropertyValue("StartX","79300");
    alg.setPropertyValue("EndX","79600");

    // create function you want to fit against
    CompositeFunction *fnWithBk = new CompositeFunction();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->getParameter("A0") = 0.0;
    bk->getParameter("A1") = 0.0;
    bk->removeActive(1);  
    //bk->removeActive(1);

    BoundaryConstraint* bc_b = new BoundaryConstraint("A0",0, 20.0);
    //bk->addConstraint(bc_b);

    // set up Lorentzian fitting function
    Gaussian* fn = new Gaussian();
    fn->initialize();

    fn->getParameter("Height") = 200.0;
    fn->getParameter("PeakCentre") = 79450.0;
    fn->getParameter("Sigma") = 300.0;

    // add constraint to function
    BoundaryConstraint* bc1 = new BoundaryConstraint("Height",100, 300.0);
    BoundaryConstraint* bc2 = new BoundaryConstraint("PeakCentre",79200, 79700.0);
    BoundaryConstraint* bc3 = new BoundaryConstraint("Sigma",20, 100.0);
    //fn->addConstraint(bc1);
    //fn->addConstraint(bc2);
    fn->addConstraint(bc3);

    fnWithBk->addFunction(bk);
    fnWithBk->addFunction(fn);

    alg.setFunction(fnWithBk);

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg.execute() )
    )
    TS_ASSERT( alg.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 5.1604,1);

    TS_ASSERT_DELTA( fn->height(), 232.1146 ,1);
    TS_ASSERT_DELTA( fn->centre(), 79430.1 ,1);
    TS_ASSERT_DELTA( fn->getParameter("Sigma"), 26.14 ,0.1);
    TS_ASSERT_DELTA( bk->getParameter("A0"), 8.0575 ,0.1);
    TS_ASSERT_DELTA( bk->getParameter("A1"), 0.0 ,0.01); 

    AnalysisDataService::Instance().remove(outputSpace);
  }

  void testAgainstMockData()
  {
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

    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // set up gaussian fitting function
    Gaussian* gaus = new Gaussian();
    gaus->initialize();
    gaus->setCentre(11.2);
    gaus->setHeight(100.7);
    gaus->setWidth(2.2);

    alg2.setFunction(gaus);

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
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

    AnalysisDataService::Instance().remove(wsName);
  }

  void testAgainstMockDataSimplex()
  {
    // create mock data to test against
    std::string wsName = "GaussMockDataSimplex";
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

    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // set up gaussian fitting function
    SimplexGaussian* gaus = new SimplexGaussian();
    gaus->initialize();
    gaus->setCentre(11.2);
    gaus->setHeight(100.7);
    gaus->setWidth(2.2);

    alg2.setFunction(gaus);

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
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


    TS_ASSERT_DELTA( gaus->height(), 97.7836 ,0.2);
    TS_ASSERT_DELTA( gaus->centre(), 11.2356 ,0.1);
    TS_ASSERT_DELTA( gaus->width(), 2.6240 ,0.1);

    AnalysisDataService::Instance().remove(wsName);
  }

  // here we have an example where an upper constraint on Sigma <= 100 makes
  // the Gaussian fit below success. The starting value of Sigma is here 300. 
  // Note that the fit is equally successful if we had no constraint on Sigma
  // and used a starting of Sigma = 100.
  // Note that the no constraint simplex with Sigma = 300 also does not locate
  // the correct minimum but not as badly as levenberg-marquardt
  void testAgainstHRPD_DatasetWithConstraintsSimplex()
  {
    // load dataset to test against
    std::string inputFile = "../../../../Test/Data/HRP38692.RAW";
    LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", inputFile);
    std::string outputSpace = "MAR_Dataset";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    loader.execute();


    Fit alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set which spectrum to fit against and initial starting values
    alg.setPropertyValue("InputWorkspace",outputSpace);
    alg.setPropertyValue("WorkspaceIndex","2");
    alg.setPropertyValue("StartX","79300");
    alg.setPropertyValue("EndX","79600");

    // create function you want to fit against
    CompositeFunction *fnWithBk = new CompositeFunction();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->getParameter("A0") = 0.0;
    bk->getParameter("A1") = 0.0;
    bk->removeActive(1);  
    //bk->removeActive(1);

    BoundaryConstraint* bc_b = new BoundaryConstraint("A0",0, 20.0);
    //bk->addConstraint(bc_b);

    // set up Lorentzian fitting function
    SimplexGaussian* fn = new SimplexGaussian();
    fn->initialize();

    fn->getParameter("Height") = 200.0;
    fn->getParameter("PeakCentre") = 79450.0;
    fn->getParameter("Sigma") = 300.0;

    // add constraint to function
    BoundaryConstraint* bc1 = new BoundaryConstraint("Height",100, 300.0);
    BoundaryConstraint* bc2 = new BoundaryConstraint("PeakCentre",79200, 79700.0);
    BoundaryConstraint* bc3 = new BoundaryConstraint("Sigma",20, 100.0);
    //fn->addConstraint(bc1);
    //fn->addConstraint(bc2);
    fn->addConstraint(bc3);

    fnWithBk->addFunction(bk);
    fnWithBk->addFunction(fn);

    alg.setFunction(fnWithBk);

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg.execute() )
    )
    TS_ASSERT( alg.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 5.1604,1);

    TS_ASSERT_DELTA( fn->height(), 232.1146 ,1);
    TS_ASSERT_DELTA( fn->centre(), 79430.1 ,1);
    TS_ASSERT_DELTA( fn->getParameter("Sigma"), 26.14 ,0.1);
    TS_ASSERT_DELTA( bk->getParameter("A0"), 8.0575 ,0.1);
    TS_ASSERT_DELTA( bk->getParameter("A1"), 0.0 ,0.01); 

    AnalysisDataService::Instance().remove(outputSpace);
  }


};

#endif /*GAUSSIANTEST_H_*/
