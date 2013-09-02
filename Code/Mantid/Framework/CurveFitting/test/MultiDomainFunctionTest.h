#ifndef MULTIDOMAINFUNCTIONTEST_H_
#define MULTIDOMAINFUNCTIONTEST_H_

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/JointDomain.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidCurveFitting/LevenbergMarquardtMDMinimizer.h"
#include "MantidCurveFitting/Fit.h"

#include "MantidTestHelpers/FakeObjects.h"

#include <cxxtest/TestSuite.h>
#include <boost/make_shared.hpp>
#include <algorithm>
#include <iostream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;

class MultiDomainFunctionTest_Function: public virtual IFunction1D, public virtual ParamFunction
{
public:
  MultiDomainFunctionTest_Function():IFunction1D(),ParamFunction()
  {
    this->declareParameter("A",0);
    this->declareParameter("B",0);
  }
  virtual std::string name() const {return "MultiDomainFunctionTest_Function";}
protected:
  virtual void function1D(double* out, const double* xValues, const size_t nData)const
  {
    const double A = getParameter(0);
    const double B = getParameter(1);

    for(size_t i = 0; i < nData; ++i)
    {
      double x = xValues[i];
      out[i] = A + B * x;
    }
  }
  virtual void functionDeriv1D(Jacobian* out, const double* xValues, const size_t nData)
  {
    for(size_t i = 0; i < nData; ++i)
    {
      double x = xValues[i];
      out->set(i,1,x);
      out->set(i,0,1.0);
    }
  }
};

class MultiDomainFunctionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultiDomainFunctionTest *createSuite() { return new MultiDomainFunctionTest(); }
  static void destroySuite( MultiDomainFunctionTest *suite ) { delete suite; }

  MultiDomainFunctionTest()
  {
    FrameworkManager::Instance();
    multi = boost::make_shared<MultiDomainFunction>();
    multi->addFunction(boost::make_shared<MultiDomainFunctionTest_Function>());
    multi->addFunction(boost::make_shared<MultiDomainFunctionTest_Function>());
    multi->addFunction(boost::make_shared<MultiDomainFunctionTest_Function>());

    multi->getFunction(0)->setParameter("A",0);
    multi->getFunction(0)->setParameter("B",0);

    multi->getFunction(1)->setParameter("A",0);
    multi->getFunction(1)->setParameter("B",0);

    multi->getFunction(2)->setParameter("A",0);
    multi->getFunction(2)->setParameter("B",0);

    domain = boost::make_shared<JointDomain>();
    domain->addDomain(boost::make_shared<FunctionDomain1DVector>(0,1,9));
    domain->addDomain(boost::make_shared<FunctionDomain1DVector>(1,2,10));
    domain->addDomain(boost::make_shared<FunctionDomain1DVector>(2,3,11));

    const double A0 = 0, A1 = 1, A2 = 2;
    const double B0 = 1, B1 = 2, B2 = 3;

    ws1.reset(new WorkspaceTester);
    ws1->initialize(1,10,10);
    {
      Mantid::MantidVec& x = ws1->dataX(0);
      Mantid::MantidVec& y = ws1->dataY(0);
      //Mantid::MantidVec& e = ws1->dataE(0);
      for(size_t i = 0; i < ws1->blocksize(); ++i)
      {
        x[i] =  0.1 * double(i);
        y[i] =  A0 + A1 + A2 + (B0 + B1 + B2) * x[i];
      }
    }

    ws2.reset(new WorkspaceTester);
    ws2->initialize(1,10,10);
    {
      Mantid::MantidVec& x = ws2->dataX(0);
      Mantid::MantidVec& y = ws2->dataY(0);
      //Mantid::MantidVec& e = ws2->dataE(0);
      for(size_t i = 0; i < ws2->blocksize(); ++i)
      {
        x[i] = 1 + 0.1 * double(i);
        y[i] = A0 + A1 + (B0 + B1) * x[i];
      }
    }

    ws3.reset(new WorkspaceTester);
    ws3->initialize(1,10,10);
    {
      Mantid::MantidVec& x = ws3->dataX(0);
      Mantid::MantidVec& y = ws3->dataY(0);
      //Mantid::MantidVec& e = ws3->dataE(0);
      for(size_t i = 0; i < ws3->blocksize(); ++i)
      {
        x[i] = 2 + 0.1 * double(i);
        y[i] = A0 + A2 + (B0 + B2) * x[i];
      }
    }
  }

  void test_fit()
  {

    //system("pause");
    auto values = boost::make_shared<FunctionValues>(*domain);
    const double A0 = 0, A1 = 1, A2 = 2;
    const double B0 = 1, B1 = 2, B2 = 3;

    auto& d0 = static_cast<const FunctionDomain1D&>(domain->getDomain(0));
    for(size_t i = 0; i < d0.size(); ++i)
    {
      values->setFitData(i, A0 + A1 + A2 + (B0 + B1 + B2) * d0[i]);
    }

    auto& d1 = static_cast<const FunctionDomain1D&>(domain->getDomain(1));
    for(size_t i = 0; i < d1.size(); ++i)
    {
      values->setFitData(9 + i, A0 + A1 + (B0 + B1) * d1[i]);
    }

    auto& d2 = static_cast<const FunctionDomain1D&>(domain->getDomain(2));
    for(size_t i = 0; i < d2.size(); ++i)
    {
      values->setFitData(19 + i, A0 + A2 + (B0 + B2) * d2[i]);
    }
    values->setFitWeights(1);

    multi->clearDomainIndices();
    std::vector<size_t> ii(2);
    ii[0] = 0;
    ii[1] = 1;
    multi->setDomainIndices(1,ii);
    ii[0] = 0;
    ii[1] = 2;
    multi->setDomainIndices(2,ii);

    boost::shared_ptr<CostFuncLeastSquares> costFun(new CostFuncLeastSquares);
    costFun->setFittingFunction(multi,domain,values);
    TS_ASSERT_EQUALS(costFun->nParams(),6);

    LevenbergMarquardtMDMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());

    TS_ASSERT_EQUALS(s.getError(),"success");
    TS_ASSERT_DELTA(s.costFunctionVal(),0,1e-4);
    
    TS_ASSERT_DELTA(multi->getFunction(0)->getParameter("A"),0,1e-8);
    TS_ASSERT_DELTA(multi->getFunction(0)->getParameter("B"),1,1e-8);
    TS_ASSERT_DELTA(multi->getFunction(1)->getParameter("A"),1,1e-8);
    TS_ASSERT_DELTA(multi->getFunction(1)->getParameter("B"),2,1e-8);
    TS_ASSERT_DELTA(multi->getFunction(2)->getParameter("A"),2,1e-8);
    TS_ASSERT_DELTA(multi->getFunction(2)->getParameter("B"),3,1e-8);
  }

  void test_Fit_algorithm()
  {

    multi->getFunction(0)->setParameter("A",0);
    multi->getFunction(0)->setParameter("B",0);
    multi->getFunction(1)->setParameter("A",0);
    multi->getFunction(1)->setParameter("B",0);
    multi->getFunction(2)->setParameter("A",0);
    multi->getFunction(2)->setParameter("B",0);

    Fit fit;
    fit.initialize();
    fit.setProperty("Function",boost::dynamic_pointer_cast<IFunction>(multi));
    fit.setProperty("InputWorkspace",ws1);
    fit.setProperty("WorkspaceIndex",0);
    fit.setProperty("InputWorkspace_1",ws2);
    fit.setProperty("WorkspaceIndex_1",0);
    fit.setProperty("InputWorkspace_2",ws3);
    fit.setProperty("WorkspaceIndex_2",0);
    fit.execute();

    IFunction_sptr fun = fit.getProperty("Function");
    TS_ASSERT_DELTA(fun->getParameter("f0.A"),0,1e-8);
    TS_ASSERT_DELTA(fun->getParameter("f0.B"),1,1e-8);
    TS_ASSERT_DELTA(fun->getParameter("f1.A"),1,1e-8);
    TS_ASSERT_DELTA(fun->getParameter("f1.B"),2,1e-8);
    TS_ASSERT_DELTA(fun->getParameter("f2.A"),2,1e-8);
    TS_ASSERT_DELTA(fun->getParameter("f2.B"),3,1e-8);

  }

  void test_Fit_resetting_properties()
  {
    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Fit");
    Mantid::API::IAlgorithm& fit = *alg;
    fit.initialize();
    fit.setProperty("Function",boost::dynamic_pointer_cast<IFunction>(multi));
    fit.setProperty("InputWorkspace",ws1);
    fit.setProperty("WorkspaceIndex",0);
    fit.setProperty("InputWorkspace",ws2);
    fit.setProperty("WorkspaceIndex",1);
    fit.setProperty("InputWorkspace_1",ws2);
    fit.setProperty("InputWorkspace_1",ws1);
  }

private:
  boost::shared_ptr<MultiDomainFunction> multi;
  boost::shared_ptr<JointDomain> domain;
  MatrixWorkspace_sptr ws1,ws2,ws3;
};

#endif /*MULTIDOMAINFUNCTIONTEST_H_*/
