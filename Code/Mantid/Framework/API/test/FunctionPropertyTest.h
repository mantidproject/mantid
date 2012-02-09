#ifndef FUNCTIONPROPERTYTEST_H_
#define FUNCTIONPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ParamFunction.h"
#include <boost/shared_ptr.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

class FunctionPropertyTest_Function: public virtual ParamFunction, public virtual IFitFunction
{
public:
  FunctionPropertyTest_Function()
  {
    this->declareParameter("A",1.0);
    this->declareParameter("B",2.0);
  }
  virtual std::string name()const {return "FunctionPropertyTest_Function";}
  virtual void setWorkspace(boost::shared_ptr<const Workspace>) {}
  virtual void function(FunctionDomain&)const {}
  virtual boost::shared_ptr<const Workspace> getWorkspace()const {return boost::shared_ptr<const Workspace>();}
  virtual void setWorkspace(boost::shared_ptr<const Workspace>,bool) {}
  virtual void setSlicing(const std::string&) {}

  /// Returns the size of the fitted data (number of double values returned by the function getData())
  virtual size_t dataSize()const {return 0;}
  /// Returns a reference to the fitted data. These data are taken from the workspace set by setWorkspace() method.
  virtual const double* getData()const {return NULL;}
  virtual const double* getWeights()const {return NULL;}
  /// Function you want to fit to. 
  /// @param out :: The buffer for writing the calculated values. Must be big enough to accept dataSize() values
  virtual void function(double*)const {}
};

DECLARE_FUNCTION(FunctionPropertyTest_Function);

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
    std::string error;
    TS_ASSERT_THROWS_NOTHING(error = prop.setValue("name=FunctionPropertyTest_Function,A=3"));
    TS_ASSERT(error.empty());
    boost::shared_ptr<IFitFunction> fun_p = prop;
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
    boost::shared_ptr<IFitFunction> fun_p(FunctionFactory::Instance().createInitialized("name=FunctionPropertyTest_Function,A=3"));
    TS_ASSERT(fun_p);
    prop = fun_p;
    boost::shared_ptr<IFitFunction> fun1_p = prop;
    TS_ASSERT(fun1_p);
    TS_ASSERT_EQUALS(fun_p,fun1_p);
    TS_ASSERT_EQUALS(fun1_p->asString(),"name=FunctionPropertyTest_Function,A=3,B=2");
    TS_ASSERT_EQUALS(fun1_p->getParameter("A"), 3.0);
    TS_ASSERT_EQUALS(fun1_p->getParameter("B"), 2.0);
  }
private:
};

#endif /*FUNCTIONPROPERTYTEST_H_*/
