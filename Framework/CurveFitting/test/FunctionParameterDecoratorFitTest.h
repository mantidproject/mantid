#ifndef FUNCTIONPARAMETERDECORATORFITTEST_H
#define FUNCTIONPARAMETERDECORATORFITTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/FunctionParameterDecorator.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidCurveFitting/Algorithms/Fit.h"

#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;

class FunctionParameterDecoratorFitTest;

/* This class is used to test that Fit works with the decorators. It simply
 * forwards calls to function to the decorated function.
 */
class SimpleFunctionParameterDecorator : public FunctionParameterDecorator {
  friend class FunctionParameterDecoratorFitTest;

public:
  SimpleFunctionParameterDecorator() {}
  ~SimpleFunctionParameterDecorator() override {}

  std::string name() const override {
    return "SimpleFunctionParameterDecorator";
  }

  void function(const FunctionDomain &domain,
                FunctionValues &values) const override {
    throwIfNoFunctionSet();

    IFunction_sptr fn = getDecoratedFunction();
    fn->function(domain, values);
  }

  void functionDeriv(const FunctionDomain &domain,
                     Jacobian &jacobian) override {
    throwIfNoFunctionSet();

    IFunction_sptr fn = getDecoratedFunction();
    fn->functionDeriv(domain, jacobian);
  }
};

DECLARE_FUNCTION(SimpleFunctionParameterDecorator)

class FunctionParameterDecoratorFitTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FunctionParameterDecoratorFitTest *createSuite() {
    return new FunctionParameterDecoratorFitTest();
  }
  static void destroySuite(FunctionParameterDecoratorFitTest *suite) {
    delete suite;
  }

  FunctionParameterDecoratorFitTest() { FrameworkManager::Instance(); }

  void testFunctionIsRegistered() {
    IFunction_sptr fn = FunctionFactory::Instance().createFunction(
        "SimpleFunctionParameterDecorator");

    TS_ASSERT(fn);
  }

  void testFit() {
    Workspace2D_sptr ws =
        WorkspaceCreationHelper::create1DWorkspaceConstant(20, 1.5, 0.5, true);

    FunctionParameterDecorator_sptr fn =
        boost::make_shared<SimpleFunctionParameterDecorator>();
    fn->setDecoratedFunction("FlatBackground");
    fn->setParameter("A0", 10.5);

    IAlgorithm_sptr fitAlg = AlgorithmManager::Instance().create("Fit");
    fitAlg->setProperty("Function", boost::static_pointer_cast<IFunction>(fn));
    fitAlg->setProperty("InputWorkspace", ws);

    fitAlg->execute();

    TS_ASSERT(fitAlg->isExecuted());

    IFunction_sptr fitFunction = fitAlg->getProperty("Function");
    TS_ASSERT_DELTA(fitFunction->getParameter("A0"), 1.5, 1e-15);
  }
};

#endif // FUNCTIONPARAMETERDECORATORFITTEST_H
