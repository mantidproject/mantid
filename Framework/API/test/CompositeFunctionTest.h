#ifndef COMPOSITEFUNCTIONTEST_H_
#define COMPOSITEFUNCTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid;
using namespace Mantid::API;

class CompositeFunctionTest_MocSpectrum : public SpectrumTester {
public:
  CompositeFunctionTest_MocSpectrum(size_t nx, size_t ny)
      : SpectrumTester(HistogramData::getHistogramXMode(nx, ny),
                       HistogramData::Histogram::YMode::Counts) {
    dataX().resize(nx);
    dataY().resize(ny);
    dataE().resize(ny);
  }
};

class CompositeFunctionTest_MocMatrixWorkspace : public MatrixWorkspace {
public:
  CompositeFunctionTest_MocMatrixWorkspace(size_t nspec, size_t nx, size_t ny)
      : m_blocksize(ny) {
    for (size_t i = 0; i < nspec; ++i) {
      CompositeFunctionTest_MocSpectrum sp(nx, ny);
      m_spectra.push_back(sp);
    }
  }

  ~CompositeFunctionTest_MocMatrixWorkspace() override {}

  // Section required for iteration
  /// Returns the number of single indexable items in the workspace
  std::size_t size() const override { return m_spectra.size() * m_blocksize; }
  /// Returns the size of each block of data returned by the dataY accessors
  std::size_t blocksize() const override { return m_blocksize; }
  /// Returns the number of histograms in the workspace
  std::size_t getNumberHistograms() const override { return m_spectra.size(); }

  /// Return the underlying ISpectrum ptr at the given workspace index.
  ISpectrum &getSpectrum(const size_t index) override {
    return m_spectra[index];
  }

  /// Return the underlying ISpectrum ptr (const version) at the given workspace
  /// index.
  const ISpectrum &getSpectrum(const size_t index) const override {
    return m_spectra[index];
  }
  const std::string id(void) const override { return ""; }
  void init(const size_t &, const size_t &, const size_t &) override {}
  void init(const HistogramData::Histogram &) override {}
  void generateHistogram(const std::size_t, const MantidVec &, MantidVec &,
                         MantidVec &, bool) const override {}

  void clearFileBacked(bool) override{};
  ITableWorkspace_sptr makeBoxTable(size_t /* start*/,
                                    size_t /*num*/) override {
    return ITableWorkspace_sptr();
  }

private:
  CompositeFunctionTest_MocMatrixWorkspace *doClone() const override {
    throw std::runtime_error("Cloning of "
                             "CompositeFunctionTest_MocMatrixWorkspace is not "
                             "implemented.");
  }
  CompositeFunctionTest_MocMatrixWorkspace *doCloneEmpty() const override {
    throw std::runtime_error("Cloning of "
                             "CompositeFunctionTest_MocMatrixWorkspace is not "
                             "implemented.");
  }
  std::vector<CompositeFunctionTest_MocSpectrum> m_spectra;
  size_t m_blocksize;
};

class Gauss : public IPeakFunction {
public:
  Gauss() {
    declareParameter("c");
    declareParameter("h", 1.);
    declareParameter("s", 1.);
  }

  std::string name() const override { return "Gauss"; }

  void functionLocal(double *out, const double *xValues,
                     const size_t nData) const override {
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i] - c;
      out[i] = h * exp(-0.5 * x * x * w);
    }
  }
  void functionDerivLocal(Jacobian *out, const double *xValues,
                          const size_t nData) override {
    // throw Mantid::Kernel::Exception::NotImplementedError("");
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
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

class Linear : public ParamFunction, public IFunction1D {
public:
  Linear() {
    declareParameter("a");
    declareParameter("b");
  }

  std::string name() const override { return "Linear"; }

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

class Cubic : public ParamFunction, public IFunction1D {
public:
  Cubic() {
    declareParameter("c0");
    declareParameter("c1");
    declareParameter("c2");
    declareParameter("c3");
  }

  std::string name() const override { return "Cubic"; }

  void function1D(double *out, const double *xValues,
                  const size_t nData) const override {
    double c0 = getParameter("c0");
    double c1 = getParameter("c1");
    double c2 = getParameter("c2");
    double c3 = getParameter("c3");
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i];
      out[i] = c0 + x * (c1 + x * (c2 + x * c3));
    }
  }
  void functionDeriv1D(Jacobian *out, const double *xValues,
                       const size_t nData) override {
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i];
      out->set(static_cast<int>(i), 0, 1.);
      out->set(static_cast<int>(i), 1, x);
      out->set(static_cast<int>(i), 2, x * x);
      out->set(static_cast<int>(i), 3, x * x * x);
    }
  }
};

class CompositeFunctionTest : public CxxTest::TestSuite {
public:
  static CompositeFunctionTest *createSuite() {
    return new CompositeFunctionTest();
  }
  static void destroySuite(CompositeFunctionTest *suite) { delete suite; }

  CompositeFunctionTest() {
    FunctionFactory::Instance().subscribe<Linear>("Linear");
  }

  ~CompositeFunctionTest() override {
    FunctionFactory::Instance().unsubscribe("Linear");
  }

  void testAdd() {
    IFunction_sptr g1 = IFunction_sptr(new Gauss());
    IFunction_sptr g2 = IFunction_sptr(new Gauss());
    IFunction_sptr bk = IFunction_sptr(new Linear());
    IFunction_sptr cub = IFunction_sptr(new Cubic());

    CompositeFunction *mfun = new CompositeFunction;

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a", 0.8);

    g1->setParameter("c", 1.1);
    g1->setParameter("h", 1.2);
    g1->setParameter("s", 1.3);

    cub->setParameter("c0", 2.1);
    cub->setParameter("c1", 2.2);
    cub->setParameter("c2", 2.3);
    cub->setParameter("c3", 2.4);

    g2->setParameter("c", 3.1);
    g2->setParameter("h", 3.2);
    g2->setParameter("s", 3.3);

    TS_ASSERT_EQUALS(mfun->nParams(), 12);

    TS_ASSERT_EQUALS(mfun->getParameter(0), 0.8);
    TS_ASSERT_EQUALS(mfun->getParameter(1), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter(2), 1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(3), 1.2);
    TS_ASSERT_EQUALS(mfun->getParameter(4), 1.3);
    TS_ASSERT_EQUALS(mfun->getParameter(5), 2.1);
    TS_ASSERT_EQUALS(mfun->getParameter(6), 2.2);
    TS_ASSERT_EQUALS(mfun->getParameter(7), 2.3);
    TS_ASSERT_EQUALS(mfun->getParameter(8), 2.4);
    TS_ASSERT_EQUALS(mfun->getParameter(9), 3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(10), 3.2);
    TS_ASSERT_EQUALS(mfun->getParameter(11), 3.3);

    TS_ASSERT_EQUALS(mfun->parameterName(0), "f0.a");
    TS_ASSERT_EQUALS(mfun->parameterName(1), "f0.b");
    TS_ASSERT_EQUALS(mfun->parameterName(2), "f1.c");
    TS_ASSERT_EQUALS(mfun->parameterName(3), "f1.h");
    TS_ASSERT_EQUALS(mfun->parameterName(4), "f1.s");
    TS_ASSERT_EQUALS(mfun->parameterName(5), "f2.c0");
    TS_ASSERT_EQUALS(mfun->parameterName(6), "f2.c1");
    TS_ASSERT_EQUALS(mfun->parameterName(7), "f2.c2");
    TS_ASSERT_EQUALS(mfun->parameterName(8), "f2.c3");
    TS_ASSERT_EQUALS(mfun->parameterName(9), "f3.c");
    TS_ASSERT_EQUALS(mfun->parameterName(10), "f3.h");
    TS_ASSERT_EQUALS(mfun->parameterName(11), "f3.s");

    TS_ASSERT_EQUALS(mfun->getParameter("f0.a"), 0.8);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.b"), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.c"), 1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.h"), 1.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.s"), 1.3);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c0"), 2.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c1"), 2.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c2"), 2.3);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c3"), 2.4);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.c"), 3.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.h"), 3.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.s"), 3.3);

    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.a"), 0);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.b"), 1);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.c"), 2);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.h"), 3);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.s"), 4);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c0"), 5);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c1"), 6);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c2"), 7);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c3"), 8);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.c"), 9);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.h"), 10);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.s"), 11);

    std::string str = "name=Linear,a=0.8,b=0;";
    str += "name=Gauss,c=1.1,h=1.2,s=1.3;";
    str += "name=Cubic,c0=2.1,c1=2.2,c2=2.3,c3=2.4;";
    str += "name=Gauss,c=3.1,h=3.2,s=3.3";

    TS_ASSERT_EQUALS(mfun->asString(), str);

    delete mfun;
  }

  void testTies() {
    CompositeFunction *mfun = new CompositeFunction;
    IFunction_sptr g1 = IFunction_sptr(new Gauss());
    IFunction_sptr g2 = IFunction_sptr(new Gauss());
    IFunction_sptr bk = IFunction_sptr(new Linear());
    IFunction_sptr cub = IFunction_sptr(new Cubic());

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a", 0.8);

    g1->setParameter("c", 1.1);
    g1->setParameter("h", 1.2);
    g1->setParameter("s", 1.3);

    cub->setParameter("c0", 2.1);
    cub->setParameter("c1", 2.2);
    cub->setParameter("c2", 2.3);
    cub->setParameter("c3", 2.4);

    g2->setParameter("c", 3.1);
    g2->setParameter("h", 3.2);
    g2->setParameter("s", 3.3);

    mfun->tie("f0.a", "0");
    mfun->tie("f0.b", "0");
    mfun->tie("f1.s", "0");
    mfun->tie("f2.c1", "0");
    mfun->tie("f2.c2", "0");
    mfun->tie("f3.h", "0");

    TS_ASSERT_EQUALS(mfun->activeParameter(2), 1.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(3), 1.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(5), 2.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(8), 2.4);
    TS_ASSERT_EQUALS(mfun->activeParameter(9), 3.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(11), 3.3);

    TS_ASSERT_EQUALS(mfun->nameOfActive(2), "f1.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(3), "f1.h");
    TS_ASSERT_EQUALS(mfun->nameOfActive(5), "f2.c0");
    TS_ASSERT_EQUALS(mfun->nameOfActive(8), "f2.c3");
    TS_ASSERT_EQUALS(mfun->nameOfActive(9), "f3.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(11), "f3.s");

    TS_ASSERT(mfun->isFixed(0));
    TS_ASSERT(mfun->isFixed(1));
    TS_ASSERT(!mfun->isFixed(2));
    TS_ASSERT(!mfun->isFixed(3));
    TS_ASSERT(mfun->isFixed(4));
    TS_ASSERT(!mfun->isFixed(5));
    TS_ASSERT(mfun->isFixed(6));
    TS_ASSERT(mfun->isFixed(7));
    TS_ASSERT(!mfun->isFixed(8));
    TS_ASSERT(!mfun->isFixed(9));
    TS_ASSERT(mfun->isFixed(10));
    TS_ASSERT(!mfun->isFixed(11));

    TS_ASSERT_EQUALS(mfun->nParams(), 12);

    TS_ASSERT_EQUALS(mfun->getParameter(0), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter(1), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter(2), 1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(3), 1.2);
    TS_ASSERT_EQUALS(mfun->getParameter(4), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter(5), 2.1);
    TS_ASSERT_EQUALS(mfun->getParameter(6), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter(7), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter(8), 2.4);
    TS_ASSERT_EQUALS(mfun->getParameter(9), 3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(10), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter(11), 3.3);

    TS_ASSERT_EQUALS(mfun->parameterName(0), "f0.a");
    TS_ASSERT_EQUALS(mfun->parameterName(1), "f0.b");
    TS_ASSERT_EQUALS(mfun->parameterName(2), "f1.c");
    TS_ASSERT_EQUALS(mfun->parameterName(3), "f1.h");
    TS_ASSERT_EQUALS(mfun->parameterName(4), "f1.s");
    TS_ASSERT_EQUALS(mfun->parameterName(5), "f2.c0");
    TS_ASSERT_EQUALS(mfun->parameterName(6), "f2.c1");
    TS_ASSERT_EQUALS(mfun->parameterName(7), "f2.c2");
    TS_ASSERT_EQUALS(mfun->parameterName(8), "f2.c3");
    TS_ASSERT_EQUALS(mfun->parameterName(9), "f3.c");
    TS_ASSERT_EQUALS(mfun->parameterName(10), "f3.h");
    TS_ASSERT_EQUALS(mfun->parameterName(11), "f3.s");

    TS_ASSERT_EQUALS(mfun->getParameter("f0.a"), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.b"), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.c"), 1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.h"), 1.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.s"), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c0"), 2.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c1"), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c2"), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c3"), 2.4);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.c"), 3.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.h"), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.s"), 3.3);

    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.a"), 0);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.b"), 1);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.c"), 2);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.h"), 3);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.s"), 4);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c0"), 5);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c1"), 6);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c2"), 7);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c3"), 8);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.c"), 9);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.h"), 10);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.s"), 11);

    delete mfun;
  }

  void testSetActive() {
    CompositeFunction *mfun = new CompositeFunction;
    IFunction_sptr g1 = IFunction_sptr(new Gauss());
    IFunction_sptr g2 = IFunction_sptr(new Gauss());
    IFunction_sptr bk = IFunction_sptr(new Linear());
    IFunction_sptr cub = IFunction_sptr(new Cubic());

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a", 0.8); // 0

    g1->setParameter("c", 1.1); // 2
    g1->setParameter("h", 1.2); // 3
    g1->setParameter("s", 1.3); // 4

    cub->setParameter("c0", 2.1); // 5
    cub->setParameter("c1", 2.2); // 6
    cub->setParameter("c2", 2.3); // 7
    cub->setParameter("c3", 2.4); // 8

    g2->setParameter("c", 3.1); // 9
    g2->setParameter("h", 3.2); // 10
    g2->setParameter("s", 3.3); // 11

    mfun->tie("f0.a", "-1");
    mfun->tie("f0.b", "-2");
    mfun->tie("f1.s", "-3");
    mfun->tie("f2.c1", "-4");
    mfun->tie("f2.c2", "-5");
    mfun->tie("f3.h", "-6");

    mfun->setActiveParameter(2, 100);
    mfun->setActiveParameter(3, 101);
    mfun->setActiveParameter(5, 102);
    mfun->setActiveParameter(8, 103);
    mfun->setActiveParameter(9, 104);
    mfun->setActiveParameter(11, 105);

    TS_ASSERT_EQUALS(mfun->activeParameter(2), 100);
    TS_ASSERT_EQUALS(mfun->activeParameter(3), 101);
    TS_ASSERT_EQUALS(mfun->activeParameter(5), 102);
    TS_ASSERT_EQUALS(mfun->activeParameter(8), 103);
    TS_ASSERT_EQUALS(mfun->activeParameter(9), 104);
    TS_ASSERT_EQUALS(mfun->activeParameter(11), 105);

    TS_ASSERT_EQUALS(mfun->nParams(), 12);

    TS_ASSERT_EQUALS(mfun->getParameter(0), -1);
    TS_ASSERT_EQUALS(mfun->getParameter(1), -2);
    TS_ASSERT_EQUALS(mfun->getParameter(2), 100);
    TS_ASSERT_EQUALS(mfun->getParameter(3), 101);
    TS_ASSERT_EQUALS(mfun->getParameter(4), -3);
    TS_ASSERT_EQUALS(mfun->getParameter(5), 102);
    TS_ASSERT_EQUALS(mfun->getParameter(6), -4);
    TS_ASSERT_EQUALS(mfun->getParameter(7), -5);
    TS_ASSERT_EQUALS(mfun->getParameter(8), 103);
    TS_ASSERT_EQUALS(mfun->getParameter(9), 104);
    TS_ASSERT_EQUALS(mfun->getParameter(10), -6);
    TS_ASSERT_EQUALS(mfun->getParameter(11), 105);

    delete mfun;
  }

  void testFix() {
    CompositeFunction *mfun = new CompositeFunction;
    IFunction_sptr g1 = IFunction_sptr(new Gauss());
    IFunction_sptr g2 = IFunction_sptr(new Gauss());
    IFunction_sptr bk = IFunction_sptr(new Linear());
    IFunction_sptr cub = IFunction_sptr(new Cubic());

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a", 0.8);

    g1->setParameter("c", 1.1);
    g1->setParameter("h", 1.2);
    g1->setParameter("s", 1.3);

    cub->setParameter("c0", 2.1);
    cub->setParameter("c1", 2.2);
    cub->setParameter("c2", 2.3);
    cub->setParameter("c3", 2.4);

    g2->setParameter("c", 3.1);
    g2->setParameter("h", 3.2);
    g2->setParameter("s", 3.3);

    mfun->fix(0);
    mfun->fix(1);
    mfun->fix(4);
    // g1->fix(2);  // This doesn't work
    mfun->fix(6);
    mfun->fix(7);
    mfun->fix(10);
    // g2->fix(1);  // This doesn't work

    TS_ASSERT_THROWS(mfun->setActiveParameter(0, 0), std::runtime_error);
    TS_ASSERT_THROWS(mfun->setActiveParameter(1, 0), std::runtime_error);
    TS_ASSERT_THROWS(mfun->setActiveParameter(4, 0), std::runtime_error);
    TS_ASSERT_THROWS(mfun->setActiveParameter(6, 0), std::runtime_error);
    TS_ASSERT_THROWS(mfun->setActiveParameter(7, 0), std::runtime_error);
    TS_ASSERT_THROWS(mfun->setActiveParameter(10, 0), std::runtime_error);

    mfun->setActiveParameter(2, 100);
    mfun->setActiveParameter(3, 101);
    mfun->setActiveParameter(5, 102);
    mfun->setActiveParameter(8, 103);
    mfun->setActiveParameter(9, 104);
    mfun->setActiveParameter(11, 105);

    TS_ASSERT_EQUALS(mfun->activeParameter(2), 100);
    TS_ASSERT_EQUALS(mfun->activeParameter(3), 101);
    TS_ASSERT_EQUALS(mfun->activeParameter(5), 102);
    TS_ASSERT_EQUALS(mfun->activeParameter(8), 103);
    TS_ASSERT_EQUALS(mfun->activeParameter(9), 104);
    TS_ASSERT_EQUALS(mfun->activeParameter(11), 105);

    TS_ASSERT_EQUALS(mfun->nParams(), 12);

    TS_ASSERT_EQUALS(mfun->getParameter(0), 0.8);
    TS_ASSERT_EQUALS(mfun->getParameter(1), 0.0);
    TS_ASSERT_EQUALS(mfun->getParameter(2), 100);
    TS_ASSERT_EQUALS(mfun->getParameter(3), 101);
    TS_ASSERT_EQUALS(mfun->getParameter(4), 1.3);
    TS_ASSERT_EQUALS(mfun->getParameter(5), 102);
    TS_ASSERT_EQUALS(mfun->getParameter(6), 2.2);
    TS_ASSERT_EQUALS(mfun->getParameter(7), 2.3);
    TS_ASSERT_EQUALS(mfun->getParameter(8), 103);
    TS_ASSERT_EQUALS(mfun->getParameter(9), 104);
    TS_ASSERT_EQUALS(mfun->getParameter(10), 3.2);
    TS_ASSERT_EQUALS(mfun->getParameter(11), 105);

    delete mfun;
  }

  void testApplyTies() {
    CompositeFunction *mfun = new CompositeFunction;
    IFunction_sptr g1 = IFunction_sptr(new Gauss());
    IFunction_sptr g2 = IFunction_sptr(new Gauss());
    IFunction_sptr bk = IFunction_sptr(new Linear());
    IFunction_sptr cub = IFunction_sptr(new Cubic());

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a", 0.8);

    g1->setParameter("c", 1.1);
    g1->setParameter("h", 1.2);
    g1->setParameter("s", 1.3);

    cub->setParameter("c0", 2.1);
    cub->setParameter("c1", 2.2);
    cub->setParameter("c2", 2.3);
    cub->setParameter("c3", 2.4);

    g2->setParameter("c", 3.1);
    g2->setParameter("h", 3.2);
    g2->setParameter("s", 3.3);

    mfun->tie("f0.b", "77");
    mfun->tie("f0.a", "2*f0.b");
    mfun->tie("f1.s", "f3.s/2");
    mfun->tie("f2.c1", "f2.c3^2");
    mfun->tie("f2.c2", "sqrt(f2.c3)");
    mfun->tie("f3.h", "f2.c0+f0.b");

    mfun->applyTies();

    TS_ASSERT_EQUALS(mfun->activeParameter(2), 1.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(3), 1.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(5), 2.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(8), 2.4);
    TS_ASSERT_EQUALS(mfun->activeParameter(9), 3.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(11), 3.3);

    TS_ASSERT_EQUALS(mfun->nParams(), 12);

    TS_ASSERT_EQUALS(mfun->getParameter("f0.a"), 154);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.b"), 77);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.c"), 1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.h"), 1.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.s"), 1.65);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c0"), 2.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c1"), 2.4 * 2.4);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c2"), sqrt(2.4));
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c3"), 2.4);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.c"), 3.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.h"), 79.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.s"), 3.3);

    delete mfun;
  }

  void testApplyTiesInWrongOrder() {
    CompositeFunction *mfun = new CompositeFunction;
    IFunction_sptr g1 = IFunction_sptr(new Gauss());
    IFunction_sptr g2 = IFunction_sptr(new Gauss());
    IFunction_sptr bk = IFunction_sptr(new Linear());
    IFunction_sptr cub = IFunction_sptr(new Cubic());

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a", 0.8);

    g1->setParameter("c", 1.1);
    g1->setParameter("h", 1.2);
    g1->setParameter("s", 1.3);

    cub->setParameter("c0", 2.1);
    cub->setParameter("c1", 2.2);
    cub->setParameter("c2", 2.3);
    cub->setParameter("c3", 2.4);

    g2->setParameter("c", 3.1);
    g2->setParameter("h", 3.2);
    g2->setParameter("s", 3.3);

    mfun->tie("f0.a", "2*f0.b");
    mfun->tie("f0.b", "77");
    mfun->tie("f1.s", "f3.s/2");
    mfun->tie("f2.c1", "f2.c3^2");
    mfun->tie("f2.c2", "sqrt(f2.c3)");
    mfun->tie("f3.h", "f2.c0+f0.b");

    mfun->applyTies();

    TS_ASSERT_EQUALS(mfun->activeParameter(2), 1.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(3), 1.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(5), 2.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(8), 2.4);
    TS_ASSERT_EQUALS(mfun->activeParameter(9), 3.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(11), 3.3);

    TS_ASSERT_EQUALS(mfun->nParams(), 12);

    TS_ASSERT_EQUALS(mfun->getParameter(0), 154);
    TS_ASSERT_EQUALS(mfun->getParameter(1), 77);
    TS_ASSERT_EQUALS(mfun->getParameter(2), 1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(3), 1.2);
    TS_ASSERT_EQUALS(mfun->getParameter(4), 1.65);
    TS_ASSERT_EQUALS(mfun->getParameter(5), 2.1);
    TS_ASSERT_EQUALS(mfun->getParameter(6), 2.4 * 2.4);
    TS_ASSERT_EQUALS(mfun->getParameter(7), sqrt(2.4));
    TS_ASSERT_EQUALS(mfun->getParameter(8), 2.4);
    TS_ASSERT_EQUALS(mfun->getParameter(9), 3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(10), 79.1);
    TS_ASSERT_EQUALS(mfun->getParameter(11), 3.3);

    delete mfun;
  }

  void testRemoveFunction() {

    CompositeFunction *mfun = new CompositeFunction;
    IFunction_sptr g1 = IFunction_sptr(new Gauss());
    IFunction_sptr g2 = IFunction_sptr(new Gauss());
    IFunction_sptr bk = IFunction_sptr(new Linear());
    IFunction_sptr cub = IFunction_sptr(new Cubic());

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a", 0.8);

    g1->setParameter("c", 1.1);
    g1->setParameter("h", 1.2);
    g1->setParameter("s", 1.3);

    cub->setParameter("c0", 2.1);
    cub->setParameter("c1", 2.2);
    cub->setParameter("c2", 2.3);
    cub->setParameter("c3", 2.4);

    g2->setParameter("c", 3.1);
    g2->setParameter("h", 3.2);
    g2->setParameter("s", 3.3);

    mfun->tie("f0.a", "101");
    mfun->tie("f0.b", "102");
    mfun->tie("f1.s", "103");
    mfun->tie("f2.c1", "104");
    mfun->tie("f2.c2", "105");
    mfun->tie("f3.h", "106");

    mfun->removeFunction(2);

    mfun->applyTies();

    TS_ASSERT_EQUALS(mfun->nFunctions(), 3);

    TS_ASSERT_EQUALS(mfun->nParams(), 8);

    TS_ASSERT_EQUALS(mfun->getParameter(0), 101);
    TS_ASSERT_EQUALS(mfun->getParameter(1), 102);
    TS_ASSERT_EQUALS(mfun->getParameter(2), 1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(3), 1.2);
    TS_ASSERT_EQUALS(mfun->getParameter(4), 103);
    TS_ASSERT_EQUALS(mfun->getParameter(5), 3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(6), 106);
    TS_ASSERT_EQUALS(mfun->getParameter(7), 3.3);

    TS_ASSERT_EQUALS(mfun->parameterName(0), "f0.a");
    TS_ASSERT_EQUALS(mfun->parameterName(1), "f0.b");
    TS_ASSERT_EQUALS(mfun->parameterName(2), "f1.c");
    TS_ASSERT_EQUALS(mfun->parameterName(3), "f1.h");
    TS_ASSERT_EQUALS(mfun->parameterName(4), "f1.s");
    TS_ASSERT_EQUALS(mfun->parameterName(5), "f2.c");
    TS_ASSERT_EQUALS(mfun->parameterName(6), "f2.h");
    TS_ASSERT_EQUALS(mfun->parameterName(7), "f2.s");

    TS_ASSERT_EQUALS(mfun->getParameter("f0.a"), 101);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.b"), 102);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.c"), 1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.h"), 1.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.s"), 103);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c"), 3.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.h"), 106);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.s"), 3.3);

    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.a"), 0);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.b"), 1);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.c"), 2);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.h"), 3);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.s"), 4);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c"), 5);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.h"), 6);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.s"), 7);

    TS_ASSERT_EQUALS(mfun->nameOfActive(2), "f1.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(3), "f1.h");
    TS_ASSERT_EQUALS(mfun->nameOfActive(5), "f2.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(7), "f2.s");

    TS_ASSERT_EQUALS(mfun->activeParameter(2), 1.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(3), 1.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(5), 3.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(7), 3.3);

    TS_ASSERT(mfun->isFixed(0));
    TS_ASSERT(mfun->isFixed(1));
    TS_ASSERT(!mfun->isFixed(2));
    TS_ASSERT(!mfun->isFixed(3));
    TS_ASSERT(mfun->isFixed(4));
    TS_ASSERT(!mfun->isFixed(5));
    TS_ASSERT(mfun->isFixed(6));
    TS_ASSERT(!mfun->isFixed(7));

    delete mfun;
  }

  void test_removeFunction_removes_tie() {
    CompositeFunction mfun;
    IFunction_sptr g1 = IFunction_sptr(new Gauss());
    IFunction_sptr g2 = IFunction_sptr(new Gauss());

    mfun.addFunction(g1);
    mfun.addFunction(g2);

    mfun.tie("f0.h", "1-f1.h");
    mfun.removeFunction(0);
    TS_ASSERT_EQUALS(mfun.writeTies(), "");
  }

  // replacing function has fewer parameters
  void testReplaceFunction() {
    IFunction_sptr g1 = IFunction_sptr(new Gauss());
    IFunction_sptr g2 = IFunction_sptr(new Gauss());
    IFunction_sptr bk = IFunction_sptr(new Linear());
    IFunction_sptr cub = IFunction_sptr(new Cubic());
    IFunction_sptr bk1 = IFunction_sptr(new Linear());

    CompositeFunction *mfun = new CompositeFunction;

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a", 0.8);

    g1->setParameter("c", 1.1);
    g1->setParameter("h", 1.2);
    g1->setParameter("s", 1.3);

    cub->setParameter("c0", 2.1);
    cub->setParameter("c1", 2.2);
    cub->setParameter("c2", 2.3);
    cub->setParameter("c3", 2.4);

    g2->setParameter("c", 3.1);
    g2->setParameter("h", 3.2);
    g2->setParameter("s", 3.3);

    mfun->tie("f0.a", "101");
    mfun->tie("f0.b", "102");
    mfun->tie("f1.s", "103");
    mfun->tie("f2.c1", "104");
    mfun->tie("f2.c2", "105");
    mfun->tie("f3.h", "106");

    bk1->setParameter("a", 4.1);
    bk1->setParameter("b", 4.2);

    mfun->replaceFunction(2, bk1);

    mfun->applyTies();

    TS_ASSERT_EQUALS(mfun->nFunctions(), 4);

    TS_ASSERT_EQUALS(mfun->nParams(), 10);

    TS_ASSERT_EQUALS(mfun->getParameter(0), 101);
    TS_ASSERT_EQUALS(mfun->getParameter(1), 102);
    TS_ASSERT_EQUALS(mfun->getParameter(2), 1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(3), 1.2);
    TS_ASSERT_EQUALS(mfun->getParameter(4), 103);
    TS_ASSERT_EQUALS(mfun->getParameter(5), 4.1);
    TS_ASSERT_EQUALS(mfun->getParameter(6), 4.2);
    TS_ASSERT_EQUALS(mfun->getParameter(7), 3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(8), 106);
    TS_ASSERT_EQUALS(mfun->getParameter(9), 3.3);

    TS_ASSERT_EQUALS(mfun->parameterName(0), "f0.a");
    TS_ASSERT_EQUALS(mfun->parameterName(1), "f0.b");
    TS_ASSERT_EQUALS(mfun->parameterName(2), "f1.c");
    TS_ASSERT_EQUALS(mfun->parameterName(3), "f1.h");
    TS_ASSERT_EQUALS(mfun->parameterName(4), "f1.s");
    TS_ASSERT_EQUALS(mfun->parameterName(5), "f2.a");
    TS_ASSERT_EQUALS(mfun->parameterName(6), "f2.b");
    TS_ASSERT_EQUALS(mfun->parameterName(7), "f3.c");
    TS_ASSERT_EQUALS(mfun->parameterName(8), "f3.h");
    TS_ASSERT_EQUALS(mfun->parameterName(9), "f3.s");

    TS_ASSERT_EQUALS(mfun->getParameter("f0.a"), 101);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.b"), 102);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.c"), 1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.h"), 1.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.s"), 103);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.a"), 4.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.b"), 4.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.c"), 3.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.h"), 106);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.s"), 3.3);

    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.a"), 0);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.b"), 1);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.c"), 2);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.h"), 3);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.s"), 4);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.a"), 5);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.b"), 6);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.c"), 7);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.h"), 8);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.s"), 9);

    TS_ASSERT_EQUALS(mfun->nameOfActive(2), "f1.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(3), "f1.h");
    TS_ASSERT_EQUALS(mfun->nameOfActive(5), "f2.a");
    TS_ASSERT_EQUALS(mfun->nameOfActive(6), "f2.b");
    TS_ASSERT_EQUALS(mfun->nameOfActive(7), "f3.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(9), "f3.s");

    TS_ASSERT_EQUALS(mfun->activeParameter(2), 1.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(3), 1.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(5), 4.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(6), 4.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(7), 3.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(9), 3.3);

    TS_ASSERT(mfun->isFixed(0));
    TS_ASSERT(mfun->isFixed(1));
    TS_ASSERT(!mfun->isFixed(2));
    TS_ASSERT(!mfun->isFixed(3));
    TS_ASSERT(mfun->isFixed(4));
    TS_ASSERT(!mfun->isFixed(5));
    TS_ASSERT(!mfun->isFixed(6));
    TS_ASSERT(!mfun->isFixed(7));
    TS_ASSERT(mfun->isFixed(8));
    TS_ASSERT(!mfun->isFixed(9));

    delete mfun;
  }

  // replacing function has more parameters
  void testReplaceFunction1() {
    IFunction_sptr g1 = IFunction_sptr(new Gauss());
    IFunction_sptr g2 = IFunction_sptr(new Gauss());
    IFunction_sptr bk = IFunction_sptr(new Linear());
    IFunction_sptr cub = IFunction_sptr(new Cubic());
    IFunction_sptr cub1 = IFunction_sptr(new Cubic());

    CompositeFunction *mfun = new CompositeFunction;

    mfun->addFunction(bk);
    mfun->addFunction(g1);
    mfun->addFunction(cub);
    mfun->addFunction(g2);

    bk->setParameter("a", 0.8);

    g1->setParameter("c", 1.1);
    g1->setParameter("h", 1.2);
    g1->setParameter("s", 1.3);

    cub->setParameter("c0", 2.1);
    cub->setParameter("c1", 2.2);
    cub->setParameter("c2", 2.3);
    cub->setParameter("c3", 2.4);

    g2->setParameter("c", 3.1);
    g2->setParameter("h", 3.2);
    g2->setParameter("s", 3.3);

    mfun->tie("f0.a", "101");
    mfun->tie("f0.b", "102");
    mfun->tie("f1.s", "103");
    mfun->tie("f2.c1", "104");
    mfun->tie("f2.c2", "105");
    mfun->tie("f3.h", "106");

    cub1->setParameter("c0", 4.1);
    cub1->setParameter("c1", 4.2);
    cub1->setParameter("c2", 4.3);
    cub1->setParameter("c3", 4.4);

    mfun->replaceFunction(0, cub1);

    mfun->applyTies();

    TS_ASSERT_EQUALS(mfun->nFunctions(), 4);

    TS_ASSERT_EQUALS(mfun->nParams(), 14);

    TS_ASSERT_EQUALS(mfun->getParameter(0), 4.1);
    TS_ASSERT_EQUALS(mfun->getParameter(1), 4.2);
    TS_ASSERT_EQUALS(mfun->getParameter(2), 4.3);
    TS_ASSERT_EQUALS(mfun->getParameter(3), 4.4);
    TS_ASSERT_EQUALS(mfun->getParameter(4), 1.1);
    TS_ASSERT_EQUALS(mfun->getParameter(5), 1.2);
    TS_ASSERT_EQUALS(mfun->getParameter(6), 103);
    TS_ASSERT_EQUALS(mfun->getParameter(7), 2.1);
    TS_ASSERT_EQUALS(mfun->getParameter(8), 104);
    TS_ASSERT_EQUALS(mfun->getParameter(9), 105);
    TS_ASSERT_EQUALS(mfun->getParameter(10), 2.4);
    TS_ASSERT_EQUALS(mfun->getParameter(11), 3.1);
    TS_ASSERT_EQUALS(mfun->getParameter(12), 106);
    TS_ASSERT_EQUALS(mfun->getParameter(13), 3.3);

    TS_ASSERT_EQUALS(mfun->parameterName(0), "f0.c0");
    TS_ASSERT_EQUALS(mfun->parameterName(1), "f0.c1");
    TS_ASSERT_EQUALS(mfun->parameterName(2), "f0.c2");
    TS_ASSERT_EQUALS(mfun->parameterName(3), "f0.c3");
    TS_ASSERT_EQUALS(mfun->parameterName(4), "f1.c");
    TS_ASSERT_EQUALS(mfun->parameterName(5), "f1.h");
    TS_ASSERT_EQUALS(mfun->parameterName(6), "f1.s");
    TS_ASSERT_EQUALS(mfun->parameterName(7), "f2.c0");
    TS_ASSERT_EQUALS(mfun->parameterName(8), "f2.c1");
    TS_ASSERT_EQUALS(mfun->parameterName(9), "f2.c2");
    TS_ASSERT_EQUALS(mfun->parameterName(10), "f2.c3");
    TS_ASSERT_EQUALS(mfun->parameterName(11), "f3.c");
    TS_ASSERT_EQUALS(mfun->parameterName(12), "f3.h");
    TS_ASSERT_EQUALS(mfun->parameterName(13), "f3.s");

    TS_ASSERT_EQUALS(mfun->getParameter("f0.c0"), 4.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.c1"), 4.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.c2"), 4.3);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.c3"), 4.4);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.c"), 1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.h"), 1.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.s"), 103);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c0"), 2.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c1"), 104);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c2"), 105);
    TS_ASSERT_EQUALS(mfun->getParameter("f2.c3"), 2.4);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.c"), 3.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.h"), 106);
    TS_ASSERT_EQUALS(mfun->getParameter("f3.s"), 3.3);

    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.c0"), 0);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.c1"), 1);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.c2"), 2);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f0.c3"), 3);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.c"), 4);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.h"), 5);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f1.s"), 6);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c0"), 7);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c1"), 8);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c2"), 9);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f2.c3"), 10);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.c"), 11);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.h"), 12);
    TS_ASSERT_EQUALS(mfun->parameterIndex("f3.s"), 13);

    TS_ASSERT_EQUALS(mfun->activeParameter(0), 4.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(1), 4.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(2), 4.3);
    TS_ASSERT_EQUALS(mfun->activeParameter(3), 4.4);
    TS_ASSERT_EQUALS(mfun->activeParameter(4), 1.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(5), 1.2);
    TS_ASSERT_EQUALS(mfun->activeParameter(7), 2.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(10), 2.4);
    TS_ASSERT_EQUALS(mfun->activeParameter(11), 3.1);
    TS_ASSERT_EQUALS(mfun->activeParameter(13), 3.3);

    TS_ASSERT_EQUALS(mfun->nameOfActive(0), "f0.c0");
    TS_ASSERT_EQUALS(mfun->nameOfActive(1), "f0.c1");
    TS_ASSERT_EQUALS(mfun->nameOfActive(2), "f0.c2");
    TS_ASSERT_EQUALS(mfun->nameOfActive(3), "f0.c3");
    TS_ASSERT_EQUALS(mfun->nameOfActive(4), "f1.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(5), "f1.h");
    TS_ASSERT_EQUALS(mfun->nameOfActive(7), "f2.c0");
    TS_ASSERT_EQUALS(mfun->nameOfActive(10), "f2.c3");
    TS_ASSERT_EQUALS(mfun->nameOfActive(11), "f3.c");
    TS_ASSERT_EQUALS(mfun->nameOfActive(13), "f3.s");

    TS_ASSERT(!mfun->isFixed(0));
    TS_ASSERT(!mfun->isFixed(1));
    TS_ASSERT(!mfun->isFixed(2));
    TS_ASSERT(!mfun->isFixed(3));
    TS_ASSERT(!mfun->isFixed(4));
    TS_ASSERT(!mfun->isFixed(5));
    TS_ASSERT(mfun->isFixed(6));
    TS_ASSERT(!mfun->isFixed(7));
    TS_ASSERT(mfun->isFixed(8));
    TS_ASSERT(mfun->isFixed(9));
    TS_ASSERT(!mfun->isFixed(10));
    TS_ASSERT(!mfun->isFixed(11));
    TS_ASSERT(mfun->isFixed(12));
    TS_ASSERT(!mfun->isFixed(13));

    delete mfun;
  }

  void testAddFunctionsWithTies() {
    CompositeFunction *mfun = new CompositeFunction;
    IFunction_sptr g = IFunction_sptr(new Gauss());
    IFunction_sptr bk = IFunction_sptr(new Linear());

    bk->setParameter("a", 0.1);
    bk->setParameter("b", 0.2);

    bk->tie("b", "a/2");

    g->setParameter("c", 1.1);
    g->setParameter("h", 1.2);
    g->setParameter("s", 1.3);
    g->tie("s", "1.33");

    mfun->addFunction(bk);
    mfun->addFunction(g);

    mfun->tie("f1.h", "f0.b*4");

    TS_ASSERT_EQUALS(mfun->nParams(), 5);

    TS_ASSERT(mfun->isActive(0));  // f0.a
    TS_ASSERT(!mfun->isActive(1)); // f0.b
    TS_ASSERT(mfun->isActive(2));  // f1.c
    TS_ASSERT(!mfun->isActive(3)); // f1.h
    TS_ASSERT(mfun->isFixed(4));   // f1.s

    mfun->applyTies();

    TS_ASSERT_EQUALS(mfun->getParameter("f0.a"), 0.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.b"), 0.05);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.c"), 1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.h"), 0.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f1.s"), 1.33);

    delete mfun;
  }

  void testRemoveFunctionWithTies() {
    CompositeFunction *mfun = new CompositeFunction;
    IFunction_sptr g = IFunction_sptr(new Gauss());
    IFunction_sptr bk = IFunction_sptr(new Linear());

    bk->setParameter("a", 0.1);
    bk->setParameter("b", 0.2);

    g->setParameter("c", 1.1);
    g->setParameter("h", 1.2);
    g->setParameter("s", 1.3);

    mfun->addFunction(bk);
    mfun->addFunction(g);

    mfun->tie("f1.h", "f0.b*4");
    mfun->tie("f1.s", "f1.h/4");

    TS_ASSERT_EQUALS(mfun->nParams(), 5);

    mfun->removeFunction(0);

    TS_ASSERT_EQUALS(mfun->nParams(), 3);

    TS_ASSERT(mfun->isActive(0));
    TS_ASSERT(mfun->isActive(1));
    TS_ASSERT(!mfun->isActive(2));

    mfun->applyTies();

    TS_ASSERT_EQUALS(mfun->getParameter("f0.c"), 1.1);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.h"), 1.2);
    TS_ASSERT_EQUALS(mfun->getParameter("f0.s"), 0.3);

    delete mfun;
  }

  void testReplaceEmptyFunction() {
    CompositeFunction *mfun = new CompositeFunction;
    IFunction_sptr g = IFunction_sptr(new Gauss());
    IFunction_sptr cf = IFunction_sptr(new CompositeFunction());
    IFunction_sptr bk = IFunction_sptr(new Linear());
    IFunction_sptr cub = IFunction_sptr(new Cubic());

    mfun->addFunction(bk);
    mfun->addFunction(cf);
    mfun->addFunction(cub);

    mfun->replaceFunctionPtr(cf, g);

    TS_ASSERT_EQUALS(mfun->asString(), "name=Linear,a=0,b=0;name=Gauss,c=0,h=1,"
                                       "s=1;name=Cubic,c0=0,c1=0,c2=0,c3=0");

    delete mfun;
  }

  void test_setWorkspaceWorks() {
    CompositeFunction *mfun = new CompositeFunction;
    IFunction_sptr g1 = IFunction_sptr(new Gauss());
    IFunction_sptr bk = IFunction_sptr(new Linear());

    mfun->addFunction(bk);
    mfun->addFunction(g1);

    MatrixWorkspace_sptr ws(
        new CompositeFunctionTest_MocMatrixWorkspace(10, 11, 10));

    MantidVec &x = ws->dataX(3);
    MantidVec &y = ws->dataY(3);
    for (size_t i = 0; i < y.size(); ++i) {
      x[i] = 0.1 * static_cast<double>(i);
      y[i] = static_cast<double>(i);
    }
    x.back() = 0.1 * static_cast<double>(y.size());

    // Failing to explicitly construct a string here from the char* leads to the
    //   setWorkspace(Workspace_sptr, bool) overload getting called!!!
    // TS_ASSERT_THROWS_NOTHING(mfun->setWorkspace(ws,std::string("WorkspaceIndex=3,StartX=0.2,EndX
    // = 0.8")));

    delete mfun;
  }

  void test_ctreatingWithFactory() {
    std::string funStr =
        "composite=CompositeFunction,NumDeriv=true;name=Linear;name=Linear";
    auto fun = FunctionFactory::Instance().createInitialized(funStr);
    TS_ASSERT(fun);
    TS_ASSERT(fun->hasAttribute("NumDeriv"));
    bool b = fun->getAttribute("NumDeriv").asBool();
    TS_ASSERT(b);
    TS_ASSERT_EQUALS(fun->asString(), "composite=CompositeFunction,NumDeriv="
                                      "true;name=Linear,a=0,b=0;name=Linear,a="
                                      "0,b=0");

    fun = FunctionFactory::Instance().createInitialized(
        "name=Linear;name=Linear");
    TS_ASSERT(fun);
    TS_ASSERT(fun->hasAttribute("NumDeriv"));
    b = fun->getAttribute("NumDeriv").asBool();
    TS_ASSERT(!b);
  }

  void test_local_name() {
    std::string funStr = "name=Linear;(name=Linear;(name=Linear;name=Linear))";
    auto fun = boost::dynamic_pointer_cast<CompositeFunction>(
        FunctionFactory::Instance().createInitialized(funStr));
    TS_ASSERT_EQUALS(fun->parameterLocalIndex(0), 0);
    TS_ASSERT_EQUALS(fun->parameterLocalIndex(2), 0);
    TS_ASSERT_EQUALS(fun->parameterLocalIndex(4), 2);
    TS_ASSERT_EQUALS(fun->parameterLocalIndex(6), 4);

    TS_ASSERT_EQUALS(fun->parameterLocalName(0), "a");
    TS_ASSERT_EQUALS(fun->parameterLocalName(2), "f0.a");
    TS_ASSERT_EQUALS(fun->parameterLocalName(4), "f1.f0.a");
    TS_ASSERT_EQUALS(fun->parameterLocalName(6), "f1.f1.a");

    TS_ASSERT_EQUALS(fun->parameterLocalIndex(0, true), 0);
    TS_ASSERT_EQUALS(fun->parameterLocalIndex(2, true), 0);
    TS_ASSERT_EQUALS(fun->parameterLocalIndex(4, true), 0);
    TS_ASSERT_EQUALS(fun->parameterLocalIndex(6, true), 0);

    TS_ASSERT_EQUALS(fun->parameterLocalName(0, true), "a");
    TS_ASSERT_EQUALS(fun->parameterLocalName(2, true), "a");
    TS_ASSERT_EQUALS(fun->parameterLocalName(4, true), "a");
    TS_ASSERT_EQUALS(fun->parameterLocalName(6, true), "a");
  }
};

#endif /*COMPOSITEFUNCTIONTEST_H_*/
