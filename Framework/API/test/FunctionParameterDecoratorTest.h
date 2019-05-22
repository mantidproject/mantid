// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_WRAPPEDFUNCTIONTEST_H_
#define MANTID_API_WRAPPEDFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionParameterDecorator.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/make_shared.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

using ::testing::_;
using ::testing::Mock;

class FunctionParameterDecoratorTest;

class TestableFunctionParameterDecorator : public FunctionParameterDecorator {
  friend class FunctionParameterDecoratorTest;

public:
  TestableFunctionParameterDecorator() {}
  ~TestableFunctionParameterDecorator() override {}

  std::string name() const override {
    return "TestableFunctionParameterDecorator";
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

DECLARE_FUNCTION(TestableFunctionParameterDecorator)

class FunctionWithParameters : public ParamFunction {
public:
  FunctionWithParameters() : ParamFunction(), m_workspace() {}

  std::string name() const override { return "FunctionWithParameters"; }

  void init() override {
    declareParameter("Height");
    declareParameter("PeakCentre");
    declareParameter("Sigma");
  }

  void function(const FunctionDomain &domain,
                FunctionValues &values) const override {
    UNUSED_ARG(domain);
    UNUSED_ARG(values);
    // Does nothing, not required for this test.
  }

  void setWorkspace(boost::shared_ptr<const Workspace> ws) override {
    m_workspace = ws;
  }

  Workspace_const_sptr getWorkspace() const { return m_workspace; }

private:
  Workspace_const_sptr m_workspace;
};
DECLARE_FUNCTION(FunctionWithParameters)

class FunctionWithAttributes : public ParamFunction {
public:
  FunctionWithAttributes() : ParamFunction() {}

  std::string name() const override { return "FunctionWithAttributes"; }

  void init() override {
    declareParameter("PeakCentre");
    declareParameter("Sigma");
    declareParameter("Height");

    declareAttribute("Attribute1", IFunction::Attribute(1));
    declareAttribute("Attribute2", IFunction::Attribute("Test"));
  }

  void function(const FunctionDomain &domain,
                FunctionValues &values) const override {
    UNUSED_ARG(domain);
    UNUSED_ARG(values);
    // Does nothing, not required for this test.
  }
};

DECLARE_FUNCTION(FunctionWithAttributes)

class FunctionParameterDecoratorTest : public CxxTest::TestSuite {
public:
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

    TS_ASSERT_THROWS_NOTHING(fn.setDecoratedFunction("FunctionWithParameters"));

    IFunction_sptr decorated = fn.getDecoratedFunction();
    TS_ASSERT(decorated);
    TS_ASSERT_EQUALS(decorated->name(), "FunctionWithParameters");
  }

  void testSetDecoratedFunctionInvalidName() {
    TestableFunctionParameterDecorator fn;
    TS_ASSERT_THROWS(fn.setDecoratedFunction("INVALIDFUNCTION"),
                     const Exception::NotFoundError &);
    TS_ASSERT(!fn.getDecoratedFunction());
  }

  void testThrowIfNoFunctionSet() {
    TestableFunctionParameterDecorator fn;
    TS_ASSERT_THROWS(fn.throwIfNoFunctionSet(), const std::runtime_error &);
    fn.setDecoratedFunction("FunctionWithParameters");
    TS_ASSERT_THROWS_NOTHING(fn.throwIfNoFunctionSet());
  }

  void testNParams() {
    TestableFunctionParameterDecorator invalidFn;
    TS_ASSERT_EQUALS(invalidFn.nParams(), 0);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();
    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();

    TS_ASSERT_EQUALS(fn->nParams(), decoratedFunction->nParams());
  }

  void testHasParameter() {
    TestableFunctionParameterDecorator fn;

    fn.setDecoratedFunction("FunctionWithParameters");

    TS_ASSERT(fn.hasParameter("Height"));
    TS_ASSERT(fn.hasParameter("PeakCentre"));
    TS_ASSERT(fn.hasParameter("Sigma"));
    TS_ASSERT(!fn.hasParameter("Hello"));
  }

  void testGetSetParameter() {
    TestableFunctionParameterDecorator invalidFn;
    TS_ASSERT_THROWS(invalidFn.setParameter(0, 2.0),
                     const std::runtime_error &);
    TS_ASSERT_THROWS(invalidFn.getParameter(0), const std::runtime_error &);
    TS_ASSERT_THROWS(invalidFn.setParameter("Height", 2.0),
                     const std::runtime_error &);
    TS_ASSERT_THROWS(invalidFn.getParameter("Height"),
                     const std::runtime_error &);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();

    TS_ASSERT_THROWS_NOTHING(fn->setParameter(0, 2.0));

    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();
    TS_ASSERT_EQUALS(fn->getParameter(0), decoratedFunction->getParameter(0));
    TS_ASSERT_EQUALS(fn->getParameter(0), 2.0);
    TS_ASSERT_THROWS(fn->getParameter(10), const std::out_of_range &);

    TS_ASSERT_THROWS_NOTHING(fn->setParameter("Height", 4.0));
    TS_ASSERT_EQUALS(fn->getParameter("Height"),
                     decoratedFunction->getParameter("Height"));
    TS_ASSERT_EQUALS(fn->getParameter("Height"), 4.0);
    TS_ASSERT_THROWS(fn->getParameter("DoesNotExist"),
                     const std::invalid_argument &);
  }

  void testExplicitelySet() {
    TestableFunctionParameterDecorator invalidFn;
    TS_ASSERT_THROWS(invalidFn.isExplicitlySet(0), const std::runtime_error &);

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
    TS_ASSERT_THROWS(invalidFn.getError(0), const std::runtime_error &);
    TS_ASSERT_THROWS(invalidFn.setError(0, 2.0), const std::runtime_error &);

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
    TS_ASSERT_THROWS(invalidFn.isFixed(0), const std::runtime_error &);
    TS_ASSERT_THROWS(invalidFn.fix(0), const std::runtime_error &);
    TS_ASSERT_THROWS(invalidFn.unfix(0), const std::runtime_error &);

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
    TS_ASSERT_EQUALS(invalidFn.nAttributes(), 0);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();
    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();

    TS_ASSERT_EQUALS(fn->nAttributes(), decoratedFunction->nAttributes());
    TS_ASSERT_EQUALS(fn->nAttributes(), 0);

    fn->setDecoratedFunction("FunctionWithAttributes");
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
    TS_ASSERT_THROWS(invalidFn.tie("Name", "a=b"), const std::runtime_error &);
    TS_ASSERT_THROWS(invalidFn.applyTies(), const std::runtime_error &);
    TS_ASSERT_THROWS(invalidFn.clearTies(), const std::runtime_error &);
    TS_ASSERT_THROWS(invalidFn.removeTie(0), const std::runtime_error &);
    TS_ASSERT_THROWS(invalidFn.getTie(0), const std::runtime_error &);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();
    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();

    fn->tie("Height", "Height=2.0*Sigma");
    ParameterTie *tie = fn->getTie(fn->parameterIndex("Height"));
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

  void testTiesInComposite() {
    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();

    CompositeFunction_sptr composite = boost::make_shared<CompositeFunction>();
    composite->addFunction(fn);

    TS_ASSERT_THROWS_NOTHING(composite->addTies("f0.Height=2.0*f0.Sigma"));

    composite->setParameter("f0.Sigma", 3.0);
    composite->applyTies();
    TS_ASSERT_EQUALS(composite->getParameter("f0.Height"), 6.0);
  }

  void testTiesInWrappedComposite() {
    FunctionParameterDecorator_sptr outer =
        boost::make_shared<TestableFunctionParameterDecorator>();
    outer->setDecoratedFunction("CompositeFunction");

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();

    CompositeFunction_sptr composite =
        boost::dynamic_pointer_cast<CompositeFunction>(
            outer->getDecoratedFunction());
    composite->addFunction(fn);

    TS_ASSERT_THROWS_NOTHING(outer->addTies("f0.Height=2.0*f0.Sigma"));

    outer->setParameter("f0.Sigma", 3.0);
    outer->applyTies();
    TS_ASSERT_EQUALS(outer->getParameter("f0.Height"), 6.0);
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
                     const std::runtime_error &);
    TS_ASSERT_THROWS(invalidFn.parameterDescription(0),
                     const std::runtime_error &);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();

    TS_ASSERT_THROWS_NOTHING(fn->setParameterDescription(0, "None"));

    IFunction_sptr decoratedFunction = fn->getDecoratedFunction();
    TS_ASSERT_EQUALS(fn->parameterDescription(0),
                     decoratedFunction->parameterDescription(0));
    TS_ASSERT_EQUALS(fn->parameterDescription(0), "None");
    TS_ASSERT_THROWS(fn->parameterDescription(10), const std::out_of_range &);

    TS_ASSERT_THROWS_NOTHING(
        fn->setParameterDescription("Height", "Something"));
    TS_ASSERT_THROWS(fn->setParameterDescription("DoesNotExist", "Something"),
                     const std::invalid_argument &);
  }

  void testBeforeDecoratedFunctionSetIsCalled() {
    MockTestableFunctionParameterDecorator fn;
    EXPECT_CALL(fn, beforeDecoratedFunctionSet(_)).Times(1);

    fn.setDecoratedFunction("FunctionWithParameters");

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

  void testSetWorkspace() {
    // using WorkspaceGroup because it is in API
    Workspace_const_sptr ws = boost::make_shared<const WorkspaceGroup>();

    TestableFunctionParameterDecorator invalidFn;
    TS_ASSERT_THROWS(invalidFn.setWorkspace(ws), const std::runtime_error &);

    FunctionParameterDecorator_sptr fn =
        getFunctionParameterDecoratorGaussian();

    TS_ASSERT_THROWS_NOTHING(fn->setWorkspace(ws));

    boost::shared_ptr<FunctionWithParameters> decorated =
        boost::dynamic_pointer_cast<FunctionWithParameters>(
            fn->getDecoratedFunction());

    TS_ASSERT_EQUALS(decorated->getWorkspace(), ws);
  }

private:
  FunctionParameterDecorator_sptr getFunctionParameterDecoratorGaussian() {
    FunctionParameterDecorator_sptr fn =
        boost::make_shared<TestableFunctionParameterDecorator>();
    fn->setDecoratedFunction("FunctionWithParameters");

    return fn;
  }

  class MockTestableFunctionParameterDecorator
      : public TestableFunctionParameterDecorator {
  public:
    GNU_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_METHOD1(beforeDecoratedFunctionSet, void(const IFunction_sptr &));
    GNU_DIAG_ON_SUGGEST_OVERRIDE
  };
};

#endif /* MANTID_API_WRAPPEDFUNCTIONTEST_H_ */
