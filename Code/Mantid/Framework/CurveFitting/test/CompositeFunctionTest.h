#ifndef CURVEFITTING_COMPOSITEFUNCTIONTEST_H_
#define CURVEFITTING_COMPOSITEFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FrameworkManager.h"

#include "MantidCurveFitting/SimplexMinimizer.h"
#include "MantidCurveFitting/BFGS_Minimizer.h"
#include "MantidCurveFitting/LevenbergMarquardtMDMinimizer.h"
#include "MantidCurveFitting/UserFunction.h"
#include "MantidCurveFitting/ExpDecay.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidCurveFitting/GSLJacobian.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;

typedef Mantid::DataObjects::Workspace2D_sptr WS_type;
typedef Mantid::DataObjects::TableWorkspace_sptr TWS_type;

class CurveFittingGauss: public IPeakFunction
{
public:
  CurveFittingGauss()
  {
    declareParameter("c");
    declareParameter("h",1.);
    declareParameter("s",1.);
  }

  std::string name()const{return "CurveFittingGauss";}

  void functionLocal(double* out, const double* xValues, const size_t nData)const
  {
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(size_t i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      out[i] = h*exp(-0.5*x*x*w);
    }
  }
  void functionDerivLocal(Jacobian* out, const double* xValues, const size_t nData)
  {
    //throw Mantid::Kernel::Exception::NotImplementedError("");
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for(size_t i=0;i<nData;i++)
    {
      double x = xValues[i] - c;
      double e = exp(-0.5*x*x*w);
      out->set(i,0,x*h*e*w);
      out->set(i,1,e);
      out->set(i,2,-0.5*x*x*h*e);
    }
  }

  double centre()const
  {
    return getParameter(0);
  }

  double height()const
  {
    return getParameter(1);
  }

  double fwhm()const
  {
    return getParameter(2);
  }

  void setCentre(const double c)
  {
    setParameter(0,c);
  }
  void setHeight(const double h)
  {
    setParameter(1,h);
  }

  void setFwhm(const double w)
  {
    setParameter(2,w);
  }

};


class CurveFittingLinear: public ParamFunction, public IFunction1D
{
public:
  CurveFittingLinear()
  {
    declareParameter("a");
    declareParameter("b");
  }

  std::string name()const{return "CurveFittingLinear";}

  void function1D(double* out, const double* xValues, const size_t nData)const
  {
    double a = getParameter("a");
    double b = getParameter("b");
    for(size_t i=0;i<nData;i++)
    {
      out[i] = a + b * xValues[i];
    }
  }
  void functionDeriv1D(Jacobian* out, const double* xValues, const size_t nData)
  {
    //throw Mantid::Kernel::Exception::NotImplementedError("");
    for(size_t i=0;i<nData;i++)
    {
      out->set(i,0,1.);
      out->set(i,1,xValues[i]);
    }
  }

};

DECLARE_FUNCTION(CurveFittingLinear);
DECLARE_FUNCTION(CurveFittingGauss);

class CompositeFunctionTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CompositeFunctionTest *createSuite() { return new CompositeFunctionTest(); }
  static void destroySuite( CompositeFunctionTest *suite ) { delete suite; }

  CompositeFunctionTest()
  {
    Kernel::ConfigService::Instance().setString("curvefitting.peakRadius","100");
    FrameworkManager::Instance();
  }

  void testFit()
  {
    boost::shared_ptr<CompositeFunction> mfun( new CompositeFunction() );
    boost::shared_ptr<CurveFittingGauss> g1( new CurveFittingGauss() );
    boost::shared_ptr<CurveFittingGauss> g2( new CurveFittingGauss() );
    boost::shared_ptr<CurveFittingLinear> bk( new CurveFittingLinear() );

    mfun.addFunction(bk);
    mfun.addFunction(g1);
    mfun.addFunction(g2);

    bk->setParameter("a",0.8);
    //bk->setParameter("b",0.1);

    g1->setParameter("c",3.1);
    g1->setParameter("h",1.1);
    g1->setParameter("s",1.);

    g2->setParameter("c",7.1);
    g2->setParameter("h",1.1);
    g2->setParameter("s",1.);

    TS_ASSERT_EQUALS(mfun->nParams(),8);

    TS_ASSERT_EQUALS(mfun.getParameter(0),0.8);
    TS_ASSERT_EQUALS(mfun.getParameter(1),0.0);
    TS_ASSERT_EQUALS(mfun.getParameter(2),3.1);
    TS_ASSERT_EQUALS(mfun.getParameter(3),1.1);
    TS_ASSERT_EQUALS(mfun.getParameter(4),1.0);
    TS_ASSERT_EQUALS(mfun.getParameter(5),7.1);
    TS_ASSERT_EQUALS(mfun.getParameter(6),1.1);
    TS_ASSERT_EQUALS(mfun.getParameter(7),1.0);

    WS_type ws = mkWS(1,0,10,0.1);
    addNoise(ws,0.1);
    storeWS("mfun",ws);

    IFunction_sptr out;

    Fit alg;
    alg.initialize();

    alg.setProperty("Function",boost::dynamic_pointer_cast<IFunction>(mfun));
    alg.setPropertyValue("InputWorkspace","mfun");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setProperty("CreateOutput",true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    WS_type outWS = getWS("mfun_Workspace");

    const Mantid::MantidVec& Y00 = ws->readY(0);
    const Mantid::MantidVec& Y0 = outWS->readY(0);
    const Mantid::MantidVec& Y = outWS->readY(1);
    const Mantid::MantidVec& R = outWS->readY(2);
    for(size_t i=0;i<Y.size();i++)
    {
      TS_ASSERT_EQUALS(Y00[i],Y0[i]);
      TS_ASSERT_DELTA(Y0[i],Y[i],0.1);
      TS_ASSERT_DIFFERS(R[i],0);
    }
    TS_ASSERT_EQUALS( alg.getPropertyValue("OutputStatus"), "success");

    out = alg.getProperty("Function");

    TS_ASSERT_EQUALS(out->parameterName(0),"f0.a");
    TS_ASSERT_DELTA(out->getParameter(0),0.9956,0.1);

    TS_ASSERT_EQUALS(out->parameterName(1),"f0.b");
    TS_ASSERT_DELTA(out->getParameter(1),0.1002,0.1);

    TS_ASSERT_EQUALS(out->parameterName(2),"f1.c");
    TS_ASSERT_DELTA(out->getParameter(2),3.9887,0.1);

    TS_ASSERT_EQUALS(out->parameterName(3),"f1.h");
    TS_ASSERT_DELTA(out->getParameter(3),1.0192,0.1);

    TS_ASSERT_EQUALS(out->parameterName(4),"f1.s");
    TS_ASSERT_DELTA(out->getParameter(4),2.1341,0.3);

    TS_ASSERT_EQUALS(out->parameterName(5),"f2.c");
    TS_ASSERT_DELTA(out->getParameter(5),6,0.2);

    TS_ASSERT_EQUALS(out->parameterName(6),"f2.h");
    TS_ASSERT_DELTA(out->getParameter(6),1.9823,0.1);

    TS_ASSERT_EQUALS(out->parameterName(7),"f2.s");
    TS_ASSERT_DELTA(out->getParameter(7),2.8530,0.3);

    TWS_type outParams = getTWS("mfun_Parameters");
    TS_ASSERT(outParams);

    TS_ASSERT_EQUALS(outParams->rowCount(),9);
    TS_ASSERT_EQUALS(outParams->columnCount(),3);

    TableRow row = outParams->getFirstRow();
    TS_ASSERT_EQUALS(row.String(0),"f0.a");
    TS_ASSERT_DELTA(row.Double(1),1,0.1);

    row = outParams->getRow(1);
    TS_ASSERT_EQUALS(row.String(0),"f0.b");
    TS_ASSERT_DELTA(row.Double(1),0.1,0.1);

    row = outParams->getRow(2);
    TS_ASSERT_EQUALS(row.String(0),"f1.c");
    TS_ASSERT_DELTA(row.Double(1),4,0.2);

    row = outParams->getRow(3);
    TS_ASSERT_EQUALS(row.String(0),"f1.h");
    TS_ASSERT_DELTA(row.Double(1),1,0.2);

    row = outParams->getRow(4);
    TS_ASSERT_EQUALS(row.String(0),"f1.s");
    TS_ASSERT_DELTA(row.Double(1),2.13,0.2);

    row = outParams->getRow(5);
    TS_ASSERT_EQUALS(row.String(0),"f2.c");
    TS_ASSERT_DELTA(row.Double(1),6,0.2);

    row = outParams->getRow(6);
    TS_ASSERT_EQUALS(row.String(0),"f2.h");
    TS_ASSERT_DELTA(row.Double(1),2,0.2);

    row = outParams->getRow(7);
    TS_ASSERT_EQUALS(row.String(0),"f2.s");
    TS_ASSERT_DELTA(row.Double(1),3.0,0.2);
    
    removeWS("mfun");
    removeWS("mfun_0_Workspace");
    removeWS("mfun_0_Parameters");

  }

  void test_with_Simplex()
  {
    std::vector<double> x(10),y(10);
    for(size_t i = 0; i < x.size(); ++i)
    {
      x[i] = 0.1 * i;
      y[i] = 3.3 * x[i] + 4.4;
    }
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1D(x));
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitData(y);
    values->setFitWeights(1.0);

    boost::shared_ptr<CompositeFunction> mfun(new CompositeFunction);

    boost::shared_ptr<UserFunction> fun1(new UserFunction);
    fun1->setAttributeValue("Formula","a*x");
    fun1->setParameter("a",1.1);

    boost::shared_ptr<UserFunction> fun2(new UserFunction);
    fun2->setAttributeValue("Formula","0*x + b");
    fun2->setParameter("b",2.2);

    mfun->addFunction(fun1);
    mfun->addFunction(fun2);

    boost::shared_ptr<CostFuncLeastSquares> costFun(new CostFuncLeastSquares);
    costFun->setFittingFunction(mfun,domain,values);

    SimplexMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(),0.0,0.0001);
    TS_ASSERT_DELTA(mfun->getParameter("f0.a"),3.3,0.01);
    TS_ASSERT_DELTA(mfun->getParameter("f1.b"),4.4,0.01);
    TS_ASSERT_EQUALS(s.getError(),"success");
  }

  void test_with_BFGS()
  {
    std::vector<double> x(10),y(10);
    for(size_t i = 0; i < x.size(); ++i)
    {
      double t = 0.1 * i;
      x[i] = t;
      y[i] = 0.1 * t * t + 3.3 * t + 4.4;
    }
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1D(x));
    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitData(y);
    values->setFitWeights(1.0);

    boost::shared_ptr<CompositeFunction> mfun(new CompositeFunction);

    boost::shared_ptr<UserFunction> fun1(new UserFunction);
    fun1->setAttributeValue("Formula","a*x");
    fun1->setParameter("a",1.1);

    boost::shared_ptr<UserFunction> fun2(new UserFunction);
    fun2->setAttributeValue("Formula","c*x^2 + b");
    fun2->setParameter("c",0.00);
    fun2->setParameter("b",2.2);

    mfun->addFunction(fun1);
    mfun->addFunction(fun2);

    //CurveFitting::GSLJacobian J(mfun, values->size());
    //mfun->functionDeriv(*domain,J);
    //for(size_t i = 0; i < values->size(); ++i)
    //{
    //  std::cerr << (*domain)[i] << "   " << J.get(i,0) << ' ' << J.get(i,1) << ' ' << J.get(i,2) << std::endl;
    //}

    boost::shared_ptr<CostFuncLeastSquares> costFun(new CostFuncLeastSquares);
    costFun->setFittingFunction(mfun,domain,values);

    BFGS_Minimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(),0.0,0.0001);
    TS_ASSERT_DELTA(mfun->getParameter("f0.a"),3.3,0.01);
    TS_ASSERT_DELTA(mfun->getParameter("f1.c"),0.1,0.01);
    TS_ASSERT_DELTA(mfun->getParameter("f1.b"),4.4,0.01);
    TS_ASSERT_EQUALS(s.getError(),"success");
  }

  void test_with_LM()
  {
    API::FunctionDomain1D_sptr domain(new API::FunctionDomain1D( 0.0, 10.0, 10));
    API::FunctionValues mockData(*domain);
    UserFunction dataMaker;
    dataMaker.setAttributeValue("Formula","a*x+b+c*x^2");
    dataMaker.setParameter("a",3.3);
    dataMaker.setParameter("b",4.4);
    dataMaker.setParameter("c",0.1);
    dataMaker.function(*domain,mockData);

    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitDataFromCalculated(mockData);
    values->setFitWeights(1.0);

    boost::shared_ptr<CompositeFunction> mfun(new CompositeFunction);

    boost::shared_ptr<UserFunction> fun1(new UserFunction);
    fun1->setAttributeValue("Formula","a*x");
    fun1->setParameter("a",1.1);

    boost::shared_ptr<UserFunction> fun2(new UserFunction);
    fun2->setAttributeValue("Formula","c*x^2 + b");
    fun2->setParameter("c",0.00);
    fun2->setParameter("b",2.2);

    mfun->addFunction(fun1);
    mfun->addFunction(fun2);

    boost::shared_ptr<CostFuncLeastSquares> costFun(new CostFuncLeastSquares);
    costFun->setFittingFunction(mfun,domain,values);

    LevenbergMarquardtMDMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());
    TS_ASSERT_DELTA(costFun->val(),0.0,0.0001);
    TS_ASSERT_DELTA(mfun->getParameter("f0.a"),3.3,0.01);
    TS_ASSERT_DELTA(mfun->getParameter("f1.c"),0.1,0.01);
    TS_ASSERT_DELTA(mfun->getParameter("f1.b"),4.4,0.01);
    TS_ASSERT_EQUALS(s.getError(),"success");
  }

private:
  WS_type mkWS(int nSpec,double x0,double x1,double dx,bool isHist=false)
  {
    int nX = int((x1 - x0)/dx) + 1;
    int nY = nX - (isHist?1:0);
    if (nY <= 0)
      throw std::invalid_argument("Cannot create an empty workspace");

    Mantid::DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>
      (WorkspaceFactory::Instance().create("Workspace2D",nSpec,nX,nY));

    double x;

    for(int iSpec=0;iSpec<nSpec;iSpec++)
    {
      Mantid::MantidVec& X = ws->dataX(iSpec);
      Mantid::MantidVec& Y = ws->dataY(iSpec);
      Mantid::MantidVec& E = ws->dataE(iSpec);
      for(int i=0;i<nY;i++)
      {
        x = x0 + dx*i;
        X[i] = x;
        double x1 = x-4;
        double x2 = x-6;
        Y[i] = 1. + 0.1*x + exp(-0.5*(x1*x1)*2)+2*exp(-0.5*(x2*x2)*3);
        E[i] = 1;
      }
      if (isHist)
        X.back() = X[nY-1] + dx;
    }
    return ws;
  }

  void storeWS(const std::string& name,WS_type ws)
  {
    AnalysisDataService::Instance().add(name,ws);
  }

  void removeWS(const std::string& name)
  {
    AnalysisDataService::Instance().remove(name);
  }

  WS_type getWS(const std::string& name)
  {
    return AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::Workspace2D>(name);
  }

  TWS_type getTWS(const std::string& name)
  {
    return AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>(name);
  }

  void addNoise(WS_type ws,double noise)
  {
    for(size_t iSpec=0;iSpec<ws->getNumberHistograms();iSpec++)
    {
      Mantid::MantidVec& Y = ws->dataY(iSpec);
      Mantid::MantidVec& E = ws->dataE(iSpec);
      for(size_t i=0;i<Y.size();i++)
      {
        Y[i] += noise*(-.5 + double(rand())/RAND_MAX);
        E[i] += noise;
      }
    }
  }

  void interrupt()
  {
    int iii;
    std::cerr<<"Enter a number:";
    std::cin>>iii;
  }
};

#endif /*CURVEFITTING_COMPOSITEFUNCTIONTEST_H_*/
