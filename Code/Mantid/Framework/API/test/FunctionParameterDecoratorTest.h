#ifndef MANTID_API_WRAPPEDFUNCTIONTEST_H_
#define MANTID_API_WRAPPEDFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionParameterDecorator.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/Exception.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

using ::testing::_;
using ::testing::Mock;

class FunctionParameterDecoratorTest;

class TestableFunctionParameterDecorator : public FunctionParameterDecorator {
  friend class FunctionParameterDecoratorTest;

public:
  TestableFunctionParameterDecorator() {}
  ~TestableFunctionParameterDecorator() {}

  std::string name() const { return "TestableFunctionParameterDecorator"; }

  void function(const FunctionDomain &domain, FunctionValues &values) const {
    throwIfNoFunctionSet();

    IFunction_sptr fn = getDecoratedFunction();
    fn->function(domain, values);
  }

  void functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) {
    throwIfNoFunctionSet();

    IFunction_sptr fn = getDecoratedFunction();
    fn->functionDeriv(domain, jacobian);
  }
};

DECLARE_FUNCTION(TestableFunctionParameterDecorator);

class FunctionParameterDecoratorTest : public CxxTest::TestSuite {
public:
  FunctionParameterDecoratorTest() { FrameworkManager::Instance(); }
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FunctionParameterDecoratorTest *createSuite() {
    return new FunctionParameterDecoratorTest();
  }
  static void destroySuite(FunctionParameterDecoratorTest *suite) {
    delete suite;
  }

  void testSetDecoratedFunction() {
    TestableFunctionParameterDecorator fn;

    TS_ASSERT_THROWS_NOTHING(fn.setDecoratedFunction("Gaussian"));

    IFunction_sptr decorated = fn.getDecoratedFunction();
    TS_ASSERT(decorated);
    TS_ASSERT_EQUALS(decorated->name(), "Gaussian");
  }

  void testSetDecoratedFunctionInvalidName() {
    TestableFunctionParameterDecorator fn;
    TS_ASSERT_THROWS(fn.setDecoratedFunction("INVALIDFUNCTION"),
                     Exception::NotFoundError);
    TS_ASSERT(!fn.getDecoratedFunction());
  }

  void testThrowIfNoFunctionSet() {
    TestableFunctionParameterDecorator fn;
    TS_ASSERT_THROWS(fn.throwIfNoFunctionSet(), std::runtime_error);
    fn.setDecoratedFunction("Gaussian");
    TS_ASSERT_THROWS_NOTHING(fn.throwIfNoFunctionSet());
  }

  void testNParams() {
    TestableFunctionParameterDecorator invalidFn;
    TS_ASSERT_THROWS(invalidFn.nParams(), std::runtime_error);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();
    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();

    TS_ASSERT_EQUALS(fn->nParams(), decoratedFunction->nParams());
  }

  void testGetSetParameter() {
    TestableFunctionParameterDecorator invalidFn;
    TS_ASSERT_THROWS(invalidFn.setParameter(0, 2.0), std::runtime_error);
    TS_ASSERT_THROWS(invalidFn.getParameter(0), std::runtime_error);
    TS_ASSERT_THROWS(invalidFn.setParameter("Height", 2.0), std::runtime_error);
    TS_ASSERT_THROWS(invalidFn.getParameter("Height"), std::runtime_error);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();

    TS_ASSERT_THROWS_NOTHING(fn->setParameter(0, 2.0));

    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();
    TS_ASSERT_EQUALS(fn->getParameter(0), decoratedFunction->getParameter(0));
    TS_ASSERT_EQUALS(fn->getParameter(0), 2.0);
    TS_ASSERT_THROWS(fn->getParameter(10), std::out_of_range);

    TS_ASSERT_THROWS_NOTHING(fn->setParameter("Height", 4.0));
    TS_ASSERT_EQUALS(fn->getParameter("Height"),
                     decoratedFunction->getParameter("Height"));
    TS_ASSERT_EQUALS(fn->getParameter("Height"), 4.0);
    TS_ASSERT_THROWS(fn->getParameter("DoesNotExist"), std::invalid_argument);
  }

  void testExplicitelySet() {
    TestableFunctionParameterDecorator invalidFn;
    TS_ASSERT_THROWS(invalidFn.isExplicitlySet(0), std::runtime_error);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();

    TS_ASSERT_THROWS_NOTHING(fn->setParameter(0, 2.0));

    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();

    for (size_t i = 0; i < fn->nParams(); ++i) {
      TS_ASSERT_EQUALS(fn->isExplicitlySet(i),
                       decoratedFunction->isExplicitlySet(i));
    }
  }

  void testGetSetError() {
    TestableFunctionParameterDecorator invalidFn;
    TS_ASSERT_THROWS(invalidFn.getError(0), std::runtime_error);
    TS_ASSERT_THROWS(invalidFn.setError(0, 2.0), std::runtime_error);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();
    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();

    TS_ASSERT_THROWS_NOTHING(fn->setError(0, 3.0));
    TS_ASSERT_EQUALS(fn->getError(0), 3.0);
    for (size_t i = 0; i < fn->nParams(); ++i) {
      TS_ASSERT_EQUALS(fn->getError(i), decoratedFunction->getError(i));
    }
  }

  void testFixUnfixIsFixed() {
    TestableFunctionParameterDecorator invalidFn;
    TS_ASSERT_THROWS(invalidFn.isFixed(0), std::runtime_error);
    TS_ASSERT_THROWS(invalidFn.fix(0), std::runtime_error);
    TS_ASSERT_THROWS(invalidFn.unfix(0), std::runtime_error);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();
    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();

    for (size_t i = 0; i < fn->nParams(); ++i) {
      TS_ASSERT_THROWS_NOTHING(fn->fix(i));
      TS_ASSERT_EQUALS(fn->isFixed(i), decoratedFunction->isFixed(i));
      TS_ASSERT_EQUALS(fn->isFixed(i), true);
      TS_ASSERT_THROWS_NOTHING(fn->unfix(i));
      TS_ASSERT_EQUALS(fn->isFixed(i), decoratedFunction->isFixed(i));
      TS_ASSERT_EQUALS(fn->isFixed(i), false);
    }
  }

  void testAttributes() {
    TestableFunctionParameterDecorator invalidFn;
    TS_ASSERT_THROWS(invalidFn.nAttributes(), std::runtime_error);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();
    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();

    TS_ASSERT_EQUALS(fn->nAttributes(), decoratedFunction->nAttributes());
    TS_ASSERT_EQUALS(fn->nAttributes(), 0);

    fn->setDecoratedFunction("Chebyshev");
    decoratedFunction = fn->getDecoratedFunction();
    TS_ASSERT_EQUALS(fn->nAttributes(), decoratedFunction->nAttributes());
    TS_ASSERT_DIFFERS(fn->nAttributes(), 0);

    std::vector<std::string> decoratorAttributes = fn->getAttributeNames();
    std::vector<std::string> wrappedAttributes =
        decoratedFunction->getAttributeNames();

    TS_ASSERT_EQUALS(decoratorAttributes.size(), wrappedAttributes.size());

    for (size_t i = 0; i < fn->nAttributes(); ++i) {
      TS_ASSERT_EQUALS(decoratorAttributes[i], wrappedAttributes[i]);
      std::string attribute = decoratorAttributes[i];

      TS_ASSERT_EQUALS(fn->hasAttribute(attribute),
                       decoratedFunction->hasAttribute(attribute));
      TS_ASSERT_EQUALS(fn->hasAttribute(attribute), true);
    }

    TS_ASSERT_THROWS_NOTHING(
        fn->setAttribute(decoratorAttributes[0], IFunction::Attribute(4.0)));
    TS_ASSERT_EQUALS(
        fn->getAttribute(decoratorAttributes[0]).value(),
        decoratedFunction->getAttribute(decoratorAttributes[0]).value());
  }

  void testTies() {
    TestableFunctionParameterDecorator invalidFn;
    TS_ASSERT_THROWS(invalidFn.tie("Name", "a=b"), std::runtime_error);
    TS_ASSERT_THROWS(invalidFn.applyTies(), std::runtime_error);
    TS_ASSERT_THROWS(invalidFn.clearTies(), std::runtime_error);
    TS_ASSERT_THROWS(invalidFn.removeTie(0), std::runtime_error);
    TS_ASSERT_THROWS(invalidFn.getTie(0), std::runtime_error);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();
    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();

    ParameterTie *tie = fn->tie("Height", "Height=2.0*Sigma");
    TS_ASSERT(tie);
    TS_ASSERT_EQUALS(decoratedFunction->getTie(0), tie);

    TS_ASSERT_THROWS_NOTHING(fn->clearTies());
    TS_ASSERT_THROWS_NOTHING(fn->addTies("Height=4.0*Sigma"));
    TS_ASSERT_EQUALS(fn->getTie(0), decoratedFunction->getTie(0));
    TS_ASSERT(fn->getTie(0));
    TS_ASSERT_THROWS_NOTHING(fn->removeTie(0));

    tie = fn->getTie(0);
    TS_ASSERT(!tie);
  }

  void testConstraints() {
    TestableFunctionParameterDecorator invalidFn;
    TS_ASSERT_THROWS(invalidFn.addConstraints("0<a<1"), std::runtime_error);
    TS_ASSERT_THROWS(invalidFn.getConstraint(0), std::runtime_error);
    TS_ASSERT_THROWS(invalidFn.removeConstraint("Height"), std::runtime_error);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();
    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();

    TS_ASSERT_THROWS_NOTHING(fn->addConstraints("0.0<Height<10.0"));
    TS_ASSERT_EQUALS(fn->getConstraint(0), decoratedFunction->getConstraint(0));
    TS_ASSERT(fn->getConstraint(0));
  }

  void testParameterNames() {
    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();

    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();

    std::vector<std::string> decoratorNames = fn->getParameterNames();
    std::vector<std::string> wrappedNames =
        decoratedFunction->getParameterNames();

    TS_ASSERT_EQUALS(decoratorNames.size(), wrappedNames.size());
    TS_ASSERT_EQUALS(wrappedNames.size(), 3);

    for (size_t i = 0; i < decoratorNames.size(); ++i) {
      TS_ASSERT_EQUALS(decoratorNames[i], wrappedNames[i]);
    }
  }

  void testSetParameterDescription() {
    TestableFunctionParameterDecorator invalidFn;
    TS_ASSERT_THROWS(invalidFn.setParameterDescription(0, "None"),
                     std::runtime_error);
    TS_ASSERT_THROWS(invalidFn.parameterDescription(0), std::runtime_error);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();

    TS_ASSERT_THROWS_NOTHING(fn->setParameterDescription(0, "None"));

    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();
    TS_ASSERT_EQUALS(fn->parameterDescription(0),
                     decoratedFunction->parameterDescription(0));
    TS_ASSERT_EQUALS(fn->parameterDescription(0), "None");
    TS_ASSERT_THROWS(fn->parameterDescription(10), std::out_of_range);

    TS_ASSERT_THROWS_NOTHING(
        fn->setParameterDescription("Height", "Something"));
    TS_ASSERT_THROWS(fn->setParameterDescription("DoesNotExist", "Something"),
                     std::invalid_argument);
  }

  void testBeforeDecoratedFunctionSetIsCalled() {
    MockTestableFunctionParameterDecorator fn;
    EXPECT_CALL(fn, beforeDecoratedFunctionSet(_)).Times(1);

    fn.setDecoratedFunction("Gaussian");

    TS_ASSERT(Mock::VerifyAndClearExpectations(&fn));
  }

  void testClone() {
    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();

    fn->setParameter("Height", 3.0);
    fn->setParameter("PeakCentre", 0.5);
    fn->setParameter("Sigma", 0.3);

    IFunction_sptr cloned = fn->clone();

    TS_ASSERT(cloned);

    FunctionParameterDecorator_sptr castedClone =
        boost::dynamic_pointer_cast<FunctionParameterDecorator>(cloned);
    TS_ASSERT(castedClone);
    TS_ASSERT_EQUALS(cloned->name(), fn->name());

    TS_ASSERT_EQUALS(cloned->getParameter("Height"), 3.0);
    TS_ASSERT_EQUALS(cloned->getParameter("PeakCentre"), 0.5);
    TS_ASSERT_EQUALS(cloned->getParameter("Sigma"), 0.3);
  }

private:
  FunctionParameterDecorator_sptr getFunctionParameterDecoratorGaussian() {
    FunctionParameterDecorator_sptr fn =
        boost::make_shared<TestableFunctionParameterDecorator>();
    fn->setDecoratedFunction("Gaussian");

    return fn;
  }

  class MockTestableFunctionParameterDecorator
      : public TestableFunctionParameterDecorator {
  public:
    MOCK_METHOD1(beforeDecoratedFunctionSet, void(const IFunction_sptr &));
  };
};

#endif /* MANTID_API_WRAPPEDFUNCTIONTEST_H_ */
