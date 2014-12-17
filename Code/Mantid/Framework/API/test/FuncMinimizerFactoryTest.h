#ifndef FUNCMINIMIZERFACTORYTEST_H_
#define FUNCMINIMIZERFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/System.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::API;

class FuncMinimizerFactoryTest_A: public IFuncMinimizer
{
public:
  FuncMinimizerFactoryTest_A() 
  {
    declareProperty("paramA",0.0);
    declareProperty("paramB",0.0);
  }

  /// Overloading base class methods
  std::string name()const {return "Boevs";}
  bool iterate(size_t) {return true;}
  int hasConverged() {return 101;}
  double costFunctionVal() {return 5.0;}
  void initialize(API::ICostFunction_sptr,size_t)
  {
  }
};

DECLARE_FUNCMINIMIZER(FuncMinimizerFactoryTest_A, nedtur);


class FuncMinimizerFactoryTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FuncMinimizerFactoryTest *createSuite() { return new FuncMinimizerFactoryTest(); }
  static void destroySuite( FuncMinimizerFactoryTest *suite ) { delete suite; }

  FuncMinimizerFactoryTest()
  {
    Mantid::API::FrameworkManager::Instance();
  }

  void testCreateFunction()
  {
    IFuncMinimizer* minimizerA = FuncMinimizerFactory::Instance().createUnwrapped("nedtur");
    TS_ASSERT(minimizerA);
    TS_ASSERT(minimizerA->name().compare("Boevs") == 0);

    delete minimizerA;
  }

  void test_createMinimizer_setAllProperties()
  {
    auto minimizer = FuncMinimizerFactory::Instance().createMinimizer("nedtur, paramA= 3.14, paramB = 2.73");
    TS_ASSERT( minimizer );
    TS_ASSERT( minimizer->existsProperty("paramA") );
    TS_ASSERT( minimizer->existsProperty("paramB") );
    double a = minimizer->getProperty("paramA");
    double b = minimizer->getProperty("paramB");
    TS_ASSERT_EQUALS( a, 3.14 );
    TS_ASSERT_EQUALS( b, 2.73 );
  }

  void test_createMinimizer_defaultPropeties()
  {
    auto minimizer = FuncMinimizerFactory::Instance().createMinimizer("nedtur");
    TS_ASSERT( minimizer );
    TS_ASSERT( minimizer->existsProperty("paramA") );
    TS_ASSERT( minimizer->existsProperty("paramB") );
    double a = minimizer->getProperty("paramA");
    double b = minimizer->getProperty("paramB");
    TS_ASSERT_EQUALS( a, 0.0 );
    TS_ASSERT_EQUALS( b, 0.0 );
  }

  void test_createMinimizer_setOneProperty()
  {
    auto minimizer = FuncMinimizerFactory::Instance().createMinimizer("nedtur, paramB = 2.73");
    TS_ASSERT( minimizer );
    TS_ASSERT( minimizer->existsProperty("paramA") );
    TS_ASSERT( minimizer->existsProperty("paramB") );
    double a = minimizer->getProperty("paramA");
    double b = minimizer->getProperty("paramB");
    TS_ASSERT_EQUALS( a, 0.0 );
    TS_ASSERT_EQUALS( b, 2.73 );
  }

};

#endif /*FUNCMINIMIZERFACTORYTEST_H_*/
