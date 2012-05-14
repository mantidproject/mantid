#ifndef CURVEFITTING_FITMWTEST_H_
#define CURVEFITTING_FITMWTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/FakeObjects.h"

#include "MantidCurveFitting/FitMW.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidCurveFitting/UserFunction.h"
#include "MantidCurveFitting/ExpDecay.h"
#include "MantidCurveFitting/SeqDomain.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/FunctionDomain1D.h"

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
      //Mantid::MantidVec& e = ws2->dataE(is);
      for(size_t i = 0; i < ws2->blocksize(); ++i)
      {
        x[i] = 0.1 * double(i);
        y[i] =  (10.0 + double(is)) * exp( -(x[i])/ (0.5*(1 + double(is))) );
      }
    }

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height",1.);
    fun->setParameter("Lifetime",1.0);

    Fit fit;
    fit.initialize();

    fit.setProperty("Function",fun);
    fit.setProperty("InputWorkspace",ws2);
    fit.setProperty("WorkspaceIndex",0);
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

    //Fit fit1;
    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
    Mantid::API::IAlgorithm& fit1 = *alg;
    fit1.initialize();

    fit1.setProperty("Function",fun);
    fit1.setProperty("InputWorkspace",ws2);
    fit1.setProperty("WorkspaceIndex",1);

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
      //Mantid::MantidVec& e = ws2->dataE(is);
      for(size_t i = 0; i < ws2->blocksize(); ++i)
      {
        x[i] = 0.1 * double(i);
        y[i] =  (10.0 + double(is)) * exp( -(x[i] + 0.05)/ (0.5*(1 + double(is))) );
      }
      x.back() = x[x.size()-2] + 0.1;
    }

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height",1.);
    fun->setParameter("Lifetime",1.);

    Fit fit;
    fit.initialize();

    fit.setProperty("Function",fun);
    fit.setProperty("InputWorkspace",ws2);
    fit.setProperty("WorkspaceIndex",0);

    fit.execute();

    TS_ASSERT(fit.isExecuted());

    TS_ASSERT_DELTA( fun->getParameter("Height"), 10.0, 1e-3);
    TS_ASSERT_DELTA( fun->getParameter("Lifetime"), 0.5, 1e-4);

    Fit fit1;
    fit1.initialize();

    fit1.setProperty("Function",fun);
    fit1.setProperty("InputWorkspace",ws2);
    fit1.setProperty("WorkspaceIndex",1);

    fit1.execute();

    TS_ASSERT(fit1.isExecuted());

    TS_ASSERT_DELTA( fun->getParameter("Height"), 11.0, 1e-3);
    TS_ASSERT_DELTA( fun->getParameter("Lifetime"), 1.0, 1e-4);

  }

  void test_create_SeqDomain()
  {
    MatrixWorkspace_sptr ws2(new WorkspaceTester);
    ws2->initialize(2,11,10);

    for(size_t is = 0; is < ws2->getNumberHistograms(); ++is)
    {
      Mantid::MantidVec& x = ws2->dataX(is);
      Mantid::MantidVec& y = ws2->dataY(is);
      //Mantid::MantidVec& e = ws2->dataE(is);
      for(size_t i = 0; i < ws2->blocksize(); ++i)
      {
        x[i] = 0.1 * double(i);
        if ( i < 3 )
        {
          y[i] =  1.0;
        }
        else if ( i < 6 )
        {
          y[i] =  2.0;
        }
        else if ( i < 9 )
        {
          y[i] =  3.0;
        }
        else
        {
          y[i] =  4.0;
        }
      }
      x.back() = x[x.size()-2] + 0.1;
    }

    FunctionDomain_sptr domain;
    IFunctionValues_sptr values;

    FitMW fitmw(FitMW::Sequential);
    fitmw.setWorkspace( ws2 );
    fitmw.setWorkspaceIndex( 0 );
    fitmw.setMaxSize(3);
    fitmw.createDomain( domain, values );

    SeqDomain* seq = dynamic_cast<SeqDomain*>(domain.get());
    TS_ASSERT( seq );
    TS_ASSERT_EQUALS(seq->getNDomains(), 4);
    TS_ASSERT_EQUALS(seq->size(), 10);

    FunctionDomain_sptr d;
    IFunctionValues_sptr v;
    seq->getDomainAndValues( 0, d, v );
    TS_ASSERT_EQUALS( d->size(), 3 );
    TS_ASSERT_EQUALS( v->size(), 3 );
    auto d1d = static_cast<Mantid::API::FunctionDomain1D*>(d.get());
    auto v1d = static_cast<Mantid::API::FunctionValues*>(v.get());
    TS_ASSERT( d1d );
    TS_ASSERT_DELTA( (*d1d)[0], 0.05, 1e-13 );
    TS_ASSERT_DELTA( (*d1d)[1], 0.15, 1e-13 );
    TS_ASSERT_DELTA( (*d1d)[2], 0.25, 1e-13 );
    TS_ASSERT_DELTA( v1d->getFitData(0), 1.0, 1e-13 );
    TS_ASSERT_DELTA( v1d->getFitData(1), 1.0, 1e-13 );
    TS_ASSERT_DELTA( v1d->getFitData(2), 1.0, 1e-13 );
    v.reset();
    seq->getDomainAndValues( 1, d, v );
    TS_ASSERT_EQUALS( d->size(), 3 );
    TS_ASSERT_EQUALS( v->size(), 3 );
    d1d = static_cast<Mantid::API::FunctionDomain1D*>(d.get());
    v1d = static_cast<Mantid::API::FunctionValues*>(v.get());
    TS_ASSERT( d1d );
    TS_ASSERT_DELTA( (*d1d)[0], 0.35, 1e-13 );
    TS_ASSERT_DELTA( (*d1d)[1], 0.45, 1e-13 );
    TS_ASSERT_DELTA( (*d1d)[2], 0.55, 1e-13 );
    TS_ASSERT_DELTA( v1d->getFitData(0), 2.0, 1e-13 );
    TS_ASSERT_DELTA( v1d->getFitData(1), 2.0, 1e-13 );
    TS_ASSERT_DELTA( v1d->getFitData(2), 2.0, 1e-13 );
    v.reset();
    seq->getDomainAndValues( 2, d, v );
    TS_ASSERT_EQUALS( d->size(), 3 );
    TS_ASSERT_EQUALS( v->size(), 3 );
    d1d = static_cast<Mantid::API::FunctionDomain1D*>(d.get());
    v1d = static_cast<Mantid::API::FunctionValues*>(v.get());
    TS_ASSERT( d1d );
    TS_ASSERT_DELTA( (*d1d)[0], 0.65, 1e-13 );
    TS_ASSERT_DELTA( (*d1d)[1], 0.75, 1e-13 );
    TS_ASSERT_DELTA( (*d1d)[2], 0.85, 1e-13 );
    TS_ASSERT_DELTA( v1d->getFitData(0), 3.0, 1e-13 );
    TS_ASSERT_DELTA( v1d->getFitData(1), 3.0, 1e-13 );
    TS_ASSERT_DELTA( v1d->getFitData(2), 3.0, 1e-13 );
    v.reset();
    seq->getDomainAndValues( 3, d, v );
    TS_ASSERT_EQUALS( d->size(), 1 );
    TS_ASSERT_EQUALS( v->size(), 1 );
    d1d = static_cast<Mantid::API::FunctionDomain1D*>(d.get());
    v1d = static_cast<Mantid::API::FunctionValues*>(v.get());
    TS_ASSERT( d1d );
    TS_ASSERT_DELTA( (*d1d)[0], 0.95, 1e-13 );
    TS_ASSERT_DELTA( v1d->getFitData(0), 4.0, 1e-13 );

  }

};

#endif /*CURVEFITTING_FITMWTEST_H_*/
