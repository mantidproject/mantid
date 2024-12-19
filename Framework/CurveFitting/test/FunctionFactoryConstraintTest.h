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
#include "MantidAPI/ParamFunction.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::API;

class FunctionFactoryConstraintTest_FunctA : public ParamFunction, public IFunction1D {
  int m_attr;

public:
  FunctionFactoryConstraintTest_FunctA() {
    declareParameter("a0");
    declareParameter("a1");
  }
  std::string name() const override { return "FunctionFactoryConstraintTest_FunctA"; }
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

class FunctionFactoryConstraintTest_FunctB : public ParamFunction, public IFunction1D {
public:
  FunctionFactoryConstraintTest_FunctB() {
    declareParameter("b0");
    declareParameter("b1");
  }

  std::string name() const override { return "FunctionFactoryConstraintTest_FunctB"; }

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

class FunctionFactoryConstraintTest_CompFunctA : public CompositeFunction {
  std::string m_attr;

public:
  FunctionFactoryConstraintTest_CompFunctA() {}

  std::string name() const override { return "FunctionFactoryConstraintTest_CompFunctA"; }

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

class FunctionFactoryConstraintTest_CompFunctB : public CompositeFunction {
public:
  FunctionFactoryConstraintTest_CompFunctB() {}

  std::string name() const override { return "FunctionFactoryConstraintTest_CompFunctB"; }
};

DECLARE_FUNCTION(FunctionFactoryConstraintTest_FunctA)
DECLARE_FUNCTION(FunctionFactoryConstraintTest_FunctB)
DECLARE_FUNCTION(FunctionFactoryConstraintTest_CompFunctA)
DECLARE_FUNCTION(FunctionFactoryConstraintTest_CompFunctB)

class FunctionFactoryConstraintTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FunctionFactoryConstraintTest *createSuite() { return new FunctionFactoryConstraintTest(); }
  static void destroySuite(FunctionFactoryConstraintTest *suite) { delete suite; }

  FunctionFactoryConstraintTest() { Mantid::API::FrameworkManager::Instance(); }

  void testCreateWithConstraint1() {
    std::string fnString = "name=FunctionFactoryConstraintTest_FunctA,a0=0.1,"
                           "a1=1.1,constraint=0<a0<0.2";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0), "a0");
    TS_ASSERT_EQUALS(funa->parameterName(1), "a1");
    TS_ASSERT_EQUALS(funa->nParams(), 2);
    TS_ASSERT_EQUALS(funa->getParameter("a0"), 0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"), 1.1);

    IConstraint *c = funa->getConstraint(0);
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(), 0);

    funa->setParameter("a0", 1);
    TS_ASSERT_EQUALS(c->check(), 640);

    funa->setParameter("a0", -1);
    TS_ASSERT_EQUALS(c->check(), 1000);

    TS_ASSERT_EQUALS(funa->asString(), "name=FunctionFactoryConstraintTest_FunctA,a0=-1,"
                                       "a1=1.1,constraints=(0<a0<0.2)");
    funa->setConstraintPenaltyFactor("a0", 10.2);
    TS_ASSERT_EQUALS(funa->asString(), "name=FunctionFactoryConstraintTest_FunctA,a0=-1,"
                                       "a1=1.1,constraints=(0<a0<0.2,penalty=10.2)");
  }

  void testCreateWithConstraint2() {
    std::string fnString = "name=FunctionFactoryConstraintTest_FunctA,a0=0.1,a1=1.1,"
                           "constraints=(0<a0<0.2,a1>10)";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->parameterName(0), "a0");
    TS_ASSERT_EQUALS(funa->parameterName(1), "a1");
    TS_ASSERT_EQUALS(funa->nParams(), 2);
    TS_ASSERT_EQUALS(funa->getParameter("a0"), 0.1);
    TS_ASSERT_EQUALS(funa->getParameter("a1"), 1.1);

    IConstraint *c0 = funa->getConstraint(0);
    TS_ASSERT(c0);
    TS_ASSERT_EQUALS(c0->check(), 0);

    funa->setParameter("a0", 1);
    TS_ASSERT_EQUALS(c0->check(), 640);

    funa->setParameter("a0", -1);
    TS_ASSERT_EQUALS(c0->check(), 1000);

    IConstraint *c1 = funa->getConstraint(1);
    TS_ASSERT(c1);
    TS_ASSERT_EQUALS(c1->check(), 79210);

    funa->setParameter("a1", 11);
    TS_ASSERT_EQUALS(c1->check(), 0);

    funa->setConstraintPenaltyFactor("a1", 18.4);
    TS_ASSERT_EQUALS(funa->asString(), "name=FunctionFactoryConstraintTest_FunctA,a0=-1,"
                                       "a1=11,constraints=(0<a0<0.2,10<a1,penalty=18.4)");
  }

  void testSetConstraintPenaltyFactor1() {
    std::string fnString = "name=FunctionFactoryConstraintTest_FunctA,a0=0.1,a1=15.1,"
                           "constraints=(0<a0<0.2,a1>10,penalty=12.)";

    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);

    IConstraint *c0 = funa->getConstraint(0);
    TS_ASSERT(c0);
    TS_ASSERT_EQUALS(c0->check(), 0);
    TS_ASSERT_EQUALS(c0->getPenaltyFactor(), c0->getDefaultPenaltyFactor())

    IConstraint *c1 = funa->getConstraint(1);
    TS_ASSERT(c1);
    TS_ASSERT_EQUALS(c1->check(), 0);
    TS_ASSERT_EQUALS(c1->getPenaltyFactor(), 12.);

    TS_ASSERT_EQUALS(funa->asString(), "name=FunctionFactoryConstraintTest_FunctA,a0=0.1,"
                                       "a1=15.1,constraints=(0<a0<0.2,10<a1,penalty=12)");

    c1->setPenaltyFactor(c1->getDefaultPenaltyFactor());
    TS_ASSERT_EQUALS(c1->getPenaltyFactor(), c1->getDefaultPenaltyFactor());
    TS_ASSERT_EQUALS(funa->asString(), "name=FunctionFactoryConstraintTest_FunctA,a0=0.1,"
                                       "a1=15.1,constraints=(0<a0<0.2,10<a1)");

    c0->setPenaltyFactor(0.5);
    TS_ASSERT_EQUALS(c0->getPenaltyFactor(), 0.5);
    TS_ASSERT_EQUALS(funa->asString(), "name=FunctionFactoryConstraintTest_FunctA,a0=0.1,"
                                       "a1=15.1,constraints=(0<a0<0.2,penalty=0.5,10<a1)");

    funa->setParameter("a0", 0.5);
    TS_ASSERT_EQUALS(c0->check(), 0.045);
    TS_ASSERT_EQUALS(c0->getPenaltyFactor(), 0.5);
    TS_ASSERT_EQUALS(funa->asString(), "name=FunctionFactoryConstraintTest_FunctA,a0=0.5,"
                                       "a1=15.1,constraints=(0<a0<0.2,penalty=0.5,10<a1)");
  }

  void testSetConstraintPenaltyFactor2() {
    std::string fnString = "name=FunctionFactoryConstraintTest_FunctA,a0=0.1,a1=15.1";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);

    funa->addConstraints("0<a0<0.2,penalty=8,a1>10");
    IConstraint *c0 = funa->getConstraint(0);
    TS_ASSERT(c0);
    IConstraint *c1 = funa->getConstraint(1);
    TS_ASSERT(c1);
    TS_ASSERT_EQUALS(c0->getPenaltyFactor(), 8.);
    TS_ASSERT_EQUALS(c1->getPenaltyFactor(), c1->getDefaultPenaltyFactor());

    funa->clearConstraints();
    funa->addConstraints("0<a0<0.2,a1>10,penalty=0.1");
    IConstraint *c2 = funa->getConstraint(0);
    IConstraint *c3 = funa->getConstraint(1);
    TS_ASSERT(c2);
    TS_ASSERT(c3);
    TS_ASSERT_EQUALS(c2->asString(), "0<a0<0.2");
    TS_ASSERT_EQUALS(c3->asString(), "10<a1,penalty=0.1");
  }

  void testCreateCompositeWithConstraints() {
    std::string fnString = "composite=FunctionFactoryConstraintTest_CompFunctA,attr = \"hello\";"
                           "name=FunctionFactoryConstraintTest_FunctA;name="
                           "FunctionFactoryConstraintTest_FunctB,b0=0.2,b1=1.2,"
                           "constraints=(b0<1,b1>1)";

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    FunctionFactoryConstraintTest_CompFunctA *cf = dynamic_cast<FunctionFactoryConstraintTest_CompFunctA *>(fun.get());
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
    TS_ASSERT_EQUALS(fun->name(), "FunctionFactoryConstraintTest_CompFunctA");
    TS_ASSERT(fun->hasAttribute("attr"));
    TS_ASSERT_EQUALS(fun->getAttribute("attr").asString(), "hello");

    IConstraint *c = fun->getConstraint(2);
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(), 0);
    fun->setParameter("f1.b0", 2);
    TS_ASSERT_EQUALS(c->check(), 1000);

    c = fun->getConstraint(3);
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(), 0);
    fun->setParameter("f1.b1", 0.5);
    TS_ASSERT_EQUALS(c->check(), 250);
  }

  void testCreateCompositeWithConstraints1() {
    std::string fnString = "composite=FunctionFactoryConstraintTest_CompFunctA,attr = \"hello\";"
                           "name=FunctionFactoryConstraintTest_FunctA;name="
                           "FunctionFactoryConstraintTest_FunctB,b0=0.2,b1=1.2;"
                           "constraints=(f0.a0<1,f1.b1>1)";

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(fun);
    FunctionFactoryConstraintTest_CompFunctA *cf = dynamic_cast<FunctionFactoryConstraintTest_CompFunctA *>(fun.get());
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
    TS_ASSERT_EQUALS(fun->name(), "FunctionFactoryConstraintTest_CompFunctA");
    TS_ASSERT(fun->hasAttribute("attr"));
    TS_ASSERT_EQUALS(fun->getAttribute("attr").asString(), "hello");

    IConstraint *c = fun->getConstraint(0);
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(), 0);
    fun->setParameter("f0.a0", 2);
    TS_ASSERT_EQUALS(c->check(), 1000);

    c = fun->getConstraint(3);
    TS_ASSERT(c);
    TS_ASSERT_EQUALS(c->check(), 0);
    fun->setParameter("f1.b1", 0.5);
    TS_ASSERT_EQUALS(c->check(), 250);
  }

  void testCreateWithTies() {
    std::string fnString = "name=FunctionFactoryConstraintTest_FunctA,a0=0.1,"
                           "a1=1.1,ties=(a0=a1^2)";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_DELTA(funa->getParameter("a0"), 1.21, 0.0001);
    TS_ASSERT_EQUALS(funa->getParameter("a1"), 1.1);
  }

  void testCreateWithTies1() {
    std::string fnString = "name=FunctionFactoryConstraintTest_FunctA,a0=0.1,"
                           "a1=1.1,ties=(a0=a1=4)";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->getParameter("a0"), 4);
    TS_ASSERT_EQUALS(funa->getParameter("a1"), 4);
  }

  void testCreateWithTies2() {
    std::string fnString = "name=FunctionFactoryConstraintTest_FunctA,a0=0.1,"
                           "a1=1.1,ties=(a0=2,a1=4)";
    IFunction_sptr funa = FunctionFactory::Instance().createInitialized(fnString);
    TS_ASSERT(funa);
    TS_ASSERT_EQUALS(funa->getParameter("a0"), 2);
    TS_ASSERT_EQUALS(funa->getParameter("a1"), 4);
  }

  void testCreateCompositeWithTies() {
    std::string fnString = "name=FunctionFactoryConstraintTest_FunctA,ties=(a0=a1=14);"
                           "name=FunctionFactoryConstraintTest_FunctB,b0=0.2,b1=1.2;ties=(f1.b0="
                           "f0.a0+f0.a1)";

    IFunction_sptr fun = FunctionFactory::Instance().createInitialized(fnString);
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

    TS_ASSERT(fun1->isFixed(0));
    TS_ASSERT(fun1->isFixed(1));
    TS_ASSERT(!fun1->isActive(2));
    TS_ASSERT(fun1->isActive(3));
  }
};
