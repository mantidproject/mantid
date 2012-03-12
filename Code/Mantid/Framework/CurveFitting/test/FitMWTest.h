#ifndef CURVEFITTING_FITMWTEST_H_
#define CURVEFITTING_FITMWTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/FakeObjects.h"

#include "MantidCurveFitting/FitMW.h"
#include "MantidCurveFitting/UserFunction.h"
#include "MantidCurveFitting/ExpDecay.h"

#include "MantidAPI/FrameworkManager.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class FitMWTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitMWTest *createSuite() { return new FitMWTest(); }
  static void destroySuite( FitMWTest *suite ) { delete suite; }
  
  FitMWTest()
  {
    // need to have DataObjects loaded
    FrameworkManager::Instance();
  }

  void test_exec_point_data()
  {
    MatrixWorkspace_sptr ws2(new WorkspaceTester);
    ws2->initialize(2,10,10);

    for(size_t is = 0; is < ws2->getNumberHistograms(); ++is)
    {
      Mantid::MantidVec& x = ws2->dataX(is);
      Mantid::MantidVec& y = ws2->dataY(is);
      Mantid::MantidVec& e = ws2->dataE(is);
      for(size_t i = 0; i < ws2->blocksize(); ++i)
      {
        x[i] = 0.1 * i;
        y[i] =  (10.0 + is) * exp( -(x[i])/ (0.5*(1 + is)) );
      }
    }

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height",1.);
    fun->setParameter("Lifetime",1.0);

    FitMW fit;
    fit.initialize();

    fit.setProperty("InputWorkspace",ws2);
    fit.setProperty("WorkspaceIndex",0);
    fit.setProperty("Function",fun);
    fit.setProperty("CreateOutput",true);

    fit.execute();

    TS_ASSERT(fit.isExecuted());

    TS_ASSERT_DELTA( fun->getParameter("Height"), 10.0, 1e-3);
    TS_ASSERT_DELTA( fun->getParameter("Lifetime"), 0.5, 1e-4);

    double chi2 = fit.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(chi2, 0.0, 1e-8);
    //TS_ASSERT_DIFFERS(chi2, 0.0);
    TS_ASSERT_EQUALS(fit.getPropertyValue("OutputStatus"), "success");

    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      API::AnalysisDataService::Instance().retrieve("Output_Workspace"));
    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(),3);
    API::Axis* axis = outWS->getAxis(1);
    TS_ASSERT(axis);
    TS_ASSERT(axis->isText());
    TS_ASSERT_EQUALS(axis->length(), 3);
    TS_ASSERT_EQUALS(axis->label(0), "Data");
    TS_ASSERT_EQUALS(axis->label(1), "Calc");
    TS_ASSERT_EQUALS(axis->label(2), "Diff");

    const Mantid::MantidVec& Data = outWS->readY(0);
    const Mantid::MantidVec& Calc = outWS->readY(1);
    const Mantid::MantidVec& Diff = outWS->readY(2);
    for(size_t i = 0; i < outWS->blocksize(); ++i)
    {
      TS_ASSERT_EQUALS(Data[i] - Calc[i], Diff[i]);
    }

    ITableWorkspace_sptr covar = boost::dynamic_pointer_cast<ITableWorkspace>(
      API::AnalysisDataService::Instance().retrieve("Output_NormalisedCovarianceMatrix"));

    TS_ASSERT(covar);
    TS_ASSERT_EQUALS(covar->columnCount(), 3);
    TS_ASSERT_EQUALS(covar->rowCount(), 2);
    TS_ASSERT_EQUALS(covar->String(0,0), "Height");
    TS_ASSERT_EQUALS(covar->String(1,0), "Lifetime");
    TS_ASSERT_EQUALS(covar->getColumn(0)->type(), "str");
    TS_ASSERT_EQUALS(covar->getColumn(0)->name(), "Name");
    TS_ASSERT_EQUALS(covar->getColumn(1)->type(), "double");
    TS_ASSERT_EQUALS(covar->getColumn(1)->name(), "Height");
    TS_ASSERT_EQUALS(covar->getColumn(2)->type(), "double");
    TS_ASSERT_EQUALS(covar->getColumn(2)->name(), "Lifetime");
    TS_ASSERT_EQUALS(covar->Double(0,1), 100.0);
    TS_ASSERT_EQUALS(covar->Double(1,2), 100.0);
    TS_ASSERT(fabs(covar->Double(0,2)) < 100.0);
    TS_ASSERT(fabs(covar->Double(0,2)) > 0.0);
    TS_ASSERT_EQUALS(covar->Double(0,2), covar->Double(1,1));

    TS_ASSERT_DIFFERS( fun->getError(0), 0.0 );
    TS_ASSERT_DIFFERS( fun->getError(1), 0.0 );

    ITableWorkspace_sptr params = boost::dynamic_pointer_cast<ITableWorkspace>(
      API::AnalysisDataService::Instance().retrieve("Output_Parameters"));

    TS_ASSERT(params);
    TS_ASSERT_EQUALS(params->columnCount(), 3);
    TS_ASSERT_EQUALS(params->rowCount(), 3);
    TS_ASSERT_EQUALS(params->String(0,0), "Height");
    TS_ASSERT_EQUALS(params->String(1,0), "Lifetime");
    TS_ASSERT_EQUALS(params->String(2,0), "Cost function value");
    TS_ASSERT_EQUALS(params->Double(0,1), fun->getParameter(0));
    TS_ASSERT_EQUALS(params->Double(1,1), fun->getParameter(1));
    TS_ASSERT_EQUALS(params->Double(2,1), chi2);
    TS_ASSERT_EQUALS(params->Double(0,2), fun->getError(0));
    TS_ASSERT_EQUALS(params->Double(1,2), fun->getError(1));
    TS_ASSERT_EQUALS(params->Double(2,2), 0.0);

    API::AnalysisDataService::Instance().clear();
    //--------------------------------------------------//

    FitMW fit1;
    fit1.initialize();

    fit1.setProperty("InputWorkspace",ws2);
    fit1.setProperty("WorkspaceIndex",1);
    fit1.setProperty("Function",fun);

    fit1.execute();

    TS_ASSERT(fit1.isExecuted());

    TS_ASSERT_DELTA( fun->getParameter("Height"), 11.0, 1e-3);
    TS_ASSERT_DELTA( fun->getParameter("Lifetime"), 1.0, 1e-4);

  }

  void test_exec_histogram_data()
  {
    MatrixWorkspace_sptr ws2(new WorkspaceTester);
    ws2->initialize(2,11,10);

    for(size_t is = 0; is < ws2->getNumberHistograms(); ++is)
    {
      Mantid::MantidVec& x = ws2->dataX(is);
      Mantid::MantidVec& y = ws2->dataY(is);
      Mantid::MantidVec& e = ws2->dataE(is);
      for(size_t i = 0; i < ws2->blocksize(); ++i)
      {
        x[i] = 0.1 * i;
        y[i] =  (10.0 + is) * exp( -(x[i] + 0.05)/ (0.5*(1 + is)) );
      }
      x.back() = x[x.size()-2] + 0.1;
    }

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height",1.);
    fun->setParameter("Lifetime",1.);

    FitMW fit;
    fit.initialize();

    fit.setProperty("InputWorkspace",ws2);
    fit.setProperty("WorkspaceIndex",0);
    fit.setProperty("Function",fun);

    fit.execute();

    TS_ASSERT(fit.isExecuted());

    TS_ASSERT_DELTA( fun->getParameter("Height"), 10.0, 1e-3);
    TS_ASSERT_DELTA( fun->getParameter("Lifetime"), 0.5, 1e-4);

    FitMW fit1;
    fit1.initialize();

    fit1.setProperty("InputWorkspace",ws2);
    fit1.setProperty("WorkspaceIndex",1);
    fit1.setProperty("Function",fun);

    fit1.execute();

    TS_ASSERT(fit1.isExecuted());

    TS_ASSERT_DELTA( fun->getParameter("Height"), 11.0, 1e-3);
    TS_ASSERT_DELTA( fun->getParameter("Lifetime"), 1.0, 1e-4);

  }


};

#endif /*CURVEFITTING_FITMWTEST_H_*/
