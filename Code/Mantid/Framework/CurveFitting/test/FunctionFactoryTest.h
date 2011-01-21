#ifndef FUNCTIONFACTORYTEST_H_
#define FUNCTIONFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/CompositeFunctionMW.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IConstraint.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::API;

class FunctionFactoryTest_FunctA: public ParamFunction, public IFunctionMW
{
  int m_attr;
public:
  FunctionFactoryTest_FunctA()
  {
    declareParameter("a0");
    declareParameter("a1");
  }
  std::string name()const{return "FunctionFactoryTest_FunctA";}
  void function(double* out, const double* xValues, const int& nData)const{}
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData){}
  bool hasAttribute(const std::string& attName)const
  {
    if (attName == "attr") return true;
    return false;
  }
  Attribute getAttribute(const std::string& attName)const
  {
    if (attName == "attr") return Attribute(m_attr);
    return getAttribute(attName);
  }
  void setAttribute(const std::string& attName,const Attribute& value)
  {
    if (attName == "attr")
    {
      int n = value.asInt();
      if (n > 0)
      {
        m_attr = n;
        clearAllParameters();
        for(int i=0;i<n;i++)
        {
          std::ostringstream ostr;
          ostr << "at_" << i;
          declareParameter(ostr.str());
        }
      }
    }
    else
    {
      setAttribute(attName,value);
    }
  }
};

class FunctionFactoryTest_FunctB: public ParamFunction, public IFunctionMW
{
public:
  FunctionFactoryTest_FunctB()
  {
    declareParameter("b0");
    declareParameter("b1");
  }

  std::string name()const{return "FunctionFactoryTest_FunctB";}

  void function(double* out, const double* xValues, const int& nData)const
  {
  }
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData)
  {
  }
};

class FunctionFactoryTest_CompFunctA: public CompositeFunctionMW
{
  std::string m_attr;
public:
  FunctionFactoryTest_CompFunctA(){}

  std::string name()const{return "FunctionFactoryTest_CompFunctA";}

  void function(double* out, const double* xValues, const int& nData)const{}
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData){}
  bool hasAttribute(const std::string& attName)const
  {
    if (attName == "attr") return true;
    return false;
  }
  Attribute getAttribute(const std::string& attName)const
  {
    if (attName == "attr") return Attribute(m_attr);
    return getAttribute(attName);
  }
  void setAttribute(const std::string& attName,const Attribute& value)
  {
    m_attr = value.asString();
  }
};

class FunctionFactoryTest_CompFunctB: public CompositeFunctionMW
{
public:
  FunctionFactoryTest_CompFunctB(){}

  std::string name()const{return "FunctionFactoryTest_CompFunctB";}

  void function(double* out, const double* xValues, const int& nData)const
  {
  }
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData)
  {
  }
};

DECLARE_FUNCTION(FunctionFactoryTest_FunctA);
DECLARE_FUNCTION(FunctionFactoryTest_FunctB);
DECLARE_FUNCTION(FunctionFactoryTest_CompFunctA);
DECLARE_FUNCTION(FunctionFactoryTest_CompFunctB);

class FunctionFactoryTest : public CxxTest::TestSuite
{
public:
  FunctionFactoryTest()
  {
    Mantid::API::FrameworkManager::Instance();
  }

  void testCreateFunction()
  {
    IFitFunction* funa = FunctionFactory::Instance().createFunction("FunctionFactoryTest_FunctA");
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);

    IFitFunction* funb = FunctionFactory::Instance().createFunction("FunctionFactoryTest_FunctB");
    TS_ASSERT(funb);
    TS_ASSERT_EQUALS(funb->parameterName(0),"b0");
    TS_ASSERT_EQUALS(funb->parameterName(1),"b1");
    TS_ASSERT_EQUALS(funb->nParams(),2);
    delete funa;
    delete funb;
  }

  void testCreateSimpleDefault()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA";
    IFitFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);
    delete funa;
  }

  void testCreateSimple()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1";
    IFitFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);
    delete funa;
  }

  void testCreateSimpleWithAttribute()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,attr=\"3\",at_0=0.1,at_1=1.1,at_2=2.1";
    IFitFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"at_0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"at_1");
    TS_ASSERT_EQUALS(funa->parameterName(2),"at_2");
    TS_ASSERT_EQUALS(funa->nParams(),3);
    TS_ASSERT_EQUALS(funa->getParameter(0),0.1);
    TS_ASSERT_EQUALS(funa->getParameter(1),1.1);
    TS_ASSERT_EQUALS(funa->getParameter(2),2.1);
    delete funa;
  }

  void testCreateComposite()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1;name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2";

    IFitFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    CompositeFunction* cf = dynamic_cast<CompositeFunction*>(fun);
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(cf->nParams(),4);
    TS_ASSERT_EQUALS(cf->parameterName(0),"f0.a0");
    TS_ASSERT_EQUALS(cf->parameterName(1),"f0.a1");
    TS_ASSERT_EQUALS(cf->parameterName(2),"f1.b0");
    TS_ASSERT_EQUALS(cf->parameterName(3),"f1.b1");
    TS_ASSERT_EQUALS(cf->getParameter(0),0.1);
    TS_ASSERT_EQUALS(cf->getParameter(1),1.1);
    TS_ASSERT_EQUALS(cf->getParameter(2),0.2);
    TS_ASSERT_EQUALS(cf->getParameter(3),1.2);
    delete fun;
  }

  void testCreateComposite1()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2";

    IFitFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    CompositeFunction* cf = dynamic_cast<CompositeFunction*>(fun);
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(cf->nParams(),4);
    TS_ASSERT_EQUALS(cf->parameterName(0),"f0.a0");
    TS_ASSERT_EQUALS(cf->parameterName(1),"f0.a1");
    TS_ASSERT_EQUALS(cf->parameterName(2),"f1.b0");
    TS_ASSERT_EQUALS(cf->parameterName(3),"f1.b1");
    TS_ASSERT_EQUALS(cf->getParameter(0),0.);
    TS_ASSERT_EQUALS(cf->getParameter(1),0.);
    TS_ASSERT_EQUALS(cf->getParameter(2),0.2);
    TS_ASSERT_EQUALS(cf->getParameter(3),1.2);
    delete fun;
  }

  void testCreateComposite2()
  {
    std::string fnString = "composite=FunctionFactoryTest_CompFunctB;";
    fnString += "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2";

    IFitFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    FunctionFactoryTest_CompFunctB* cf = dynamic_cast<FunctionFactoryTest_CompFunctB*>(fun);
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(cf->nParams(),4);
    TS_ASSERT_EQUALS(cf->parameterName(0),"f0.a0");
    TS_ASSERT_EQUALS(cf->parameterName(1),"f0.a1");
    TS_ASSERT_EQUALS(cf->parameterName(2),"f1.b0");
    TS_ASSERT_EQUALS(cf->parameterName(3),"f1.b1");
    TS_ASSERT_EQUALS(cf->getParameter(0),0.);
    TS_ASSERT_EQUALS(cf->getParameter(1),0.);
    TS_ASSERT_EQUALS(cf->getParameter(2),0.2);
    TS_ASSERT_EQUALS(cf->getParameter(3),1.2);
    TS_ASSERT_EQUALS(fun->name(),"FunctionFactoryTest_CompFunctB");
    delete fun;
  }

  void testCreateComposite3()
  {
    std::string fnString = "composite=FunctionFactoryTest_CompFunctA,attr = \"hello\";";
    fnString += "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2";

    IFitFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    FunctionFactoryTest_CompFunctA* cf = dynamic_cast<FunctionFactoryTest_CompFunctA*>(fun);
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(cf->nParams(),4);
    TS_ASSERT_EQUALS(cf->parameterName(0),"f0.a0");
    TS_ASSERT_EQUALS(cf->parameterName(1),"f0.a1");
    TS_ASSERT_EQUALS(cf->parameterName(2),"f1.b0");
    TS_ASSERT_EQUALS(cf->parameterName(3),"f1.b1");
    TS_ASSERT_EQUALS(cf->getParameter(0),0.);
    TS_ASSERT_EQUALS(cf->getParameter(1),0.);
    TS_ASSERT_EQUALS(cf->getParameter(2),0.2);
    TS_ASSERT_EQUALS(cf->getParameter(3),1.2);
    TS_ASSERT_EQUALS(fun->name(),"FunctionFactoryTest_CompFunctA");
    TS_ASSERT(fun->hasAttribute("attr"));
    TS_ASSERT_EQUALS(fun->getAttribute("attr").asString(),"hello");
    delete fun;
  }

  void testCreateCompositeNested()
  {
    std::string fnString = "(composite=FunctionFactoryTest_CompFunctA,attr = hello;";
    fnString += "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2);";
    fnString += "(composite=FunctionFactoryTest_CompFunctB;";
    fnString += "name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2;name=FunctionFactoryTest_FunctA)";

    IFitFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);

    CompositeFunction* cf = dynamic_cast<CompositeFunction*>(fun);
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(cf->nFunctions(),2);
    TS_ASSERT_EQUALS(cf->getFunction(0)->name(),"FunctionFactoryTest_CompFunctA");
    TS_ASSERT_EQUALS(cf->getFunction(1)->name(),"FunctionFactoryTest_CompFunctB");
    TS_ASSERT_EQUALS(dynamic_cast<CompositeFunction*>(cf->getFunction(0))->nFunctions(),2);
    TS_ASSERT_EQUALS(dynamic_cast<CompositeFunction*>(cf->getFunction(1))->nFunctions(),2);
    delete fun;
  }
  
  void testCreateWithConstraint1()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1,constraint=0<a0<0.2";
    IFitFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);

    IConstraint* c = funa->getConstraint(0);
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(),0);

    funa->setParameter("a0",1);
    TS_ASSERT_EQUALS(c->check(),800);

    funa->setParameter("a0",-1);
    TS_ASSERT_EQUALS(c->check(),1000);
    delete funa;

  }

  void testCreateWithConstraint2()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1,"
      "constraints=(0<a0<0.2,a1>10)";
    IFitFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);

    IConstraint* c0 = funa->getConstraint(0);
    TS_ASSERT(c0);
    TS_ASSERT_EQUALS(c0->check(),0);

    funa->setParameter("a0",1);
    TS_ASSERT_EQUALS(c0->check(),800);

    funa->setParameter("a0",-1);
    TS_ASSERT_EQUALS(c0->check(),1000);

    IConstraint* c1 = funa->getConstraint(1);
    TS_ASSERT(c1);
    TS_ASSERT_EQUALS(c1->check(),8900);

    funa->setParameter("a1",11);
    TS_ASSERT_EQUALS(c1->check(),0);

    delete funa;

  }

  void testCreateCompositeWithConstraints()
  {
    std::string fnString = "composite=FunctionFactoryTest_CompFunctA,attr = \"hello\";"
      "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2,"
      "constraints=(b0<1,b1>1)";

    IFitFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    FunctionFactoryTest_CompFunctA* cf = dynamic_cast<FunctionFactoryTest_CompFunctA*>(fun);
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(cf->nParams(),4);
    TS_ASSERT_EQUALS(cf->parameterName(0),"f0.a0");
    TS_ASSERT_EQUALS(cf->parameterName(1),"f0.a1");
    TS_ASSERT_EQUALS(cf->parameterName(2),"f1.b0");
    TS_ASSERT_EQUALS(cf->parameterName(3),"f1.b1");
    TS_ASSERT_EQUALS(cf->getParameter(0),0.);
    TS_ASSERT_EQUALS(cf->getParameter(1),0.);
    TS_ASSERT_EQUALS(cf->getParameter(2),0.2);
    TS_ASSERT_EQUALS(cf->getParameter(3),1.2);
    TS_ASSERT_EQUALS(fun->name(),"FunctionFactoryTest_CompFunctA");
    TS_ASSERT(fun->hasAttribute("attr"));
    TS_ASSERT_EQUALS(fun->getAttribute("attr").asString(),"hello");

    IConstraint* c = fun->getConstraint(2);
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(),0);
    fun->setParameter("f1.b0",2);
    TS_ASSERT_EQUALS(c->check(),1000);

    c = fun->getConstraint(3);
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(),0);
    fun->setParameter("f1.b1",0.5);
    TS_ASSERT_EQUALS(c->check(),500);

    delete fun;
  }

  void testCreateCompositeWithConstraints1()
  {
    std::string fnString = "composite=FunctionFactoryTest_CompFunctA,attr = \"hello\";"
      "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2;"
      "constraints=(f0.a0<1,f1.b1>1)";

    IFitFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    FunctionFactoryTest_CompFunctA* cf = dynamic_cast<FunctionFactoryTest_CompFunctA*>(fun);
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(cf->nParams(),4);
    TS_ASSERT_EQUALS(cf->parameterName(0),"f0.a0");
    TS_ASSERT_EQUALS(cf->parameterName(1),"f0.a1");
    TS_ASSERT_EQUALS(cf->parameterName(2),"f1.b0");
    TS_ASSERT_EQUALS(cf->parameterName(3),"f1.b1");
    TS_ASSERT_EQUALS(cf->getParameter(0),0.);
    TS_ASSERT_EQUALS(cf->getParameter(1),0.);
    TS_ASSERT_EQUALS(cf->getParameter(2),0.2);
    TS_ASSERT_EQUALS(cf->getParameter(3),1.2);
    TS_ASSERT_EQUALS(fun->name(),"FunctionFactoryTest_CompFunctA");
    TS_ASSERT(fun->hasAttribute("attr"));
    TS_ASSERT_EQUALS(fun->getAttribute("attr").asString(),"hello");

    IConstraint* c = fun->getConstraint(0);
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(),0);
    fun->setParameter("f0.a0",2);
    TS_ASSERT_EQUALS(c->check(),1000);

    c = fun->getConstraint(3);
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(),0);
    fun->setParameter("f1.b1",0.5);
    TS_ASSERT_EQUALS(c->check(),500);

    delete fun;
  }

  void testCreateWithTies()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1,ties=(a0=a1^2)";
    IFitFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);

    funa->applyTies();

    TS_ASSERT_DELTA(funa->getParameter("a0"),1.21,0.0001);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);

    delete funa;

  }

  void testCreateWithTies1()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1,ties=(a0=a1=4)";
    IFitFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);

    funa->applyTies();

    TS_ASSERT_EQUALS(funa->getParameter("a0"),4);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),4);

    delete funa;

  }

  void testCreateWithTies2()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1,ties=(a0=2,a1=4)";
    IFitFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);

    funa->applyTies();

    TS_ASSERT_EQUALS(funa->getParameter("a0"),2);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),4);

    delete funa;

  }

  void testCreateCompositeWithTies()
  {
    std::string fnString = 
      "name=FunctionFactoryTest_FunctA,ties=(a0=a1=14);"
      "name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2;ties=(f1.b0=f0.a0+f0.a1)";

    IFitFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    TS_ASSERT_EQUALS(fun->getParameter(0),0.);
    TS_ASSERT_EQUALS(fun->getParameter(1),0.);
    TS_ASSERT_EQUALS(fun->getParameter(2),0.2);
    TS_ASSERT_EQUALS(fun->getParameter(3),1.2);

    fun->applyTies();

    TS_ASSERT_EQUALS(fun->getParameter(0),14.);
    TS_ASSERT_EQUALS(fun->getParameter(1),14.);
    TS_ASSERT_EQUALS(fun->getParameter(2),28.);
    TS_ASSERT_EQUALS(fun->getParameter(3),1.2);

    IFitFunction* fun1 = FunctionFactory::Instance().createInitialized(*fun);

    fun1->setParameter(0,0.);
    fun1->setParameter(1,0.);
    fun1->setParameter(2,0.);
    fun1->setParameter(3,789);

    TS_ASSERT_EQUALS(fun1->getParameter(0),0.);
    TS_ASSERT_EQUALS(fun1->getParameter(1),0.);
    TS_ASSERT_EQUALS(fun1->getParameter(2),0.);
    TS_ASSERT_EQUALS(fun1->getParameter(3),789);

    fun1->applyTies();

    TS_ASSERT_EQUALS(fun1->getParameter(0),14.);
    TS_ASSERT_EQUALS(fun1->getParameter(1),14.);
    TS_ASSERT_EQUALS(fun1->getParameter(2),28.);
    TS_ASSERT_EQUALS(fun1->getParameter(3),789);

    delete fun;
    delete fun1;
  }

  void testCreateFitFunction_creates_old_IFitFunction()
  {
    IFitFunction *gauss = FunctionFactory::Instance().createFitFunction("Gaussian(PeakCentre=17.4e-2,Height=10,Sigma=0.33)");
    TS_ASSERT(gauss);
    TS_ASSERT(dynamic_cast<IFitFunction*>(gauss));

    TS_ASSERT_EQUALS(gauss->name(),"Gaussian");
    TS_ASSERT_EQUALS(gauss->getParameter("PeakCentre"),0.174);
    TS_ASSERT_EQUALS(gauss->getParameter("Height"),10);
    TS_ASSERT_EQUALS(gauss->getParameter("Sigma"),0.33);
  }

};

#endif /*FUNCTIONFACTORYTEST_H_*/
