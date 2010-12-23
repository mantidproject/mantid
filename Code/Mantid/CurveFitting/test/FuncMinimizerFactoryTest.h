#ifndef FUNCMINIMIZERFACTORYTEST_H_
#define FUNCMINIMIZERFACTORYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/FuncMinimizerFactory.h"
#include "MantidCurveFitting/IFuncMinimizer.h"
#include "MantidCurveFitting/GSLFunctions.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IFitFunction.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class FuncMinimizerFactoryTest_A: public IFuncMinimizer
{
  int m_attr;
public:
  FuncMinimizerFactoryTest_A() {}

  /// Overloading base class methods
  std::string name()const {return "Boevs";}
  int iterate() {return 1000;}
  int hasConverged() {return 101;}
  double costFunctionVal() {return 5.0;}
  void calCovarianceMatrix(double epsrel, gsl_matrix * covar) {}
  void initialize(double* X, const double* Y, double *sqrtWeight, const int& nData, const int& nParam, 
    gsl_vector* startGuess, IFitFunction* function, const std::string& costFunction) {}
};

DECLARE_FUNCMINIMIZER(FuncMinimizerFactoryTest_A, nedtur);


class FuncMinimizerFactoryTest : public CxxTest::TestSuite
{
public:
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

};

#endif /*FUNCMINIMIZERFACTORYTEST_H_*/
