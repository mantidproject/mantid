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
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ConfigService.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/LevenbergMarquardtMDMinimizer.h"
#include "MantidCurveFitting/UserFunction.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"

using namespace Mantid;
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
  std::string name()const{return "SimplexGaussian";}

protected:
  void functionDerivMW(Jacobian* out, const double* xValues, const size_t nData)
  {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
    throw Exception::NotImplementedError("No derivative function provided");
  }
};

DECLARE_FUNCTION(SimplexGaussian);

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

    for (size_t i = 0; i <=19; i++)
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


	  // Data taken from the peak tested in workspace index 2 of HRP38692 
    void getHRP38692Peak2Data(Mantid::MantidVec& x, Mantid::MantidVec& y, Mantid::MantidVec& e)
	{
		// x-values in time-of-flight
		for(size_t i=0; i < 8; i++) x[i] =  79292.4375 + 7.875*double(i);
		for(size_t i=8; i < 41; i++) x[i] = 79347.625 + 8.0*(double(i)-8.0);

		// y-values
		y[0] = 7;
		y[1] = 8;
		y[2] = 4;
		y[3] = 9;
		y[4] = 4;
		y[5] = 10;
		y[6] = 10;
		y[7] = 5;
		y[8] = 8;
		y[9] = 7;
		y[10] = 10;
		y[11] = 18;
		y[12] = 30;
		y[13] = 71;
		y[14] = 105;
		y[15] = 167;
		y[16] = 266;
		y[17] = 271;
		y[18] = 239;
		y[19] = 221;
		y[20] = 179;
		y[21] = 133;
		y[22] = 126;
		y[23] = 88;
		y[24] = 85;
		y[25] = 52;
		y[26] = 37;
		y[27] = 51;
		y[28] = 32;
		y[29] = 31;
		y[30] = 17;
		y[31] = 21;
		y[32] = 15;
		y[33] = 13;
		y[34] = 12;
		y[35] = 12;
		y[36] = 10;
		y[37] = 7;
		y[38] = 5;
		y[39] = 9;
		y[40] = 6;

		// errors are the square root of the Y-value
		for (size_t i=0; i < 41; i++) e[i] = sqrt( y[i] );
	}

    void test_with_Levenberg_Marquardt()
    {
      API::FunctionDomain1D_sptr domain(new API::FunctionDomain1DVector( 79292.4, 79603.6, 41));
      API::FunctionValues mockData(*domain);
      UserFunction dataMaker;
      dataMaker.setAttributeValue("Formula","b+h*exp(-((x-c)/s)^2)");
      dataMaker.setParameter("b",0);
      dataMaker.setParameter("h",232.11);
      dataMaker.setParameter("c",79430.1);
      dataMaker.setParameter("s",26.14);
      dataMaker.function(*domain,mockData);

      API::FunctionValues_sptr values(new API::FunctionValues(*domain));
      values->setFitDataFromCalculated(mockData);
      values->setFitWeights(1.0);

      CompositeFunction_sptr fnWithBk( new CompositeFunction() );

      boost::shared_ptr<LinearBackground> bk( new LinearBackground() );
      bk->initialize();

      bk->setParameter("A0",0.0);
      bk->setParameter("A1",0.0);
      bk->tie("A1","0");

      // set up Gaussian fitting function
      boost::shared_ptr<Gaussian> fn( new Gaussian() );
      fn->initialize();
      fn->setParameter("PeakCentre",79450.0);
      fn->setParameter("Height",200.0);
      fn->setParameter("Sigma",300.0);
      BoundaryConstraint* bc = new BoundaryConstraint(fn.get(),"Sigma",20.0,100.0);
	    //bc->setPenaltyFactor(1000.001);
	    fn->addConstraint(bc);


      fnWithBk->addFunction(bk);
      fnWithBk->addFunction(fn);

      boost::shared_ptr<CostFuncLeastSquares> costFun(new CostFuncLeastSquares);
      costFun->setFittingFunction(fnWithBk,domain,values);
      //TS_ASSERT_EQUALS(costFun->nParams(),3);

      LevenbergMarquardtMDMinimizer s;
      s.initialize(costFun);
      TS_ASSERT(s.minimize());

      API::IFunction_sptr res = costFun->getFittingFunction();
      std::cerr << "result=" << s.getError() << std::endl;
      std::cerr << "cost=" << costFun->val() << std::endl;
      for(size_t i = 0; i < res->nParams(); ++i)
      {
        std::cerr << res->parameterName(i) << " = " << res->getParameter(i) << std::endl;
      }

    }

    // Also pick values taken from HRPD_for_UNIT_TESTING.xml
  // here we have an example where an upper constraint on Sigma <= 100 makes
  // the Gaussian fit below success. The starting value of Sigma is here 300. 
  // Note that the fit is equally successful if we had no constraint on Sigma
  // and used a starting of Sigma = 100.
  void testAgainstPeak2WithConstraints()
  {
	// create peak2 mock data to test against
    std::string wsName = "GaussHRP38692MockData";
    int histogramNumber = 1;
    int timechannels = 41;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
	Mantid::MantidVec& x = ws2D->dataX(0); // x-values (time-of-flight)
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getHRP38692Peak2Data(x, y, e);

	//put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

	// Initialise algorithm
    Fit alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // create function you want to fit against
    CompositeFunction_sptr fnWithBk( new CompositeFunction() );

    boost::shared_ptr<LinearBackground> bk( new LinearBackground() );
    bk->initialize();

    bk->setParameter("A0",0.0);
    bk->setParameter("A1",0.0);
    bk->tie("A1","0");

    // set up Gaussian fitting function
    boost::shared_ptr<Gaussian> fn( new Gaussian() );
    fn->initialize();
    fn->setParameter("PeakCentre",79450.0);
    fn->setParameter("Height",200.0);
    fn->setParameter("Sigma",300);
    BoundaryConstraint* bc = new BoundaryConstraint(fn.get(),"Sigma",20.0,100.0);
	  bc->setPenaltyFactor(1000.001);
	  fn->addConstraint(bc);

    fnWithBk->addFunction(bk);
    fnWithBk->addFunction(fn);

    alg.setProperty("Function",boost::dynamic_pointer_cast<IFunction>(fnWithBk));
    // Set which spectrum to fit against and initial starting values
    alg.setPropertyValue("InputWorkspace", wsName);
    alg.setPropertyValue("StartX","79300");
    alg.setPropertyValue("EndX","79600");


    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg.execute() )
    )
    TS_ASSERT( alg.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 5.2, 0.1);

    IFunction_sptr out = alg.getProperty("Function");
    IPeakFunction *pk = dynamic_cast<IPeakFunction *>(dynamic_cast<CompositeFunction*>(out.get())->getFunction(1).get());
    TS_ASSERT_DELTA( pk->height(), 232. ,1);
    TS_ASSERT_DELTA( pk->centre(), 79430.1 ,10);
    TS_ASSERT_DELTA( pk->getParameter("Sigma"), 26.0 ,0.1);
    TS_ASSERT_DELTA( out->getParameter("f0.A0"), 8.09 ,0.1);
    TS_ASSERT_DELTA( out->getParameter("f0.A1"), 0.0 ,0.01); 

	 AnalysisDataService::Instance().remove(wsName);

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

    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // set up gaussian fitting function
    Gaussian gaus;
    gaus.initialize();
    gaus.setCentre(11.2);
    gaus.setHeight(100.7);
    gaus.setFwhm(2.2);

    alg2.setPropertyValue("Function",gaus.asString());

    // Set which spectrum to fit against and initial starting values
    alg2.setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(ws2D) );
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","20");
    alg2.setPropertyValue("Minimizer","Levenberg-MarquardtMD");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )
    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 0.035,0.01);

    IFunction_sptr out = alg2.getProperty("Function");
    IPeakFunction *pk = dynamic_cast<IPeakFunction *>(out.get());
    TS_ASSERT_DELTA( pk->height(), 97.8036 ,0.0001);
    TS_ASSERT_DELTA( pk->centre(), 11.2356 ,0.0001);
    TS_ASSERT_DELTA( pk->fwhm(), 2.6237 ,0.0001);
  }


  void testAgainstMockDataSimplex2()
  {
    // create mock data to test against
    std::string wsName = "GaussMockDataSimplex2";
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
    Gaussian gaus;
    gaus.initialize();
    gaus.setCentre(11.2);
    gaus.setHeight(100.7);
    gaus.setFwhm(2.2);

    alg2.setPropertyValue("Function",gaus.asString());

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","20");
    alg2.setPropertyValue("Minimizer", "Simplex");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )
    TS_ASSERT( alg2.isExecuted() );

    std::string minimizer = alg2.getProperty("Minimizer");
    TS_ASSERT( minimizer.compare("Simplex") == 0 );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 0.035,0.01);

    IFunction_sptr out = alg2.getProperty("Function");
    IPeakFunction *pk = dynamic_cast<IPeakFunction *>(out.get());
    TS_ASSERT_DELTA( pk->height(), 97.8091 ,0.01);
    TS_ASSERT_DELTA( pk->centre(), 11.2356 ,0.001);
    TS_ASSERT_DELTA( pk->fwhm(), 2.6240 ,0.001);
    std::cerr << pk->height() << std::endl;

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
	// create peak2 mock data to test against
    std::string wsName = "GaussHRP38692MockData";
    int histogramNumber = 1;
    int timechannels = 41;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
	  Mantid::MantidVec& x = ws2D->dataX(0); // x-values (time-of-flight)
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getHRP38692Peak2Data(x, y, e);

    //This test will not make sense if the configuration peakRadius is not set correctly.
    const std::string priorRadius = ConfigService::Instance().getString("curvefitting.peakRadius");
    ConfigService::Instance().setString("curvefitting.peakRadius","5");

    Fit alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // create function you want to fit against
    CompositeFunction_sptr fnWithBk( new CompositeFunction() );

    boost::shared_ptr<LinearBackground> bk( new LinearBackground() );
    bk->initialize();

    bk->setParameter("A0",0.0);
    bk->setParameter("A1",0.0);
    bk->tie("A1","0");

    //BoundaryConstraint* bc_b = new BoundaryConstraint(bk,"A0",0, 20.0);
    //bk->addConstraint(bc_b);

    // set up Gaussian fitting function
    //SimplexGaussian* fn = new SimplexGaussian();
    boost::shared_ptr<Gaussian> fn( new Gaussian() );
    fn->initialize();

    fn->setParameter("Height",200.0);
    fn->setParameter("PeakCentre",79450.0);
    fn->setParameter("Sigma",10.0);

    // add constraint to function
    BoundaryConstraint* bc3 = new BoundaryConstraint(fn.get(),"Sigma",20, 100.0);
    fn->addConstraint(bc3);

    fnWithBk->addFunction(bk);
    fnWithBk->addFunction(fn);

    alg.setProperty("Function",boost::dynamic_pointer_cast<IFunction>(fnWithBk));

    // Set which spectrum to fit against and initial starting values
    alg.setProperty("InputWorkspace",ws);
    alg.setPropertyValue("StartX","79300");
    alg.setPropertyValue("EndX","79600");
    alg.setPropertyValue("Minimizer","Simplex");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg.execute() )
    )
    TS_ASSERT( alg.isExecuted() );

    std::string minimizer = alg.getProperty("Minimizer");
    TS_ASSERT( minimizer.compare("Simplex") == 0 );

    // test the output from fit is what you expect
    double dummy = alg.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 2.5911,1);

    IFunction_sptr fun = alg.getProperty("Function");
    TS_ASSERT(fun);
    
    IFunction_sptr out = alg.getProperty("Function");
    TS_ASSERT_DELTA( out->getParameter("f1.Height"), 232 ,1);
    TS_ASSERT_DELTA( out->getParameter("f1.PeakCentre"), 79430 ,1);
    TS_ASSERT_DELTA( out->getParameter("f1.Sigma"), 26.08 ,1);
    TS_ASSERT_DELTA( out->getParameter("f0.A0"), 8 ,1);
    TS_ASSERT_DELTA( out->getParameter("f0.A1"), 0.0 ,0.01); 

    AnalysisDataService::Instance().remove(wsName);
    // Be nice and set back to what it was before
    ConfigService::Instance().setString("curvefitting.peakRadius",priorRadius);
  }

  void testIntensity()
  {
      boost::shared_ptr<Gaussian> fn( new Gaussian() );
      fn->initialize();
      fn->setHeight(2.0);
      fn->setFwhm(0.125);
      fn->setCentre(-200.0);

      // Area under a gaussian is height * sigma * sqrt(2 * pi)
      TS_ASSERT_DELTA(fn->intensity(), 0.26611675485780654483, 1e-10);
  }

  void testSetIntensity()
  {
      boost::shared_ptr<Gaussian> fn( new Gaussian() );
      fn->initialize();
      fn->setHeight(2.0);
      fn->setFwhm(0.125);
      fn->setCentre(-200.0);

      TS_ASSERT_THROWS_NOTHING(fn->setIntensity(0.5));

      TS_ASSERT_DELTA(fn->intensity(), 0.5, 1e-10);

      // FWHM does not change
      TS_ASSERT_EQUALS(fn->fwhm(), 0.125);

      // Height changes
      TS_ASSERT_DELTA(fn->height(), 3.75774911479860533509, 1e-10);
  }

  void testSetIntensityDefault()
  {
      boost::shared_ptr<Gaussian> fn( new Gaussian() );
      fn->initialize();

      TS_ASSERT_EQUALS(fn->intensity(), 0.0);

      // This does not work, because fwhm is 0 and height is 0
      TS_ASSERT_THROWS(fn->setIntensity(20.0), std::invalid_argument);
      TS_ASSERT_EQUALS(fn->intensity(), 0.0);

      // Now, fwhm is not zero
      fn->setFwhm(0.02);

      TS_ASSERT_THROWS_NOTHING(fn->setIntensity(20.0));
      TS_ASSERT_DELTA(fn->intensity(), 20.0, 1e-10);
  }


};

#endif /*GAUSSIANTEST_H_*/
