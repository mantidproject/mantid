#ifndef FUNCTIONFACTORYTEST_H_
#define FUNCTIONFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/Function.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IConstraint.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::API;

class FunctionFactoryTest_FunctA: public Function
{
  std::string m_attr;
public:
  FunctionFactoryTest_FunctA()
  {
    declareParameter("a0");
    declareParameter("a1");
  }
  std::string name()const{return "FunctionFactoryTest_FunctA";}
  void function(double* out, const double* xValues, const int& nData){}
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData){}
  bool hasAttribute(const std::string& attName)const
  {
    if (attName == "attr") return true;
    return false;
  }
  std::string getAttribute(const std::string& attName)const
  {
    if (attName == "attr") return m_attr;
    return "";
  }
  void setAttribute(const std::string& attName,const std::string& value)
  {
    if (attName == "attr")
    {
      int n = atoi(value.c_str());
      if (n > 0)
      {
        m_attr = value;
        clearAllParameters();
        for(int i=0;i<n;i++)
        {
          std::ostringstream ostr;
          ostr << "at_" << i;
          declareParameter(ostr.str());
        }
      }
    }
  }
};

class FunctionFactoryTest_FunctB: public Function
{
public:
  FunctionFactoryTest_FunctB()
  {
    declareParameter("b0");
    declareParameter("b1");
  }

  std::string name()const{return "FunctionFactoryTest_FunctB";}

  void function(double* out, const double* xValues, const int& nData)
  {
  }
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData)
  {
  }
};

class FunctionFactoryTest_CompFunctA: public CompositeFunction
{
  std::string m_attr;
public:
  FunctionFactoryTest_CompFunctA(){}

  std::string name()const{return "FunctionFactoryTest_CompFunctA";}

  void function(double* out, const double* xValues, const int& nData){}
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData){}
  bool hasAttribute(const std::string& attName)const
  {
    if (attName == "attr") return true;
    return false;
  }
  std::string getAttribute(const std::string& attName)const
  {
    if (attName == "attr") return m_attr;
    return "";
  }
  void setAttribute(const std::string& attName,const std::string& value)
  {
    m_attr = value;
  }
};

class FunctionFactoryTest_CompFunctB: public CompositeFunction
{
public:
  FunctionFactoryTest_CompFunctB(){}

  std::string name()const{return "FunctionFactoryTest_CompFunctB";}

  void function(double* out, const double* xValues, const int& nData)
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
    IFunction* funa = FunctionFactory::Instance().createFunction("FunctionFactoryTest_FunctA");
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);

    IFunction* funb = FunctionFactory::Instance().createFunction("FunctionFactoryTest_FunctB");
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
    IFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);
    delete funa;
  }

  void testCreateSimple()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1";
    IFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
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
    IFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
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

    IFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
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
  }

  void testCreateComposite1()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2";

    IFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
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
  }

  void testCreateComposite2()
  {
    std::string fnString = "composite=FunctionFactoryTest_CompFunctB;";
    fnString += "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2";

    IFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
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
  }

  void testCreateComposite3()
  {
    std::string fnString = "composite=FunctionFactoryTest_CompFunctA,attr = \"hello\";";
    fnString += "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2";

    IFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
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
    TS_ASSERT_EQUALS(fun->getAttribute("attr"),"hello");
  }

  void testCreateCompositeNested()
  {
    std::string fnString = "(composite=FunctionFactoryTest_CompFunctA,attr = hello;";
    fnString += "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2);";
    fnString += "(composite=FunctionFactoryTest_CompFunctB;";
    fnString += "name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2;name=FunctionFactoryTest_FunctA)";

    IFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);

    CompositeFunction* cf = dynamic_cast<CompositeFunction*>(fun);
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(cf->nFunctions(),2);
    TS_ASSERT_EQUALS(cf->getFunction(0)->name(),"FunctionFactoryTest_CompFunctA");
    TS_ASSERT_EQUALS(cf->getFunction(1)->name(),"FunctionFactoryTest_CompFunctB");
    TS_ASSERT_EQUALS(dynamic_cast<CompositeFunction*>(cf->getFunction(0))->nFunctions(),2);
    TS_ASSERT_EQUALS(dynamic_cast<CompositeFunction*>(cf->getFunction(1))->nFunctions(),2);
  }
  void testCreateWithConstraint()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1(0<a0<0.2),a1=1.1";
    IFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);

    IConstraint* c = funa->firstConstraint();
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(),0);

    funa->setParameter("a0",1);
    TS_ASSERT_EQUALS(c->check(),800);

    funa->setParameter("a0",-1);
    TS_ASSERT_EQUALS(c->check(),1000);
    delete funa;

  }

  void testCreateWithConstraint1()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1,constraint=0<a0<0.2";
    IFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);

    IConstraint* c = funa->firstConstraint();
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
    IFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);

    IConstraint* c0 = funa->firstConstraint();
    TS_ASSERT(c0);
    TS_ASSERT_EQUALS(c0->check(),0);

    funa->setParameter("a0",1);
    TS_ASSERT_EQUALS(c0->check(),800);

    funa->setParameter("a0",-1);
    TS_ASSERT_EQUALS(c0->check(),1000);

    IConstraint* c1 = funa->nextConstraint();
    TS_ASSERT(c1);
    TS_ASSERT_EQUALS(c1->check(),8900);

    funa->setParameter("a1",11);
    TS_ASSERT_EQUALS(c1->check(),0);

    delete funa;

  }

  void testCreateWithConstraint3()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1(0:0.2),a1=1.1";
    IFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);

    IConstraint* c = funa->firstConstraint();
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(),0);

    funa->setParameter("a0",1);
    TS_ASSERT_EQUALS(c->check(),800);

    funa->setParameter("a0",-1);
    TS_ASSERT_EQUALS(c->check(),1000);
    delete funa;

  }

  void testCreateWithConstraint4()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1(:0.2),a1=1.1";
    IFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);

    IConstraint* c = funa->firstConstraint();
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(),0);

    funa->setParameter("a0",1);
    TS_ASSERT_EQUALS(c->check(),800);

    funa->setParameter("a0",-1);
    TS_ASSERT_EQUALS(c->check(),0);
    delete funa;

  }

  void testCreateWithConstraint5()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1(0:),a1=1.1";
    IFunction* funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);

    IConstraint* c = funa->firstConstraint();
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(),0);

    funa->setParameter("a0",1);
    TS_ASSERT_EQUALS(c->check(),0);

    funa->setParameter("a0",-1);
    TS_ASSERT_EQUALS(c->check(),1000);
    delete funa;

  }

  void testCreateCompositeWithConstraints()
  {
    std::string fnString = "composite=FunctionFactoryTest_CompFunctA,attr = \"hello\";"
      "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2,"
      "constraints=(b0<1,b1>1)";

    IFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
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
    TS_ASSERT_EQUALS(fun->getAttribute("attr"),"hello");

    IConstraint* c = fun->firstConstraint();
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(),0);
    fun->setParameter("f1.b0",2);
    TS_ASSERT_EQUALS(c->check(),1000);

    c = fun->nextConstraint();
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

    IFunction* fun = FunctionFactory::Instance().createInitialized(fnString);
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
    TS_ASSERT_EQUALS(fun->getAttribute("attr"),"hello");

    IConstraint* c = fun->firstConstraint();
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(),0);
    fun->setParameter("f0.a0",2);
    TS_ASSERT_EQUALS(c->check(),1000);

    c = fun->nextConstraint();
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(),0);
    fun->setParameter("f1.b1",0.5);
    TS_ASSERT_EQUALS(c->check(),500);

    delete fun;
  }

};

#endif /*FUNCTIONFACTORYTEST_H_*/
