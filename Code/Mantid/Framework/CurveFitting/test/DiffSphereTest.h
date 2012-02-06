	#ifndef LOGNORMALTEST_H_
	#define LOGNORMALTEST_H_

	#include <cxxtest/TestSuite.h>

	#include "MantidCurveFitting/LogNormal.h"
	#include "MantidAPI/CompositeFunction.h"
	#include "MantidCurveFitting/LinearBackground.h"
	#include "MantidCurveFitting/BoundaryConstraint.h"
	#include "MantidCurveFitting/Fit.h"
	#include "MantidKernel/UnitFactory.h"
	#include "MantidAPI/AnalysisDataService.h"
	#include "MantidAPI/WorkspaceFactory.h"
	#include "MantidAPI/Algorithm.h"
	#include "MantidDataObjects/Workspace2D.h"
	#include "MantidDataHandling/LoadRaw.h"
	#include "MantidKernel/Exception.h"
	#include "MantidAPI/FunctionFactory.h"

	using namespace Mantid::Kernel;
	using namespace Mantid::API;
	using namespace Mantid::CurveFitting;
	using namespace Mantid::DataObjects;
	using namespace Mantid::DataHandling;


	class LogNormalTest : public CxxTest::TestSuite
	{
	public:

	  void getMockData(Mantid::MantidVec& y, Mantid::MantidVec& e)
	  {
		  //values extracted from y(x)=100/x*exp(-(log(x)-2.2)^2/(2*0.25^2))
		  y[0]= 0.0;
		  y[1]= 1.52798e-15;
		  y[2]= 6.4577135e-07;
		  y[3]= 0.0020337351;
		  y[4]= 0.12517292;
		  y[5]= 1.2282908;
		  y[6]= 4.3935083;
		  y[7]= 8.5229866;
		  y[8]= 11.127883;
		  y[9]= 11.110426;
		  y[10]= 9.1925694;
		  y[11]= 6.6457304;
		  y[12]= 4.353104;
		  y[13]= 2.6504159;
		  y[14]= 1.5279732;
		  y[15]= 0.84552286;
		  y[16]= 0.45371715;
		  y[17]= 0.23794487;
		  y[18]= 0.12268847;
		  y[19]= 0.0624878;

		for (int i = 0; i <=19; i++)
		{
			//estimate errors as ten percent of the "count number"
			const double cc = 0.1;
			e[i] = cc*y[i];
		}

	  }

	  void testAgainstMockData()
	  {
		Fit alg2;
		TS_ASSERT_THROWS_NOTHING(alg2.initialize());
		TS_ASSERT( alg2.isInitialized() );

		// create mock data to test against
		std::string wsName = "LogNormalMockData";
		int histogramNumber = 1;
		int timechannels = 20;
		Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
		Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
		//in this case, x-values are just the running index
		for (int i = 0; i < 20; i++) ws2D->dataX(0)[i] = 1.0*i;
		Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
		Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
		getMockData(y, e);

		//put this workspace in the data service
		TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

		// set up LogNormal fitting function
		LogNormal* fn = new LogNormal();
		fn->initialize();

		// get close to exact values. Otherwise algorithm fails to a local minimum
		fn->setParameter("Height",90.0);
		fn->setParameter("Location",2.0);
		fn->setParameter("Scale",0.20);

		/*Parameters for Height, Location and Scale can be estimated from:
		 * Let:	dx_i = (x_{i+1}-x_{i-1})/2
		 * 		P_i = y[i]
		 * Then:
		 * 	Location ~ \sum_i[ dx_i * P_i * ln(x[i]) ] / \sum_i[ dx_i * y[i] ]
		 * 	Scale^2  ~  \sum_i[ dx_i * P_i * (ln(x[i]))^2 ]  / \sum_i[ dx_i * y[i] ] - Location^2
		 * 	Height   ~ \sum_i[ dx_i * y[i] ] / ( Scale * sqrt(2*pi) )
		 * These formulas derived from the fact that  logNormal becomes Gaussian with change of variables z = ln(x), that is:
		 * 	H/x*exp(-(ln(x)-L)^2/(2*S^2))*dx == LN(x)*dx = exp(-(z-L)^2/(2*S^2))*dz == G(z)*dz
		 * 	Example: Location = \integral dz G(z) z = \integral dx LN(x) z = \integral dx LN(x) ln(x),		 *
 		 *  */

		//alg2.setFunction(fn);
		alg2.setPropertyValue("Function",*fn);


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
		double dummy = alg2.getProperty("OutputChi2overDoF");
		TS_ASSERT_DELTA( dummy, 0.001,0.001);

		IFitFunction *out = FunctionFactory::Instance().createInitialized(alg2.getPropertyValue("Function"));
		//golden standard y(x) = 100.0 / x * exp( -(log(x)-2.2)^2/(2*0.25^2) )
		TS_ASSERT_DELTA( out->getParameter("Height"), 100.0 ,0.1);
		TS_ASSERT_DELTA( out->getParameter("Location"), 2.2 ,0.1);
		TS_ASSERT_DELTA( out->getParameter("Scale"), 0.25 ,0.01);

		AnalysisDataService::Instance().remove(wsName);

	  }


	};

	#endif /*LOGNORMALTEST_H_*/
