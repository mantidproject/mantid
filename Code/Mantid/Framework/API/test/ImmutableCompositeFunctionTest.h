#ifndef IMMUTABLECOMPOSITEFUNCTIONTEST_H_
#define IMMUTABLECOMPOSITEFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ImmutableCompositeFunction.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid;
using namespace Mantid::API;


class ImmutableCompositeFunctionTest_Linear: public ParamFunction, public IFunction1D
{
public:
  ImmutableCompositeFunctionTest_Linear()
  {
    declareParameter("a");
    declareParameter("b");
  }

  std::string name()const{return "Linear";}

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
    for(size_t i=0;i<nData;i++)
    {
      out->set(static_cast<int>(i),0,1.);
      out->set(static_cast<int>(i),1,xValues[i]);
    }
  }

};

//---------------------------------------------------------------------------------
class ImmutableCompositeFunctionTest_Function: public ImmutableCompositeFunction
{
public:
  ImmutableCompositeFunctionTest_Function(): ImmutableCompositeFunction()
  {
    IFunction* fun1 = new ImmutableCompositeFunctionTest_Linear;
    fun1->setParameter( "a", 1.0 );
    fun1->setParameter( "b", 2.0 );
    addFunction( fun1 );

    IFunction* fun2 = new ImmutableCompositeFunctionTest_Linear;
    fun2->setParameter( "a", 3.0 );
    fun2->setParameter( "b", 4.0 );
    addFunction( fun2 );

    setAlias("f0.a", "a1");
    setAlias("f0.b", "b1");
    setAlias("f1.a", "a2");
    setAlias("f1.b", "b2");
  }
  std::string name()const {return "ImmutableCompositeFunctionTest_Function";}
};

DECLARE_FUNCTION(ImmutableCompositeFunctionTest_Function)

//---------------------------------------------------------------------------------
class ImmutableCompositeFunctionTest_FunctionWithTies: public ImmutableCompositeFunction
{
public:
  ImmutableCompositeFunctionTest_FunctionWithTies(): ImmutableCompositeFunction()
  {
    IFunction* fun1 = new ImmutableCompositeFunctionTest_Linear;
    fun1->setParameter( "a", 1.0 );
    fun1->setParameter( "b", 2.0 );
    addFunction( fun1 );

    IFunction* fun2 = new ImmutableCompositeFunctionTest_Linear;
    fun2->setParameter( "a", 3.0 );
    fun2->setParameter( "b", 4.0 );
    addFunction( fun2 );

    setAlias("f0.a", "a1");
    setAlias("f0.b", "b1");
    setAlias("f1.a", "a2");
    setAlias("f1.b", "b2");

    addDefaultTies("b2 = a1, a2 = a1/4");
  }
  std::string name()const {return "ImmutableCompositeFunctionTest_FunctionWithTies";}
};

DECLARE_FUNCTION(ImmutableCompositeFunctionTest_FunctionWithTies)

//---------------------------------------------------------------------------------
class ImmutableCompositeFunctionTest_FunctionThrow: public ImmutableCompositeFunction
{
public:
  ImmutableCompositeFunctionTest_FunctionThrow(): ImmutableCompositeFunction()
  {
    IFunction* fun1 = new ImmutableCompositeFunctionTest_Linear;
    fun1->setParameter( "a", 1.0 );
    fun1->setParameter( "b", 2.0 );
    addFunction( fun1 );

    IFunction* fun2 = new ImmutableCompositeFunctionTest_Linear;
    fun2->setParameter( "a", 3.0 );
    fun2->setParameter( "b", 4.0 );
    addFunction( fun2 );

    setAlias("f0.a", "a1");
    setAlias("f0.b", "b1");
    setAlias("f1.a", "a1"); // repeated alias
    setAlias("f1.b", "b2");
  }
};

//---------------------------------------------------------------------------------
class ImmutableCompositeFunctionTest_FunctionThrow1: public ImmutableCompositeFunction
{
public:
  ImmutableCompositeFunctionTest_FunctionThrow1(): ImmutableCompositeFunction()
  {
    IFunction* fun1 = new ImmutableCompositeFunctionTest_Linear;
    fun1->setParameter( "a", 1.0 );
    fun1->setParameter( "b", 2.0 );
    addFunction( fun1 );

    IFunction* fun2 = new ImmutableCompositeFunctionTest_Linear;
    fun2->setParameter( "a", 3.0 );
    fun2->setParameter( "b", 4.0 );
    addFunction( fun2 );

    setAlias("f0.a", "a1");
    setAlias("f0.b", "b1");
    setAlias("f1.a", "a2");
    setAlias("f1.c", "b2"); // name doesn't exist
  }
};

//---------------------------------------------------------------------------------
class ImmutableCompositeFunctionTest : public CxxTest::TestSuite
{
public:
  void testAdd()
  {
    ImmutableCompositeFunctionTest_Function icf;
    TS_ASSERT_EQUALS( icf.nFunctions(), 2 );
    TS_ASSERT_EQUALS( icf.getParameter(0), 1.0 );
    TS_ASSERT_EQUALS( icf.getParameter(1), 2.0 );
    TS_ASSERT_EQUALS( icf.getParameter(2), 3.0 );
    TS_ASSERT_EQUALS( icf.getParameter(3), 4.0 );
  }
  
  void testFactoryCreate()
  {
    auto fun = FunctionFactory::Instance().createInitialized("name=ImmutableCompositeFunctionTest_Function");
    TS_ASSERT( fun );
    TS_ASSERT_EQUALS( fun->nParams(), 4 );
    TS_ASSERT_EQUALS( fun->getParameter(0), 1.0 );
    TS_ASSERT_EQUALS( fun->getParameter(1), 2.0 );
    TS_ASSERT_EQUALS( fun->getParameter(2), 3.0 );
    TS_ASSERT_EQUALS( fun->getParameter(3), 4.0 );
  }

  void testFactoryInitialize()
  {
    std::string ini = "name=ImmutableCompositeFunctionTest_Function,a1=7.0,b1=8.0,a2=9.0,b2=0";
    auto fun = FunctionFactory::Instance().createInitialized(ini);
    TS_ASSERT( fun );
    TS_ASSERT_EQUALS( fun->nParams(), 4 );
    TS_ASSERT_EQUALS( fun->getParameter(0), 7.0 );
    TS_ASSERT_EQUALS( fun->getParameter(1), 8.0 );
    TS_ASSERT_EQUALS( fun->getParameter(2), 9.0 );
    TS_ASSERT_EQUALS( fun->getParameter(3), 0.0 );
  }

  void testParameterAlias()
  {
    ImmutableCompositeFunctionTest_Function icf;

    TS_ASSERT_EQUALS( icf.getParameter("a1"), 1.0 );
    TS_ASSERT_EQUALS( icf.getParameter("b1"), 2.0 );
    TS_ASSERT_EQUALS( icf.getParameter("a2"), 3.0 );
    TS_ASSERT_EQUALS( icf.getParameter("b2"), 4.0 );

    TS_ASSERT_EQUALS( icf.getParameter("f0.a"), 1.0 );
    TS_ASSERT_EQUALS( icf.getParameter("f0.b"), 2.0 );
    TS_ASSERT_EQUALS( icf.getParameter("f1.a"), 3.0 );
    TS_ASSERT_EQUALS( icf.getParameter("f1.b"), 4.0 );
  }

  void testSetParameter()
  {
    ImmutableCompositeFunctionTest_Function icf;

    icf.setParameter("a1", 11.0);
    icf.setParameter("b1", 12.0);
    icf.setParameter("a2", 13.0);
    icf.setParameter("b2", 14.0);

    TS_ASSERT_EQUALS( icf.getParameter(0), 11.0 );
    TS_ASSERT_EQUALS( icf.getParameter(1), 12.0 );
    TS_ASSERT_EQUALS( icf.getParameter(2), 13.0 );
    TS_ASSERT_EQUALS( icf.getParameter(3), 14.0 );

  }

  void testSetParameterDescription()
  {
    ImmutableCompositeFunctionTest_Function icf;

    icf.setParameterDescription("a1", "First a parameter");
    icf.setParameterDescription("b1", "First b parameter");
    icf.setParameterDescription("a2", "Second a parameter");
    icf.setParameterDescription("f1.b", "Second b parameter");

    TS_ASSERT_EQUALS( icf.parameterDescription(0), "First a parameter" );
    TS_ASSERT_EQUALS( icf.parameterDescription(1), "First b parameter" );
    TS_ASSERT_EQUALS( icf.parameterDescription(2), "Second a parameter" );
    TS_ASSERT_EQUALS( icf.parameterDescription(3), "Second b parameter" );
  }

  void testParameterIndex()
  {
    ImmutableCompositeFunctionTest_Function icf;

    TS_ASSERT_EQUALS( icf.parameterIndex("a1"), 0 );
    TS_ASSERT_EQUALS( icf.parameterIndex("b1"), 1 );
    TS_ASSERT_EQUALS( icf.parameterIndex("a2"), 2 );
    TS_ASSERT_EQUALS( icf.parameterIndex("b2"), 3 );

    TS_ASSERT_EQUALS( icf.parameterIndex("f0.a"), 0 );
    TS_ASSERT_EQUALS( icf.parameterIndex("f0.b"), 1 );
    TS_ASSERT_EQUALS( icf.parameterIndex("f1.a"), 2 );
    TS_ASSERT_EQUALS( icf.parameterIndex("f1.b"), 3 );
  }

  void testParameterName()
  {
    ImmutableCompositeFunctionTest_Function icf;

    TS_ASSERT_EQUALS( icf.parameterName(0), "a1" );
    TS_ASSERT_EQUALS( icf.parameterName(1), "b1" );
    TS_ASSERT_EQUALS( icf.parameterName(2), "a2" );
    TS_ASSERT_EQUALS( icf.parameterName(3), "b2" );
  }

  void testParameterAliasUnique()
  {
    TS_ASSERT_THROWS(ImmutableCompositeFunctionTest_FunctionThrow icf, Mantid::Kernel::Exception::ExistsError);
  }

  void testSetAliasThrowsIfNameDoesntExist()
  {
    TS_ASSERT_THROWS(ImmutableCompositeFunctionTest_FunctionThrow1 icf, std::invalid_argument);
  }

  void testAddTies()
  {
    ImmutableCompositeFunctionTest_Function icf;

    icf.addTies("b2=b1,a2=a1/5");
    TS_ASSERT( !icf.getTie( 0 ) );
    TS_ASSERT( !icf.getTie( 1 ) );
    TS_ASSERT( icf.getTie( 2 ) );
    TS_ASSERT( icf.getTie( 3 ) );

    icf.applyTies();

    TS_ASSERT_EQUALS( icf.getParameter(0), 1.0 );
    TS_ASSERT_EQUALS( icf.getParameter(1), 2.0 );
    TS_ASSERT_EQUALS( icf.getParameter(2), 0.2 );
    TS_ASSERT_EQUALS( icf.getParameter(3), 2.0 );
  }

  // BoundaryConstraint isn't defined (it's in CurveFitting) so this test doesn't work
  void xtestConstraints()
  {
    ImmutableCompositeFunctionTest_Function icf;

    icf.addConstraints("0 < b1 < 5");
    TS_ASSERT( ! icf.getConstraint( 0 ) );
    TS_ASSERT( ! icf.getConstraint( 1 ) );
    TS_ASSERT(   icf.getConstraint( 2 ) );
    TS_ASSERT( ! icf.getConstraint( 3 ) );
  }

  void testAsString()
  {
    ImmutableCompositeFunctionTest_Function icf;

    icf.setParameter(0, 11.0 );
    icf.setParameter(1, 12.0 );
    icf.setParameter(2, 13.0 );
    icf.setParameter(3, 14.0 );

    icf.addTies("b2=b1,a2=a1/5");
    icf.applyTies();

    TS_ASSERT_EQUALS( icf.asString(), "name=ImmutableCompositeFunctionTest_Function,NumDeriv=false,a1=11,b1=12,a2=2.2,b2=12,ties=(a2=a1/5,b2=b1)" );

    auto fun = FunctionFactory::Instance().createInitialized( icf.asString() );
    TS_ASSERT( fun );
    TS_ASSERT_EQUALS( fun->nParams(), 4 );
    TS_ASSERT_EQUALS( fun->getParameter(0), 11.0 );
    TS_ASSERT_EQUALS( fun->getParameter(1), 12.0 );
    TS_ASSERT_EQUALS( fun->getParameter(2), 2.2 );
    TS_ASSERT_EQUALS( fun->getParameter(3), 12.0 );
    TS_ASSERT( ! fun->getTie( 0 ) );
    TS_ASSERT( ! fun->getTie( 1 ) );
    TS_ASSERT(   fun->getTie( 2 ) );
    TS_ASSERT(   fun->getTie( 3 ) );
  }

  void testAddDefaultTies()
  {
    ImmutableCompositeFunctionTest_FunctionWithTies icf;

    icf.applyTies();

    TS_ASSERT_EQUALS( icf.getParameter(0), 1.0 );
    TS_ASSERT_EQUALS( icf.getParameter(1), 2.0 );
    TS_ASSERT_EQUALS( icf.getParameter(2), 0.25 );
    TS_ASSERT_EQUALS( icf.getParameter(3), 1.0 );

    TS_ASSERT_EQUALS( icf.asString(), "name=ImmutableCompositeFunctionTest_FunctionWithTies,NumDeriv=false,a1=1,b1=2" );

    auto fun = FunctionFactory::Instance().createInitialized( icf.asString() );
    TS_ASSERT( fun );
    TS_ASSERT_EQUALS( fun->nParams(), 4 );
    TS_ASSERT_EQUALS( fun->getParameter(0), 1.0 );
    TS_ASSERT_EQUALS( fun->getParameter(1), 2.0 );
    TS_ASSERT_EQUALS( fun->getParameter(2), 0.25 );
    TS_ASSERT_EQUALS( fun->getParameter(3), 1.0 );
    TS_ASSERT( ! fun->getTie( 0 ) );
    TS_ASSERT( ! fun->getTie( 1 ) );
    TS_ASSERT(   fun->getTie( 2 ) );
    TS_ASSERT(   fun->getTie( 3 ) );

  }

};

#endif /*IMMUTABLECOMPOSITEFUNCTIONTEST_H_*/
