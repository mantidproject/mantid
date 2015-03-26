#ifndef FUNCTIONFACTORYTEST_H_
#define FUNCTIONFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IConstraint.h"
#include "MantidKernel/System.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::API;

class FunctionFactoryTest_FunctA: public ParamFunction, public IFunction1D
{
  int m_attr;
public:
  FunctionFactoryTest_FunctA()
  {
    declareParameter("a0");
    declareParameter("a1");
  }
  std::string name()const{return "FunctionFactoryTest_FunctA";}
  void function1D(double* out, const double* xValues, const size_t nData)const
  {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
  }
  void functionDeriv1D(Jacobian* out, const double* xValues, const size_t nData)
  {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
  }
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

class FunctionFactoryTest_FunctB: public ParamFunction, public IFunction1D
{
public:
  FunctionFactoryTest_FunctB()
  {
    declareParameter("b0");
    declareParameter("b1");
  }

  std::string name()const{return "FunctionFactoryTest_FunctB";}

  void function1D(double* out, const double* xValues, const size_t nData)const
  {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
  }
  void functionDeriv1D(Jacobian* out, const double* xValues, const size_t nData)
  {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
  }
};

class FunctionFactoryTest_CompFunctA: public CompositeFunction
{
  std::string m_attr;
public:
  FunctionFactoryTest_CompFunctA(){}

  std::string name()const{return "FunctionFactoryTest_CompFunctA";}

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
    UNUSED_ARG(attName);
    m_attr = value.asString();
  }
};

class FunctionFactoryTest_CompFunctB: public CompositeFunction
{
public:
  FunctionFactoryTest_CompFunctB(){}

  std::string name()const{return "FunctionFactoryTest_CompFunctB";}

};

DECLARE_FUNCTION(FunctionFactoryTest_FunctA)
DECLARE_FUNCTION(FunctionFactoryTest_FunctB)
DECLARE_FUNCTION(FunctionFactoryTest_CompFunctA)
DECLARE_FUNCTION(FunctionFactoryTest_CompFunctB)

class FunctionFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FunctionFactoryTest *createSuite() { return new FunctionFactoryTest(); }
  static void destroySuite( FunctionFactoryTest *suite ) { delete suite; }

  FunctionFactoryTest()
  {
    Mantid::API::FrameworkManager::Instance();
  }

  void testCreateFunction()
  {
    IFunction_sptr funa = FunctionFactory::Instance().createFunction("FunctionFactoryTest_FunctA");
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);

    IFunction_sptr funb = FunctionFactory::Instance().createFunction("FunctionFactoryTest_FunctB");
    TS_ASSERT(funb);
    TS_ASSERT_EQUALS(funb->parameterName(0),"b0");
    TS_ASSERT_EQUALS(funb->parameterName(1),"b1");
    TS_ASSERT_EQUALS(funb->nParams(),2);
  }

  void testCreateSimpleDefault()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);
  }

  void testCreateSimple()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"a0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"a1");
    TS_ASSERT_EQUALS(funa->nParams(),2);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);
  }

  void testCreateSimpleWithAttribute()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,attr=\"3\",at_0=0.1,at_1=1.1,at_2=2.1";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0),"at_0");
    TS_ASSERT_EQUALS(funa->parameterName(1),"at_1");
    TS_ASSERT_EQUALS(funa->parameterName(2),"at_2");
    TS_ASSERT_EQUALS(funa->nParams(),3);
    TS_ASSERT_EQUALS(funa->getParameter(0),0.1);
    TS_ASSERT_EQUALS(funa->getParameter(1),1.1);
    TS_ASSERT_EQUALS(funa->getParameter(2),2.1);
  }

  void testCreateComposite()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1;name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2";

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    CompositeFunction_sptr cf = boost::dynamic_pointer_cast<CompositeFunction>(fun);
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

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    CompositeFunction_sptr cf = boost::dynamic_pointer_cast<CompositeFunction>(fun);
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

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    FunctionFactoryTest_CompFunctB* cf = dynamic_cast<FunctionFactoryTest_CompFunctB*>(fun.get());
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

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    FunctionFactoryTest_CompFunctA* cf = dynamic_cast<FunctionFactoryTest_CompFunctA*>(fun.get());
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
  }

  void testCreateCompositeNested()
  {
    std::string fnString = "(composite=FunctionFactoryTest_CompFunctA,attr = hello;";
    fnString += "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2);";
    fnString += "(composite=FunctionFactoryTest_CompFunctB;";
    fnString += "name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2;name=FunctionFactoryTest_FunctA)";

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);

    CompositeFunction* cf = dynamic_cast<CompositeFunction*>(fun.get());
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(cf->nFunctions(),2);
    TS_ASSERT_EQUALS(cf->getFunction(0)->name(),"FunctionFactoryTest_CompFunctA");
    TS_ASSERT_EQUALS(cf->getFunction(1)->name(),"FunctionFactoryTest_CompFunctB");
    TS_ASSERT_EQUALS(dynamic_cast<CompositeFunction*>(cf->getFunction(0).get())->nFunctions(),2);
    TS_ASSERT_EQUALS(dynamic_cast<CompositeFunction*>(cf->getFunction(1).get())->nFunctions(),2);
  }

  void testCreateWithTies()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1,ties=(a0=a1^2)";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_DELTA(funa->getParameter("a0"),1.21,0.0001);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),1.1);

  }

  void testCreateWithTies1()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1,ties=(a0=a1=4)";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),4);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),4);

  }

  void testCreateWithTies2()
  {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1,ties=(a0=2,a1=4)";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->getParameter("a0"),2);
    TS_ASSERT_EQUALS(funa->getParameter("a1"),4);

  }

  void testCreateCompositeWithTies()
  {
    std::string fnString = 
      "name=FunctionFactoryTest_FunctA,ties=(a0=a1=14);"
      "name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2;ties=(f1.b0=f0.a0+f0.a1)";

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    TS_ASSERT_EQUALS(fun->getParameter(0),14.);
    TS_ASSERT_EQUALS(fun->getParameter(1),14.);
    TS_ASSERT_EQUALS(fun->getParameter(2),28.);
    TS_ASSERT_EQUALS(fun->getParameter(3),1.2);

    IFunction_sptr fun1 = FunctionFactory::Instance().createInitialized(fun->asString());

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
  }


  void test_MultiDomainFunction_creation()
  {
    std::string fnString = "composite=MultiDomainFunction;"
      "name=FunctionFactoryTest_FunctA;"
      "name=FunctionFactoryTest_FunctB";
    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    // TODO: add more asserts
  }

  void test_getFunctionNames()
  {
    const auto& names = FunctionFactory::Instance().getFunctionNames<IFunction1D>();
    TS_ASSERT( !names.empty() );
    TS_ASSERT( std::find(names.begin(), names.end(), "FunctionFactoryTest_FunctA") != names.end() );
    // Call it again to indirectly test caching
    TS_ASSERT_EQUALS( names, FunctionFactory::Instance().getFunctionNames<IFunction1D>());
  }

};

#endif /*FUNCTIONFACTORYTEST_H_*/
