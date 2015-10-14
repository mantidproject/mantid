#ifndef PARAMETERTIETEST_H_
#define PARAMETERTIETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParameterTie.h"

using namespace Mantid;
using namespace Mantid::API;

class ParameterTieTest_Gauss : public IPeakFunction {
public:
  ParameterTieTest_Gauss() {
    declareParameter("cen");
    declareParameter("hi", 1.);
    declareParameter("sig", 1.);
  }
  std::string name() const { return "ParameterTieTest_Gauss"; }
  void functionLocal(double *out, const double *xValues,
                     const size_t nData) const {
    double c = getParameter("cen");
    double h = getParameter("hi");
    double w = getParameter("sig");
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i] - c;
      out[i] = h * exp(-0.5 * x * x * w);
    }
  }
  void functionDerivLocal(Jacobian *out, const double *xValues,
                          const size_t nData) {
    // throw Mantid::Kernel::Exception::NotImplementedError("");
    double c = getParameter("cen");
    double h = getParameter("hi");
    double w = getParameter("sig");
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i] - c;
      double e = h * exp(-0.5 * x * x * w);
      out->set(static_cast<int>(i), 0, x * h * e * w);
      out->set(static_cast<int>(i), 1, e);
      out->set(static_cast<int>(i), 2, -0.5 * x * x * h * e);
    }
  }

  double centre() const { return getParameter(0); }

  double height() const { return getParameter(1); }

  double fwhm() const { return getParameter(2); }

  void setCentre(const double c) { setParameter(0, c); }
  void setHeight(const double h) { setParameter(1, h); }

  void setFwhm(const double w) { setParameter(2, w); }
};

class ParameterTieTest_Linear : public ParamFunction, public IFunction1D {
public:
  ParameterTieTest_Linear() {
    declareParameter("a");
    declareParameter("b");
  }
  std::string name() const { return "ParameterTieTest_Linear"; }
  void function1D(double *out, const double *xValues,
                  const size_t nData) const {
    double a = getParameter("a");
    double b = getParameter("b");
    for (size_t i = 0; i < nData; i++) {
      out[i] = a + b * xValues[i];
    }
  }
  void functionDeriv1D(Jacobian *out, const double *xValues,
                       const size_t nData) {
    // throw Mantid::Kernel::Exception::NotImplementedError("");
    for (size_t i = 0; i < nData; i++) {
      out->set(static_cast<int>(i), 0, 1.);
      out->set(static_cast<int>(i), 1, xValues[i]);
    }
  }
};

class ParameterTieTest_Nothing : public ParamFunction, public IFunction1D {
public:
  ParameterTieTest_Nothing() {
    declareParameter("a");
    declareParameter("alpha12");
    declareParameter("B1e2Ta_");
  }
  std::string name() const { return "ParameterTieTest_Nothing"; }
  void function1D(double *, const double *, const size_t) const {}
};

class ParameterTieTest : public CxxTest::TestSuite {
public:
  void testComposite() {
    CompositeFunction mfun;
    IFunction_sptr g1 = IFunction_sptr(new ParameterTieTest_Gauss());
    IFunction_sptr g2 = IFunction_sptr(new ParameterTieTest_Gauss());
    IFunction_sptr bk = IFunction_sptr(new ParameterTieTest_Linear());

    mfun.addFunction(bk);
    mfun.addFunction(g1);
    mfun.addFunction(g2);

    g1->setParameter("cen", 3.1);
    g1->setParameter("hi", 1.1);
    g1->setParameter("sig", 1.);

    g2->setParameter("cen", 7.1);
    g2->setParameter("hi", 1.1);
    g2->setParameter("sig", 2.);

    bk->setParameter("a", 0.8);

    ParameterTie tie(&mfun, "f1.sig", "f2.sig^2+f0.a+1");
    TS_ASSERT_EQUALS(tie.asString(&mfun), "f1.sig=f2.sig^2+f0.a+1");

    TS_ASSERT_DELTA(tie.eval(), 5.8, 0.00001);
    TS_ASSERT_EQUALS(tie.getFunction(), g1.get());
    TS_ASSERT_EQUALS(tie.getIndex(), 2);

    TS_ASSERT_THROWS(mustThrow1(&mfun), std::invalid_argument);
    TS_ASSERT_THROWS(mustThrow2(&mfun), std::invalid_argument);
    TS_ASSERT_THROWS(mustThrow3(&mfun), std::out_of_range);

    TS_ASSERT_THROWS(tie.set("a+b"), std::invalid_argument);
  }

  void testComposite1() {
    CompositeFunction mfun;
    IFunction_sptr g1 = IFunction_sptr(new ParameterTieTest_Gauss());
    IFunction_sptr g2 = IFunction_sptr(new ParameterTieTest_Gauss());
    IFunction_sptr bk1 = IFunction_sptr(new ParameterTieTest_Linear());
    IFunction_sptr bk2 = IFunction_sptr(new ParameterTieTest_Linear());

    mfun.addFunction(bk1);
    mfun.addFunction(bk2);
    mfun.addFunction(g1);
    mfun.addFunction(g2);

    ParameterTie tie(&mfun, "f0.b", "f3.sig^2+f1.a+1");
    TS_ASSERT_EQUALS(tie.asString(&mfun), "f0.b=f3.sig^2+f1.a+1");

    TS_ASSERT_DELTA(tie.eval(), 2, 0.00001);
    TS_ASSERT_EQUALS(tie.getFunction(), bk1.get());
    TS_ASSERT_EQUALS(tie.getIndex(), 1);

    mfun.removeFunction(2);
    TS_ASSERT_EQUALS(tie.asString(&mfun), "f0.b=f2.sig^2+f1.a+1");
  }

  void testComposite2() {
    CompositeFunction mfun;
    CompositeFunction_sptr mf1 = CompositeFunction_sptr(new CompositeFunction);
    CompositeFunction_sptr mf2 = CompositeFunction_sptr(new CompositeFunction);
    IFunction_sptr g1 = IFunction_sptr(new ParameterTieTest_Gauss());
    IFunction_sptr g2 = IFunction_sptr(new ParameterTieTest_Gauss());
    IFunction_sptr bk1 = IFunction_sptr(new ParameterTieTest_Linear());
    IFunction_sptr bk2 = IFunction_sptr(new ParameterTieTest_Linear());
    IFunction_sptr nth = IFunction_sptr(new ParameterTieTest_Nothing);

    mf1->addFunction(bk1);
    mf1->addFunction(bk2);
    mf2->addFunction(g1);
    mf2->addFunction(g2);
    mf2->addFunction(nth);
    mfun.addFunction(mf1);
    mfun.addFunction(mf2);

    ParameterTie tie(mf1.get(), "f0.b", "f1.a^2+f1.b+1");
    TS_ASSERT_EQUALS(tie.asString(mf1.get()), "f0.b=f1.a^2+f1.b+1");
    TS_ASSERT_EQUALS(tie.asString(&mfun), "f0.f0.b=f0.f1.a^2+f0.f1.b+1");

    ParameterTie tie1(&mfun, "f1.f0.sig", "sin(f1.f0.sig)+f1.f1.cen/2");
    TS_ASSERT_EQUALS(tie1.asString(&mfun),
                     "f1.f0.sig=sin(f1.f0.sig)+f1.f1.cen/2");
    TS_ASSERT_EQUALS(tie1.asString(mf2.get()), "f0.sig=sin(f0.sig)+f1.cen/2");

    ParameterTie tie2(&mfun, "f1.f0.sig", "123.4");
    TS_ASSERT_EQUALS(tie2.asString(mf1.get()), "");
    TS_ASSERT_EQUALS(tie2.asString(&mfun), "f1.f0.sig=123.4");
    TS_ASSERT_EQUALS(tie2.asString(mf2.get()), "f0.sig=123.4");
    TS_ASSERT_EQUALS(tie2.asString(g1.get()), "sig=123.4");

    ParameterTie tie3(g1.get(), "sig", "123.4");
    TS_ASSERT_EQUALS(tie3.asString(mf1.get()), "");
    TS_ASSERT_EQUALS(tie3.asString(&mfun), "f1.f0.sig=123.4");
    TS_ASSERT_EQUALS(tie3.asString(mf2.get()), "f0.sig=123.4");
    TS_ASSERT_EQUALS(tie3.asString(g1.get()), "sig=123.4");

    ParameterTie tie4(mf2.get(), "f0.sig", "123.4");
    TS_ASSERT_EQUALS(tie4.asString(mf1.get()), "");
    TS_ASSERT_EQUALS(tie4.asString(&mfun), "f1.f0.sig=123.4");
    TS_ASSERT_EQUALS(tie4.asString(mf2.get()), "f0.sig=123.4");
    TS_ASSERT_EQUALS(tie4.asString(g1.get()), "sig=123.4");

    ParameterTie tie5(nth.get(), "a", "cos(B1e2Ta_)-sin(alpha12)");
    TS_ASSERT_EQUALS(tie5.asString(mf1.get()), "");
    TS_ASSERT_EQUALS(tie5.asString(&mfun),
                     "f1.f2.a=cos(f1.f2.B1e2Ta_)-sin(f1.f2.alpha12)");
    TS_ASSERT_EQUALS(tie5.asString(mf2.get()),
                     "f2.a=cos(f2.B1e2Ta_)-sin(f2.alpha12)");
    TS_ASSERT_EQUALS(tie5.asString(nth.get()), "a=cos(B1e2Ta_)-sin(alpha12)");
  }

  void testSimple() {
    ParameterTieTest_Linear bk;

    bk.setParameter("a", 0.8);
    bk.setParameter("b", 0.);

    ParameterTie tie(&bk, "b", "2*a-1");

    TS_ASSERT_EQUALS(tie.getIndex(), 1);
    TS_ASSERT_DELTA(tie.eval(), 0.6, 0.00001);
    TS_ASSERT_THROWS(mustThrow4(&bk), std::invalid_argument);
    TS_ASSERT_THROWS(mustThrow5(&bk), std::invalid_argument);
    TS_ASSERT_THROWS(tie.set("q+p"), std::invalid_argument);

    TS_ASSERT_THROWS(tie.set(""), std::runtime_error);
  }

private:
  void mustThrow1(CompositeFunction *fun) { ParameterTie tie(fun, "sig", "0"); }
  void mustThrow2(CompositeFunction *fun) {
    ParameterTie tie(fun, "g1.sig", "0");
  }
  void mustThrow3(CompositeFunction *fun) {
    ParameterTie tie(fun, "f10.sig", "0");
  }
  void mustThrow4(IFunction *fun) { ParameterTie tie(fun, "f1.a", "0"); }
  void mustThrow5(IFunction *fun) { ParameterTie tie(fun, "cen", "0"); }
};

#endif /*PARAMETERTIETEST_H_*/
