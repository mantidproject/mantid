// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PARAMETERTIETEST_H_
#define PARAMETERTIETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IPeakFunction.h"
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
  std::string name() const override { return "ParameterTieTest_Gauss"; }
  void functionLocal(double *out, const double *xValues,
                     const size_t nData) const override {
    double c = getParameter("cen");
    double h = getParameter("hi");
    double w = getParameter("sig");
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i] - c;
      out[i] = h * exp(-0.5 * x * x * w);
    }
  }
  void functionDerivLocal(Jacobian *out, const double *xValues,
                          const size_t nData) override {
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

  double centre() const override { return getParameter(0); }

  double height() const override { return getParameter(1); }

  double fwhm() const override { return getParameter(2); }

  void setCentre(const double c) override { setParameter(0, c); }
  void setHeight(const double h) override { setParameter(1, h); }

  void setFwhm(const double w) override { setParameter(2, w); }
};

class ParameterTieTest_Linear : public ParamFunction, public IFunction1D {
public:
  ParameterTieTest_Linear() {
    declareParameter("a");
    declareParameter("b");
  }
  std::string name() const override { return "ParameterTieTest_Linear"; }
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override {
    double a = getParameter("a");
    double b = getParameter("b");
    for (size_t i = 0; i < nData; i++) {
      out[i] = a + b * xValues[i];
    }
  }
  void functionDeriv1D(Jacobian *out, const double *xValues,
                       const size_t nData) override {
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
  std::string name() const override { return "ParameterTieTest_Nothing"; }
  void function1D(double *, const double *, const size_t) const override {}
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
    TS_ASSERT_EQUALS(tie.getLocalFunction(), g1.get());
    TS_ASSERT_EQUALS(tie.getLocalIndex(), 2);

    TS_ASSERT_THROWS(mustThrow1(&mfun), const std::invalid_argument &);
    TS_ASSERT_THROWS(mustThrow2(&mfun), const std::invalid_argument &);
    TS_ASSERT_THROWS(mustThrow3(&mfun), const std::out_of_range &);

    TS_ASSERT_THROWS(tie.set("a+b"), const std::invalid_argument &);
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
    TS_ASSERT_EQUALS(tie.getLocalFunction(), bk1.get());
    TS_ASSERT_EQUALS(tie.getLocalIndex(), 1);

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
    TS_ASSERT_THROWS(tie2.asString(mf1.get()), const std::logic_error &);
    TS_ASSERT_EQUALS(tie2.asString(&mfun), "f1.f0.sig=123.4");
    TS_ASSERT_EQUALS(tie2.asString(mf2.get()), "f0.sig=123.4");
    TS_ASSERT_EQUALS(tie2.asString(g1.get()), "sig=123.4");

    ParameterTie tie3(g1.get(), "sig", "123.4");
    TS_ASSERT_THROWS(tie3.asString(mf1.get()), const std::logic_error &);
    TS_ASSERT_EQUALS(tie3.asString(&mfun), "f1.f0.sig=123.4");
    TS_ASSERT_EQUALS(tie3.asString(mf2.get()), "f0.sig=123.4");
    TS_ASSERT_EQUALS(tie3.asString(g1.get()), "sig=123.4");

    ParameterTie tie4(mf2.get(), "f0.sig", "123.4");
    TS_ASSERT_THROWS(tie4.asString(mf1.get()), const std::logic_error &);
    TS_ASSERT_EQUALS(tie4.asString(&mfun), "f1.f0.sig=123.4");
    TS_ASSERT_EQUALS(tie4.asString(mf2.get()), "f0.sig=123.4");
    TS_ASSERT_EQUALS(tie4.asString(g1.get()), "sig=123.4");

    ParameterTie tie5(nth.get(), "a", "cos(B1e2Ta_)-sin(alpha12)");
    TS_ASSERT_THROWS(tie5.asString(mf1.get()), const std::logic_error &);
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

    TS_ASSERT_EQUALS(tie.getLocalIndex(), 1);
    TS_ASSERT_DELTA(tie.eval(), 0.6, 0.00001);
    TS_ASSERT_THROWS(mustThrow4(&bk), const std::invalid_argument &);
    TS_ASSERT_THROWS(mustThrow5(&bk), const std::invalid_argument &);
    TS_ASSERT_THROWS(tie.set("q+p"), const std::invalid_argument &);

    TS_ASSERT_THROWS(tie.set(""), const std::runtime_error &);
  }

  void test_untie_fixed() {
    ParameterTieTest_Linear bk;
    bk.fix(0);
    TS_ASSERT(bk.isFixed(0));
    bk.removeTie("a");
    TS_ASSERT(!bk.isFixed(0));
    bk.fix(0);
    bk.fix(1);
    bk.clearTies();
    TS_ASSERT(!bk.isFixed(0));
    TS_ASSERT(!bk.isFixed(1));
  }

  void test_untie_fixed_composite() {
    CompositeFunction_sptr mf = CompositeFunction_sptr(new CompositeFunction);
    IFunction_sptr bk1 = IFunction_sptr(new ParameterTieTest_Linear());
    IFunction_sptr bk2 = IFunction_sptr(new ParameterTieTest_Linear());
    mf->addFunction(bk1);
    mf->addFunction(bk2);
    mf->fix(0);
    mf->fix(3);
    TS_ASSERT(mf->isFixed(0));
    TS_ASSERT(mf->isFixed(3));
    mf->removeTie("f0.a");
    mf->removeTie("f1.b");
    TS_ASSERT(!mf->isFixed(0));
    TS_ASSERT(!mf->isFixed(3));
    mf->fix(0);
    mf->fix(3);
    mf->clearTies();
    TS_ASSERT(!mf->isFixed(0));
    TS_ASSERT(!mf->isFixed(3));
  }

  void test_circular_dependency() {
    auto mf = makeFunction();
    mf->tie("f0.a", "f3.f1.hi");
    mf->tie("f0.b", "f2.sig + f0.a");
    mf->tie("f2.sig", "f3.f1.hi");
    mf->tie("f3.f1.hi", "f0.b");

    TS_ASSERT_THROWS_EQUALS(
        mf->sortTies(), std::runtime_error & e, std::string(e.what()),
        "Circular dependency in "
        "ties:\nf3.f1.hi=f0.b\nf0.a=f3.f1.hi\nf0.b=f2.sig + f0.a");
  }

  void test_circular_dependency_a_a() {
    ParameterTieTest_Linear fun;
    fun.tie("a", "2*a");
    TS_ASSERT_THROWS_EQUALS(fun.sortTies(), std::runtime_error & e,
                            std::string(e.what()),
                            "Parameter is tied to itself: a=2*a");
  }

  void test_circular_dependency_a_b_a() {
    ParameterTieTest_Linear fun;
    fun.tie("a", "2*b");
    fun.tie("b", "a/2");
    TS_ASSERT_THROWS_EQUALS(fun.sortTies(), std::runtime_error & e,
                            std::string(e.what()),
                            "Circular dependency in ties:\nb=a/2\na=2*b");
  }

  void test_circular_dependency_a_b_c_a() {
    ParameterTieTest_Gauss fun;
    fun.tie("cen", "2*hi");
    fun.tie("hi", "sig/2");
    fun.tie("sig", "cen + 1");
    TS_ASSERT_THROWS_EQUALS(
        fun.sortTies(), std::runtime_error & e, std::string(e.what()),
        "Circular dependency in ties:\nsig=cen + 1\nhi=sig/2\ncen=2*hi");
  }

  void test_ties_order() {
    auto mf = makeFunction();
    mf->tie("f0.a", "f3.f1.hi");
    mf->tie("f0.b", "f2.sig + f0.a");
    mf->tie("f1.hi", "f1.cen*2");
    mf->tie("f2.sig", "f3.f1.hi");
    mf->tie("f3.f1.hi", "f1.sig");

    mf->applyTies();
    // Unordered ties applied wrongly
    TS_ASSERT(fabs(mf->getParameter("f0.a") - mf->getParameter("f3.f1.hi")) >
              1);
    TS_ASSERT(fabs(mf->getParameter("f0.b") - (mf->getParameter("f2.sig") +
                                               mf->getParameter("f0.a"))) > 1);
    TS_ASSERT(fabs(mf->getParameter("f1.hi") -
                   mf->getParameter("f1.cen") * 2.0) < 1e-5);
    TS_ASSERT(fabs(mf->getParameter("f2.sig") - mf->getParameter("f3.f1.hi")) >
              1);
    TS_ASSERT(fabs(mf->getParameter("f3.f1.hi") - mf->getParameter("f2.sig")) >
              1);
    TS_ASSERT_THROWS_NOTHING(mf->sortTies());
    mf->applyTies();
    // After ordering apply correctly
    TS_ASSERT_DELTA(mf->getParameter("f0.a"), mf->getParameter("f3.f1.hi"),
                    1e-5);
    TS_ASSERT_DELTA(mf->getParameter("f0.b"),
                    mf->getParameter("f2.sig") + mf->getParameter("f0.a"),
                    1e-5);
    TS_ASSERT_DELTA(mf->getParameter("f1.hi"), mf->getParameter("f1.cen") * 2.0,
                    1e-5);
    TS_ASSERT_DELTA(mf->getParameter("f2.sig"), mf->getParameter("f3.f1.hi"),
                    1e-5);
    TS_ASSERT_DELTA(mf->getParameter("f3.f1.hi"), mf->getParameter("f2.sig"),
                    1e-5);
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

  IFunction_sptr makeFunction() {
    CompositeFunction_sptr mf = CompositeFunction_sptr(new CompositeFunction);
    IFunction_sptr bk = IFunction_sptr(new ParameterTieTest_Linear());
    IFunction_sptr g1 = IFunction_sptr(new ParameterTieTest_Gauss());
    IFunction_sptr g2 = IFunction_sptr(new ParameterTieTest_Gauss());
    CompositeFunction_sptr cf = CompositeFunction_sptr(new CompositeFunction);
    IFunction_sptr g3 = IFunction_sptr(new ParameterTieTest_Gauss());
    IFunction_sptr g4 = IFunction_sptr(new ParameterTieTest_Gauss());
    cf->addFunction(g3);
    cf->addFunction(g4);

    mf->addFunction(bk);
    mf->addFunction(g1);
    mf->addFunction(g2);
    mf->addFunction(cf);
    for (size_t i = 0; i < mf->nParams(); ++i) {
      mf->setParameter(i, double(i + 1));
    }
    return mf;
  }
};

#endif /*PARAMETERTIETEST_H_*/
