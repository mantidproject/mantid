// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/ParamFunction.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::API;

class FunctionFactoryTest_FunctA : public ParamFunction, public IFunction1D {
  int m_attr;

public:
  FunctionFactoryTest_FunctA() {
    declareParameter("a0");
    declareParameter("a1");
  }
  std::string name() const override { return "FunctionFactoryTest_FunctA"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
  }
  void functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) override {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
  }
  bool hasAttribute(const std::string &attName) const override {
    if (attName == "attr")
      return true;
    return false;
  }
  Attribute getAttribute(const std::string &attName) const override {
    if (attName == "attr")
      return Attribute(m_attr);
    return getAttribute(attName);
  }
  void setAttribute(const std::string &attName, const Attribute &value) override {
    if (attName == "attr") {
      int n = value.asInt();
      if (n > 0) {
        m_attr = n;
        clearAllParameters();
        for (int i = 0; i < n; i++) {
          std::ostringstream ostr;
          ostr << "at_" << i;
          declareParameter(ostr.str());
        }
      }
    } else {
      setAttribute(attName, value);
    }
  }
};

class FunctionFactoryTest_FunctB : public ParamFunction, public IFunction1D {
public:
  FunctionFactoryTest_FunctB() {
    declareParameter("b0");
    declareParameter("b1");
  }

  std::string name() const override { return "FunctionFactoryTest_FunctB"; }

  void function1D(double *out, const double *xValues, const size_t nData) const override {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
  }
  void functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) override {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
  }
};

class FunctionFactoryTest_CompFunctA : public CompositeFunction {
  std::string m_attr;

public:
  FunctionFactoryTest_CompFunctA() {}

  std::string name() const override { return "FunctionFactoryTest_CompFunctA"; }

  bool hasAttribute(const std::string &attName) const override {
    if (attName == "attr")
      return true;
    return false;
  }
  Attribute getAttribute(const std::string &attName) const override {
    if (attName == "attr")
      return Attribute(m_attr);
    return getAttribute(attName);
  }
  void setAttribute(const std::string &attName, const Attribute &value) override {
    UNUSED_ARG(attName);
    m_attr = value.asString();
  }
};

class FunctionFactoryTest_CompFunctB : public CompositeFunction {
public:
  FunctionFactoryTest_CompFunctB() {}

  std::string name() const override { return "FunctionFactoryTest_CompFunctB"; }
};

DECLARE_FUNCTION(FunctionFactoryTest_FunctA)
DECLARE_FUNCTION(FunctionFactoryTest_FunctB)
DECLARE_FUNCTION(FunctionFactoryTest_CompFunctA)
DECLARE_FUNCTION(FunctionFactoryTest_CompFunctB)

class FunctionFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FunctionFactoryTest *createSuite() { return new FunctionFactoryTest(); }
  static void destroySuite(FunctionFactoryTest *suite) { delete suite; }

  FunctionFactoryTest() { Mantid::API::FrameworkManager::Instance(); }

  void testCreateFunction() {
    IFunction_sptr funa = FunctionFactory::Instance().createFunction("FunctionFactoryTest_FunctA");
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0), "a0");
    TS_ASSERT_EQUALS(funa->parameterName(1), "a1");
    TS_ASSERT_EQUALS(funa->nParams(), 2);

    IFunction_sptr funb = FunctionFactory::Instance().createFunction("FunctionFactoryTest_FunctB");
    TS_ASSERT(funb);
    TS_ASSERT_EQUALS(funb->parameterName(0), "b0");
    TS_ASSERT_EQUALS(funb->parameterName(1), "b1");
    TS_ASSERT_EQUALS(funb->nParams(), 2);
  }

  void testCreateSimpleDefault() {
    std::string fnString = "name=FunctionFactoryTest_FunctA";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0), "a0");
    TS_ASSERT_EQUALS(funa->parameterName(1), "a1");
    TS_ASSERT_EQUALS(funa->nParams(), 2);
  }

  void testCreateSimple() {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0), "a0");
    TS_ASSERT_EQUALS(funa->parameterName(1), "a1");
    TS_ASSERT_EQUALS(funa->nParams(), 2);
    TS_ASSERT_EQUALS(funa->getParameter("a0"), 0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"), 1.1);
  }

  void testCreateSimpleWithAttribute() {
    std::string fnString = "name=FunctionFactoryTest_FunctA,attr=\"3\",at_0=0.1,at_1=1.1,at_2=2.1";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0), "at_0");
    TS_ASSERT_EQUALS(funa->parameterName(1), "at_1");
    TS_ASSERT_EQUALS(funa->parameterName(2), "at_2");
    TS_ASSERT_EQUALS(funa->nParams(), 3);
    TS_ASSERT_EQUALS(funa->getParameter(0), 0.1);
    TS_ASSERT_EQUALS(funa->getParameter(1), 1.1);
    TS_ASSERT_EQUALS(funa->getParameter(2), 2.1);
  }

  void testCreateComposite() {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1;name="
                           "FunctionFactoryTest_FunctB,b0=0.2,b1=1.2";

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    CompositeFunction_sptr cf = std::dynamic_pointer_cast<CompositeFunction>(fun);
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(cf->nParams(), 4);
    TS_ASSERT_EQUALS(cf->parameterName(0), "f0.a0");
    TS_ASSERT_EQUALS(cf->parameterName(1), "f0.a1");
    TS_ASSERT_EQUALS(cf->parameterName(2), "f1.b0");
    TS_ASSERT_EQUALS(cf->parameterName(3), "f1.b1");
    TS_ASSERT_EQUALS(cf->getParameter(0), 0.1);
    TS_ASSERT_EQUALS(cf->getParameter(1), 1.1);
    TS_ASSERT_EQUALS(cf->getParameter(2), 0.2);
    TS_ASSERT_EQUALS(cf->getParameter(3), 1.2);
  }

  void testCreateComposite1() {
    std::string fnString = "name=FunctionFactoryTest_FunctA;name="
                           "FunctionFactoryTest_FunctB,b0=0.2,b1=1.2";

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    CompositeFunction_sptr cf = std::dynamic_pointer_cast<CompositeFunction>(fun);
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(cf->nParams(), 4);
    TS_ASSERT_EQUALS(cf->parameterName(0), "f0.a0");
    TS_ASSERT_EQUALS(cf->parameterName(1), "f0.a1");
    TS_ASSERT_EQUALS(cf->parameterName(2), "f1.b0");
    TS_ASSERT_EQUALS(cf->parameterName(3), "f1.b1");
    TS_ASSERT_EQUALS(cf->getParameter(0), 0.);
    TS_ASSERT_EQUALS(cf->getParameter(1), 0.);
    TS_ASSERT_EQUALS(cf->getParameter(2), 0.2);
    TS_ASSERT_EQUALS(cf->getParameter(3), 1.2);
  }

  void testCreateComposite2() {
    std::string fnString = "composite=FunctionFactoryTest_CompFunctB;";
    fnString += "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_"
                "FunctB,b0=0.2,b1=1.2";

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    FunctionFactoryTest_CompFunctB *cf = dynamic_cast<FunctionFactoryTest_CompFunctB *>(fun.get());
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(cf->nParams(), 4);
    TS_ASSERT_EQUALS(cf->parameterName(0), "f0.a0");
    TS_ASSERT_EQUALS(cf->parameterName(1), "f0.a1");
    TS_ASSERT_EQUALS(cf->parameterName(2), "f1.b0");
    TS_ASSERT_EQUALS(cf->parameterName(3), "f1.b1");
    TS_ASSERT_EQUALS(cf->getParameter(0), 0.);
    TS_ASSERT_EQUALS(cf->getParameter(1), 0.);
    TS_ASSERT_EQUALS(cf->getParameter(2), 0.2);
    TS_ASSERT_EQUALS(cf->getParameter(3), 1.2);
    TS_ASSERT_EQUALS(fun->name(), "FunctionFactoryTest_CompFunctB");
  }

  void testCreateComposite3() {
    std::string fnString = "composite=FunctionFactoryTest_CompFunctA,attr = \"hello\";";
    fnString += "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_"
                "FunctB,b0=0.2,b1=1.2";

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    FunctionFactoryTest_CompFunctA *cf = dynamic_cast<FunctionFactoryTest_CompFunctA *>(fun.get());
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(cf->nParams(), 4);
    TS_ASSERT_EQUALS(cf->parameterName(0), "f0.a0");
    TS_ASSERT_EQUALS(cf->parameterName(1), "f0.a1");
    TS_ASSERT_EQUALS(cf->parameterName(2), "f1.b0");
    TS_ASSERT_EQUALS(cf->parameterName(3), "f1.b1");
    TS_ASSERT_EQUALS(cf->getParameter(0), 0.);
    TS_ASSERT_EQUALS(cf->getParameter(1), 0.);
    TS_ASSERT_EQUALS(cf->getParameter(2), 0.2);
    TS_ASSERT_EQUALS(cf->getParameter(3), 1.2);
    TS_ASSERT_EQUALS(fun->name(), "FunctionFactoryTest_CompFunctA");
    TS_ASSERT(fun->hasAttribute("attr"));
    TS_ASSERT_EQUALS(fun->getAttribute("attr").asString(), "hello");
  }

  void testCreateCompositeNested() {
    std::string fnString = "(composite=FunctionFactoryTest_CompFunctA,attr = hello;";
    fnString += "name=FunctionFactoryTest_FunctA;name=FunctionFactoryTest_"
                "FunctB,b0=0.2,b1=1.2);";
    fnString += "(composite=FunctionFactoryTest_CompFunctB;";
    fnString += "name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2;name="
                "FunctionFactoryTest_FunctA)";

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);

    CompositeFunction *cf = dynamic_cast<CompositeFunction *>(fun.get());
    TS_ASSERT(cf);
    TS_ASSERT_EQUALS(cf->nFunctions(), 2);
    TS_ASSERT_EQUALS(cf->getFunction(0)->name(), "FunctionFactoryTest_CompFunctA");
    TS_ASSERT_EQUALS(cf->getFunction(1)->name(), "FunctionFactoryTest_CompFunctB");
    TS_ASSERT_EQUALS(dynamic_cast<CompositeFunction *>(cf->getFunction(0).get())->nFunctions(), 2);
    TS_ASSERT_EQUALS(dynamic_cast<CompositeFunction *>(cf->getFunction(1).get())->nFunctions(), 2);
  }

  void testCreateWithTies() {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1,ties=(a0=a1^2)";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_DELTA(funa->getParameter("a0"), 1.21, 0.0001);
    TS_ASSERT_EQUALS(funa->getParameter("a1"), 1.1);
  }

  void testCreateWithTies1() {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1,ties=(a0=a1=4)";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->getParameter("a0"), 4);
    TS_ASSERT_EQUALS(funa->getParameter("a1"), 4);
  }

  void testCreateWithTies2() {
    std::string fnString = "name=FunctionFactoryTest_FunctA,a0=0.1,a1=1.1,ties=(a0=2,a1=4)";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->getParameter("a0"), 2);
    TS_ASSERT_EQUALS(funa->getParameter("a1"), 4);
  }

  void testCreateWithTies3() {
    std::string fnString = "name=FunctionFactoryTest_FunctA,ties=(a0=2,a1=4*(2+2))";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->getParameter("a0"), 2);
    TS_ASSERT_EQUALS(funa->getParameter("a1"), 16);
  }

  void testCreateWithTies4() {
    std::string fnString = "name=FunctionFactoryTest_FunctA,ties=(a0=2,a1=a0/(2*2))";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->getParameter("a0"), 2);
    TS_ASSERT_EQUALS(funa->getParameter("a1"), 0.5);
  }

  void testCreateCompositeWithTies() {
    std::string fnString = "name=FunctionFactoryTest_FunctA,ties=(a0=a1=14);"
                           "name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2;ties="
                           "(f1.b0=f0.a0+f0.a1)";

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    TS_ASSERT_EQUALS(fun->getParameter(0), 14.);
    TS_ASSERT_EQUALS(fun->getParameter(1), 14.);
    TS_ASSERT_EQUALS(fun->getParameter(2), 28.);
    TS_ASSERT_EQUALS(fun->getParameter(3), 1.2);

    IFunction_sptr fun1 = FunctionFactory::Instance().createInitialized(fun->asString());

    fun1->setParameter(0, 1.);
    fun1->setParameter(1, 2.);
    fun1->setParameter(2, 0.);
    fun1->setParameter(3, 789);

    TS_ASSERT_EQUALS(fun1->getParameter(0), 1.);
    TS_ASSERT_EQUALS(fun1->getParameter(1), 2.);
    TS_ASSERT_EQUALS(fun1->getParameter(2), 0.);
    TS_ASSERT_EQUALS(fun1->getParameter(3), 789);

    fun1->applyTies();

    TS_ASSERT_EQUALS(fun1->getParameter(0), 1.);
    TS_ASSERT_EQUALS(fun1->getParameter(1), 2.);
    TS_ASSERT_EQUALS(fun1->getParameter(2), 3.);
    TS_ASSERT_EQUALS(fun1->getParameter(3), 789);
  }

  void testCreateCompositeWithTies1() {
    std::string fnString = "name=FunctionFactoryTest_FunctA,ties=(a0=a1=16);"
                           "name=FunctionFactoryTest_FunctB,b0=0.2,b1=1.2;ties="
                           "(f1.b1=f0.a1/(2*2))";

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    TS_ASSERT_EQUALS(fun->getParameter(0), 16.);
    TS_ASSERT_EQUALS(fun->getParameter(1), 16.);
    TS_ASSERT_EQUALS(fun->getParameter(2), 0.2);
    TS_ASSERT_EQUALS(fun->getParameter(3), 4.);
  }

  void test_MultiDomainFunction_creation() {
    const std::string fnString = "composite=MultiDomainFunction;"
                                 "name=FunctionFactoryTest_FunctA;"
                                 "name=FunctionFactoryTest_FunctB";
    IFunction_sptr fun;
    TS_ASSERT_THROWS_NOTHING(fun = FunctionFactory::Instance().createInitialized(fnString));
    TS_ASSERT(fun);
    const auto mdfunc = std::dynamic_pointer_cast<MultiDomainFunction>(fun);
    TS_ASSERT(mdfunc);
    if (mdfunc) {
      TS_ASSERT_EQUALS(mdfunc->nFunctions(), 2);
      const auto funcA = mdfunc->getFunction(0);
      const auto funcB = mdfunc->getFunction(1);
      TS_ASSERT_EQUALS(funcA->name(), "FunctionFactoryTest_FunctA");
      TS_ASSERT_EQUALS(funcB->name(), "FunctionFactoryTest_FunctB");
    }
  }

  void test_MultiDomainFunction_creation_moreComplex() {
    const std::string fnString = "composite=MultiDomainFunction,NumDeriv=true;(name=FunctionFactoryTest_"
                                 "FunctA,a0=0,a1=0.5;name=FunctionFactoryTest_FunctB,b0=0.1,b1=0.2,ties="
                                 "(b1=0.2),$domains=i);(name=FunctionFactoryTest_FunctA,a0=0,a1=0.5;"
                                 "name=FunctionFactoryTest_FunctB,b0=0.1,b1=0.2,$domains=i);ties=(f1.f1."
                                 "b1=f0.f1.b1)";
    IFunction_sptr fun;
    TS_ASSERT_THROWS_NOTHING(fun = FunctionFactory::Instance().createInitialized(fnString));
    TS_ASSERT(fun);
    const auto mdfunc = std::dynamic_pointer_cast<MultiDomainFunction>(fun);
    TS_ASSERT(mdfunc);
    if (mdfunc) {
      TS_ASSERT_EQUALS(mdfunc->asString(), "composite=MultiDomainFunction,NumDeriv=true;(composite="
                                           "CompositeFunction,NumDeriv=false,$domains=i;name="
                                           "FunctionFactoryTest_FunctA,a0=0,a1=0.5;name="
                                           "FunctionFactoryTest_FunctB,b0=0.1,b1=0.2,ties=(b1=0.2))"
                                           ";(composite=CompositeFunction,NumDeriv=false,$domains="
                                           "i;name=FunctionFactoryTest_FunctA,a0=0,a1=0.5;name="
                                           "FunctionFactoryTest_FunctB,b0=0.1,b1=0.2);ties=(f1.f1."
                                           "b1=f0.f1.b1)");
      TS_ASSERT_EQUALS(mdfunc->nFunctions(), 2);

      // test the domains for each function
      std::vector<size_t> domainsFirstFunc, domainsSecondFunc;
      mdfunc->getDomainIndices(0, 1, domainsFirstFunc);
      mdfunc->getDomainIndices(1, 1, domainsSecondFunc);
      TS_ASSERT_EQUALS(domainsFirstFunc, std::vector<size_t>{0});
      TS_ASSERT_EQUALS(domainsSecondFunc, std::vector<size_t>{1});

      // test composite functions
      const auto first = std::dynamic_pointer_cast<CompositeFunction>(mdfunc->getFunction(0));
      const auto second = std::dynamic_pointer_cast<CompositeFunction>(mdfunc->getFunction(1));
      TS_ASSERT(first);
      TS_ASSERT(second);

      // test each individual function
      auto testFunc = [](const CompositeFunction_sptr &f) {
        if (f) {
          TS_ASSERT_EQUALS(f->nFunctions(), 2);
          TS_ASSERT_EQUALS(f->getFunction(0)->name(), "FunctionFactoryTest_FunctA");
          TS_ASSERT_EQUALS(f->getFunction(1)->name(), "FunctionFactoryTest_FunctB");
        }
      };
      testFunc(first);
      testFunc(second);
    }
  }

  void test_getFunctionNames() {
    const auto &names = FunctionFactory::Instance().getFunctionNames<IFunction1D>();
    TS_ASSERT(!names.empty());
    TS_ASSERT(std::find(names.begin(), names.end(), "FunctionFactoryTest_FunctA") != names.end());
    // Call it again to indirectly test caching
    TS_ASSERT_EQUALS(names, FunctionFactory::Instance().getFunctionNames<IFunction1D>());
  }

  void test_getFunctionNamesGUI() {
    const auto &names = FunctionFactory::Instance().getFunctionNamesGUI();
    TS_ASSERT(!names.empty());
    auto i = std::find_if(names.begin(), names.end(),
                          [](const std::string &name) { return name.find("CrystalField") != std::string::npos; });
    TS_ASSERT(i == names.end());
  }
};
