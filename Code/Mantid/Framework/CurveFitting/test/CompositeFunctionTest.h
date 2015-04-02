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

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

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

DECLARE_FUNCTION(CurveFittingLinear)
DECLARE_FUNCTION(CurveFittingGauss)

class CompositeFunctionTest : public CxxTest::TestSuite
{
private:

  struct TestFunction
  {
    double operator()(double x,int)
    {
      double x1 = x-4;
      double x2 = x-6;
      return 1. + 0.1*x + std::exp(-0.5*(x1*x1)*2)+2*std::exp(-0.5*(x2*x2)*3);
    }
  };

  std::string m_preSetupPeakRadius;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CompositeFunctionTest *createSuite() { return new CompositeFunctionTest(); }
  static void destroySuite( CompositeFunctionTest *suite ) { delete suite; }

  CompositeFunctionTest()
  {
    m_preSetupPeakRadius = Mantid::Kernel::ConfigService::Instance().getString("curvefitting.peakRadius");
    Mantid::Kernel::ConfigService::Instance().setString("curvefitting.peakRadius","100");
    FrameworkManager::Instance();
  }

  ~CompositeFunctionTest()
  {
    Mantid::Kernel::ConfigService::Instance().setString("curvefitting.peakRadius", m_preSetupPeakRadius);
  }

  void testFit()
  {
    boost::shared_ptr<CompositeFunction> mfun( new CompositeFunction() );
    boost::shared_ptr<CurveFittingGauss> g1( new CurveFittingGauss() );
    boost::shared_ptr<CurveFittingGauss> g2( new CurveFittingGauss() );
    boost::shared_ptr<CurveFittingLinear> bk( new CurveFittingLinear() );

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(g2);

    bk->setParameter("a",0.8);
    //bk->setParameter("b",0.1);

    g1->setParameter("c",3.1);
    g1->setParameter("h",1.1);
    g1->setParameter("s",1.);

    g2->setParameter("c",7.1);
    g2->setParameter("h",1.1);
    g2->setParameter("s",1.);

    TS_ASSERT_EQUALS(mfun->nParams(),8);

    TS_ASSERT_EQUALS(mfun->getParameter(0),0.8);
    TS_ASSERT_EQUALS(mfun->getParameter(1),0.0);
    TS_ASSERT_EQUALS(mfun->getParameter(2),3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(3),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(4),1.0);
    TS_ASSERT_EQUALS(mfun->getParameter(5),7.1);
    TS_ASSERT_EQUALS(mfun->getParameter(6),1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(7),1.0);

    auto ws = WorkspaceCreationHelper::Create2DWorkspaceFromFunction(TestFunction(), 1, 0.0, 10.0, 0.1);
    WorkspaceCreationHelper::addNoise(ws,0.1);
    WorkspaceCreationHelper::storeWS("mfun",ws);

    IFunction_sptr out;

    Fit alg;
    alg.initialize();

    alg.setProperty("Function",boost::dynamic_pointer_cast<IFunction>(mfun));
    alg.setPropertyValue("InputWorkspace","mfun");
    alg.setPropertyValue("WorkspaceIndex","0");
    alg.setProperty("CreateOutput",true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    auto outWS = WorkspaceCreationHelper::getWS<MatrixWorkspace>("mfun_Workspace");

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

    auto outParams = WorkspaceCreationHelper::getWS<TableWorkspace>("mfun_Parameters");
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
    TS_ASSERT_DELTA(row.Double(1),2.13,0.3);

    row = outParams->getRow(5);
    TS_ASSERT_EQUALS(row.String(0),"f2.c");
    TS_ASSERT_DELTA(row.Double(1),6,0.2);

    row = outParams->getRow(6);
    TS_ASSERT_EQUALS(row.String(0),"f2.h");
    TS_ASSERT_DELTA(row.Double(1),2,0.2);

    row = outParams->getRow(7);
    TS_ASSERT_EQUALS(row.String(0),"f2.s");
    TS_ASSERT_DELTA(row.Double(1),3.0,0.2);
    
    WorkspaceCreationHelper::removeWS("mfun");
    WorkspaceCreationHelper::removeWS("mfun_0_Workspace");
    WorkspaceCreationHelper::removeWS("mfun_0_Parameters");

  }

  void test_with_Simplex()
  {
    std::vector<double> x(10),y(10);
    for(size_t i = 0; i < x.size(); ++i)
    {
      x[i] = 0.1 * double(i);
      y[i] = 3.3 * x[i] + 4.4;
    }
    FunctionDomain1D_sptr domain(new FunctionDomain1DVector(x));
    FunctionValues_sptr values(new FunctionValues(*domain));
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
      double t = 0.1 * double(i);
      x[i] = t;
      y[i] = 0.1 * t * t + 3.3 * t + 4.4;
    }
    FunctionDomain1D_sptr domain(new FunctionDomain1DVector(x));
    FunctionValues_sptr values(new FunctionValues(*domain));
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
    FunctionDomain1D_sptr domain(new FunctionDomain1DVector( 0.0, 10.0, 10));
    FunctionValues mockData(*domain);
    UserFunction dataMaker;
    dataMaker.setAttributeValue("Formula","a*x+b+c*x^2");
    dataMaker.setParameter("a",3.3);
    dataMaker.setParameter("b",4.4);
    dataMaker.setParameter("c",0.1);
    dataMaker.function(*domain,mockData);

    FunctionValues_sptr values(new FunctionValues(*domain));
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

};

#endif /*CURVEFITTING_COMPOSITEFUNCTIONTEST_H_*/
