#ifndef FUNCTIONPROPERTYTEST_H_
#define FUNCTIONPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ParamFunction.h"
#include <boost/shared_ptr.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

class FunctionPropertyTest_Function: public virtual ParamFunction, public virtual IFunction
{
public:
  FunctionPropertyTest_Function()
  {
    this->declareParameter("A",1.0);
    this->declareParameter("B",2.0);
  }
  virtual std::string name()const {return "FunctionPropertyTest_Function";}
  virtual void function(const FunctionDomain&,FunctionValues&)const {}
};

DECLARE_FUNCTION(FunctionPropertyTest_Function)

class FunctionPropertyTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FunctionPropertyTest *createSuite() { return new FunctionPropertyTest (); }
  static void destroySuite( FunctionPropertyTest *suite ) { delete suite; }

  void testConstructor()
  {
    TS_ASSERT_THROWS_NOTHING(FunctionProperty prop("fun"));
  }

  void testValue()
  {
    FunctionProperty prop("fun");
    TS_ASSERT_EQUALS("", prop.value());

    std::string error;
    TS_ASSERT_THROWS_NOTHING(error = prop.setValue("name=FunctionPropertyTest_Function,A=3"));
    TS_ASSERT(error.empty());
    boost::shared_ptr<IFunction> fun_p = prop;
    TS_ASSERT_EQUALS(fun_p->asString(),"name=FunctionPropertyTest_Function,A=3,B=2");
    TS_ASSERT_EQUALS(fun_p->getParameter("A"), 3.0);
    TS_ASSERT_EQUALS(fun_p->getParameter("B"), 2.0);
    TS_ASSERT_EQUALS(prop.value(),"name=FunctionPropertyTest_Function,A=3,B=2");
  }

  void testBadValue()
  {  
    FunctionProperty prop("fun");
    std::string error;
    TS_ASSERT_THROWS_NOTHING(error = prop.setValue("name=FunctionDoesnotExist,A=3"));
    TS_ASSERT(!error.empty());
    TS_ASSERT_THROWS_NOTHING(error = prop.setValue("ghvjhgvjhgcjh"));
    TS_ASSERT(!error.empty());
  }

  void testSetValue()
  {
    FunctionProperty prop("fun");
    std::string error;
    boost::shared_ptr<IFunction> fun_p(FunctionFactory::Instance().createInitialized("name=FunctionPropertyTest_Function,A=3"));
    TS_ASSERT(fun_p);
    prop = fun_p;
    boost::shared_ptr<IFunction> fun1_p = prop;
    TS_ASSERT(fun1_p);
    TS_ASSERT_EQUALS(fun_p,fun1_p);
    TS_ASSERT_EQUALS(fun1_p->asString(),"name=FunctionPropertyTest_Function,A=3,B=2");
    TS_ASSERT_EQUALS(fun1_p->getParameter("A"), 3.0);
    TS_ASSERT_EQUALS(fun1_p->getParameter("B"), 2.0);
  }

  void testSharedPointer()
  {
    FunctionProperty prop("fun");
    std::string error;
    boost::shared_ptr<FunctionPropertyTest_Function> fun_p(new FunctionPropertyTest_Function);
    TS_ASSERT(fun_p);
    fun_p->setParameter("A",3);
    prop = fun_p;
    boost::shared_ptr<IFunction> fun1_p = prop;
    TS_ASSERT(fun1_p);
    TS_ASSERT_EQUALS(fun_p,fun1_p);
    TS_ASSERT_EQUALS(fun1_p->asString(),"name=FunctionPropertyTest_Function,A=3,B=2");
    TS_ASSERT_EQUALS(fun1_p->getParameter("A"), 3.0);
    TS_ASSERT_EQUALS(fun1_p->getParameter("B"), 2.0);
  }

private:
};

#endif /*FUNCTIONPROPERTYTEST_H_*/
