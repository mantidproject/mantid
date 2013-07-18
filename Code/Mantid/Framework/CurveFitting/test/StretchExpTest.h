	#ifndef STRETCHEXPTEST_H_
	#define STRETCHEXPTEST_H_

	#include <cxxtest/TestSuite.h>

	#include "MantidCurveFitting/StretchExp.h"
	#include "MantidAPI/CompositeFunction.h"
	#include "MantidCurveFitting/LinearBackground.h"
	#include "MantidCurveFitting/BoundaryConstraint.h"
	#include "MantidCurveFitting/Fit.h"
	#include "MantidKernel/UnitFactory.h"
	#include "MantidAPI/AnalysisDataService.h"
	#include "MantidAPI/WorkspaceFactory.h"
	#include "MantidAPI/Algorithm.h"
	#include "MantidDataObjects/Workspace2D.h"
	#include "MantidKernel/Exception.h"
	#include "MantidAPI/FunctionFactory.h"
    #include "MantidAPI/FunctionDomain1D.h"
    #include "MantidAPI/FunctionValues.h"

	using namespace Mantid::Kernel;
	using namespace Mantid::API;
	using namespace Mantid::CurveFitting;
	using namespace Mantid::DataObjects;


class StretchExpTest_Jacobian: public Mantid::API::Jacobian
{
    std::vector<double> m_values;
public:
  StretchExpTest_Jacobian(){m_values.resize(3);}
  virtual void set(size_t, size_t iP, double value) {m_values[iP] = value;}
  virtual double get(size_t, size_t iP) {return m_values[iP];}
};

	class StretchExpTest : public CxxTest::TestSuite
	{
	public:

	  void getMockData(Mantid::MantidVec& y, Mantid::MantidVec& e)
	  {
		  //values extracted from y(x)=2*exp(-(x/4)^0.5)
		  y[0]= 2;
		  y[1]= 1.2130613;
		  y[2]= 0.98613738;
		  y[3]= 0.84124005;
		  y[4]= 0.73575888;
		  y[5]= 0.65384379;
		  y[6]= 0.58766531;
		  y[7]= 0.53273643;
		  y[8]= 0.48623347;
		  y[9]= 0.44626032;
		  y[10]= 0.41148132;
		  y[11]= 0.38092026;
		  y[12]= 0.35384241;
		  y[13]= 0.32968143;
		  y[14]= 0.30799199;
		  y[15]= 0.28841799;
		  y[16]= 0.27067057;
		  y[17]= 0.25451242;
		  y[18]= 0.2397465;
		  y[19]= 0.22620756;

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
		std::string wsName = "StretchExpMockData";
		int histogramNumber = 1;
		int timechannels = 20;
		Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
		Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
		//in this case, x-values are just the running index
		for (int i = 0; i < 20; i++) ws2D->dataX(0)[i] = 1.0*i + 0.00001;
		Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
		Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
		getMockData(y, e);

		//put this workspace in the data service
		TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

		// set up StretchExp fitting function
		StretchExp fn;
		fn.initialize();

		// get close to exact values with an initial guess
		fn.setParameter("Height",1.5);
		fn.setParameter("Lifetime",5.0);
		fn.setParameter("Stretching",0.4);

		//alg2.setFunction(fn);
    alg2.setPropertyValue("Function",fn.asString());


		// Set which spectrum to fit against and initial starting values
		alg2.setPropertyValue("InputWorkspace", wsName);
		alg2.setPropertyValue("WorkspaceIndex","0");
		alg2.setPropertyValue("StartX","0");
		alg2.setPropertyValue("EndX","19");

		// execute fit
	   TS_ASSERT_THROWS_NOTHING(
		  TS_ASSERT( alg2.execute() )
		)

		TS_ASSERT( alg2.isExecuted() );

		// test the output from fit is what you expect
		double dummy = alg2.getProperty("OutputChi2overDoF");
		TS_ASSERT_DELTA( dummy, 0.001,0.001);

		IFunction_sptr out = alg2.getProperty("Function");
		//golden standard y(x)=2*exp(-(x/4)^0.5)
		//allow for a 1% error in Height and Lifetime, and 10% error in the Stretching exponent
		TS_ASSERT_DELTA( out->getParameter("Height"), 2.0 ,0.02);
		TS_ASSERT_DELTA( out->getParameter("Lifetime"), 4.0 ,0.04);
		TS_ASSERT_DELTA( out->getParameter("Stretching"), 0.5 ,0.05);

    // check its categories
    const std::vector<std::string> categories = out->categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "General" );

		AnalysisDataService::Instance().remove(wsName);

	  }

      void test_derivative_at_0()
      {
          Mantid::API::FunctionDomain1DVector x(0);
          StretchExpTest_Jacobian jac;
          StretchExp fn;
          fn.initialize();
          fn.setParameter("Height",1.5);
          fn.setParameter("Lifetime",5.0);
          fn.setParameter("Stretching",0.4);
          fn.functionDeriv(x, jac);
          TS_ASSERT_EQUALS( jac.get(0,2), 0.0 );

          fn.setParameter("Stretching",0.0);
          fn.functionDeriv(x, jac);
          TS_ASSERT_EQUALS( jac.get(0,2), 0.0 );

          Mantid::API::FunctionDomain1DVector x1(0.001);
          fn.functionDeriv(x1, jac);
          TS_ASSERT_DIFFERS( jac.get(0,2), 0.0 );
          fn.setParameter("Stretching",0.4);
          fn.functionDeriv(x1, jac);
          TS_ASSERT_DIFFERS( jac.get(0,2), 0.0 );
      }

      void test_negative_x()
      {
          Mantid::API::FunctionDomain1DVector x(-0.001);
          Mantid::API::FunctionValues y(x);

          StretchExp fn;
          fn.initialize();
          fn.setParameter("Height",1.5);
          fn.setParameter("Lifetime",5.0);
          fn.setParameter("Stretching",0.4);
          TS_ASSERT_THROWS( fn.function(x,y), std::runtime_error );
      }


	};

	#endif /*STRETCHEXPTEST_H_*/
