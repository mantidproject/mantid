// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef CRYSTALFIELDSPECTRUMTEST_H_
#define CRYSTALFIELDSPECTRUMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/Functions/CrystalFieldSpectrum.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidCurveFitting/Functions/SimpleChebfun.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;

class CrystalFieldSpectrumTest : public CxxTest::TestSuite {
public:
  // Conversion factor from barn to milibarn/steradian
  const double c_mbsr = 79.5774715459;

  void test_function() {
    CrystalFieldSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperature", 44.0);
    fun.setAttributeValue("ToleranceIntensity", 0.001);
    fun.setAttributeValue("FWHM", 1.5);
    fun.buildTargetFunction();
    auto attNames = fun.getAttributeNames();
    auto parNames = fun.getParameterNames();
    TS_ASSERT_EQUALS(fun.nAttributes(), attNames.size());
    TS_ASSERT_EQUALS(fun.nParams(), parNames.size());

    auto i = fun.parameterIndex("f0.Amplitude");
    TS_ASSERT(fun.isFixed(i));
    TS_ASSERT(!fun.isActive(i));
    i = fun.parameterIndex("f0.PeakCentre");
    TS_ASSERT(fun.isFixed(i));
    TS_ASSERT(!fun.isActive(i));
    i = fun.parameterIndex("f0.FWHM");
    TS_ASSERT(!fun.isFixed(i));
    TS_ASSERT(fun.isActive(i));
    i = fun.parameterIndex("f1.Amplitude");
    TS_ASSERT(fun.isFixed(i));
    TS_ASSERT(!fun.isActive(i));
    i = fun.parameterIndex("f1.PeakCentre");
    TS_ASSERT(fun.isFixed(i));
    TS_ASSERT(!fun.isActive(i));
    i = fun.parameterIndex("f1.FWHM");
    TS_ASSERT(!fun.isFixed(i));
    TS_ASSERT(fun.isActive(i));
    i = fun.parameterIndex("f2.Amplitude");
    TS_ASSERT(fun.isFixed(i));
    TS_ASSERT(!fun.isActive(i));
    i = fun.parameterIndex("f2.PeakCentre");
    TS_ASSERT(fun.isFixed(i));
    TS_ASSERT(!fun.isActive(i));
    i = fun.parameterIndex("f2.FWHM");
    TS_ASSERT(!fun.isFixed(i));
    TS_ASSERT(fun.isActive(i));

    TS_ASSERT_DELTA(fun.getParameter("f0.PeakCentre"), 0.0, 1e-3);
    TS_ASSERT_DELTA(fun.getParameter("f0.Amplitude"), 2.749 * c_mbsr,
                    1e-3 * c_mbsr);
    TS_ASSERT_DELTA(fun.getParameter("f0.FWHM"), 1.5, 1e-3);

    TS_ASSERT_DELTA(fun.getParameter("f1.PeakCentre"), 29.3261, 1e-3);
    TS_ASSERT_DELTA(fun.getParameter("f1.Amplitude"), 0.7204 * c_mbsr,
                    1e-3 * c_mbsr);
    TS_ASSERT_DELTA(fun.getParameter("f1.FWHM"), 1.5, 1e-3);

    TS_ASSERT_DELTA(fun.getParameter("f2.PeakCentre"), 44.3412, 1e-3);
    TS_ASSERT_DELTA(fun.getParameter("f2.Amplitude"), 0.4298 * c_mbsr,
                    1e-3 * c_mbsr);
    TS_ASSERT_DELTA(fun.getParameter("f2.FWHM"), 1.5, 1e-3);

    TS_ASSERT(fun.hasParameter("B20"));
    TS_ASSERT(fun.hasParameter("B42"));
    TS_ASSERT(fun.hasParameter("f0.Amplitude"));
    TS_ASSERT(fun.hasParameter("f0.FWHM"));
    TS_ASSERT(fun.hasParameter("f2.Amplitude"));
    TS_ASSERT(fun.hasParameter("f2.PeakCentre"));
    TS_ASSERT(!fun.hasParameter("Hello"));
    TS_ASSERT(!fun.hasParameter("f0.Hello"));
  }

  void test_evaluate() {
    auto fun =
        boost::shared_ptr<CrystalFieldSpectrum>(new CrystalFieldSpectrum);
    fun->setParameter("B20", 0.37737);
    fun->setParameter("B22", 3.9770);
    fun->setParameter("B40", -0.031787);
    fun->setParameter("B42", -0.11611);
    fun->setParameter("B44", -0.12544);
    fun->setAttributeValue("Ion", "Ce");
    fun->setAttributeValue("Temperature", 44.0);
    fun->setAttributeValue("ToleranceIntensity", 0.001 * c_mbsr);
    fun->buildTargetFunction();
    fun->setParameter("f0.FWHM", 2.0);
    fun->setParameter("f1.FWHM", 20.0);
    fun->setParameter("f2.FWHM", 20.0);
    fun->setParameter("IntensityScaling", 1 / c_mbsr);
    FunctionDomain1DVector x(0.0, 55.0, 100);
    FunctionValues y(x);
    fun->function(x, y);

    auto testFun = FunctionFactory::Instance().createInitialized(
        "name=Lorentzian,PeakCentre=0.0,Amplitude=2.749,FWHM=2.0;"
        "name=Lorentzian,PeakCentre=29.3261,Amplitude=0.7204,FWHM=20.0;"
        "name=Lorentzian,PeakCentre=44.3412,Amplitude=0.4298,FWHM=20.0;");
    FunctionValues t(x);
    testFun->function(x, t);

    for (size_t i = 0; i < x.size(); ++i) {
      TS_ASSERT_DELTA(y[i] / t[i], 1, 2e-4);
    }
  }

  void test_evaluate_gaussian() {
    auto fun =
        boost::shared_ptr<CrystalFieldSpectrum>(new CrystalFieldSpectrum);
    fun->setParameter("B20", 0.37737);
    fun->setParameter("B22", 3.9770);
    fun->setParameter("B40", -0.031787);
    fun->setParameter("B42", -0.11611);
    fun->setParameter("B44", -0.12544);
    fun->setAttributeValue("Ion", "Ce");
    fun->setAttributeValue("Temperature", 44.0);
    fun->setAttributeValue("ToleranceIntensity", 0.001 * c_mbsr);
    fun->setAttributeValue("PeakShape", "Gaussian");
    fun->buildTargetFunction();
    fun->setParameter("f0.Sigma", 10.0);
    fun->setParameter("f1.Sigma", 2.0);
    fun->setParameter("f2.Sigma", 3.0);
    fun->setParameter("IntensityScaling", 1 / c_mbsr);
    FunctionDomain1DVector x(0.0, 55.0, 100);
    FunctionValues y(x);
    fun->function(x, y);

    auto height1 = std::to_string(2.749 / (10.0 * sqrt(2.0 * M_PI)));
    auto height2 = std::to_string(0.7204 / (2.0 * sqrt(2.0 * M_PI)));
    auto height3 = std::to_string(0.4298 / (3.0 * sqrt(2.0 * M_PI)));
    auto testFun = FunctionFactory::Instance().createInitialized(
        "name=Gaussian,PeakCentre=0.0,Height=" + height1 +
        ",Sigma=10.0;"
        "name=Gaussian,PeakCentre=29.3261,Height=" +
        height2 +
        ",Sigma=2.0;"
        "name=Gaussian,PeakCentre=44.3412,Height=" +
        height3 + ",Sigma=3.0;");
    FunctionValues t(x);
    testFun->function(x, t);

    for (size_t i = 0; i < x.size(); ++i) {
      TS_ASSERT_DELTA(y[i] / t[i], 1, 2e-4);
    }
  }

  void test_factory() {
    std::string funDef =
        "name=CrystalFieldSpectrum,Ion=Ce,Symmetry=C2v,"
        "Temperature=44,ToleranceIntensity=0.002, B20=0.37,B22=3.9,"
        "B40=-0.03,B42=-0.1,B44=-0.12, "
        "f0.FWHM=2.2,f1.FWHM=1.8, "
        "ties=(B60=0,B62=0,B64=0,B66=0,BmolX=0,BmolY=0,BmolZ=0,"
        "BextX=0,BextY=0,BextZ=0,f2.FWHM=2.1)";
    auto fun = FunctionFactory::Instance().createInitialized(funDef);
    TS_ASSERT(fun);
    TS_ASSERT_EQUALS(fun->name(), "CrystalFieldSpectrum");
    TS_ASSERT_EQUALS(fun->getAttribute("Ion").asString(), "Ce");
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetry").asString(), "C2v");
    TS_ASSERT_EQUALS(fun->getAttribute("Temperature").asDouble(), 44.0);
    TS_ASSERT_EQUALS(fun->getAttribute("ToleranceIntensity").asDouble(), 0.002);
    TS_ASSERT_EQUALS(fun->getParameter("B20"), 0.37);
    TS_ASSERT_EQUALS(fun->getParameter("B42"), -0.1);
    TS_ASSERT_EQUALS(fun->getParameter("f1.FWHM"), 1.8);

    auto i = fun->parameterIndex("f2.FWHM");
    auto tie = fun->getTie(i);
    TS_ASSERT(!tie);
    TS_ASSERT(fun->isFixed(i));
    TS_ASSERT_EQUALS(fun->getParameter(i), 2.1);
    i = fun->parameterIndex("B60");
    tie = fun->getTie(i);
    TS_ASSERT(!tie);
    TS_ASSERT(fun->isFixed(i));
    TS_ASSERT_EQUALS(fun->getParameter(i), 0);
    i = fun->parameterIndex("BmolY");
    tie = fun->getTie(i);
    TS_ASSERT(!tie);
    TS_ASSERT(fun->isFixed(i));
    TS_ASSERT_EQUALS(fun->getParameter(i), 0);

    size_t nTies = 0;
    for (size_t i = 0; i < fun->nParams(); ++i) {
      auto tie = fun->getTie(i);
      if (tie) {
        ++nTies;
      }
    }
    TS_ASSERT_EQUALS(nTies, 0);
  }

  void test_constraints() {
    std::string funDef =
        "name=CrystalFieldSpectrum,Ion=Ce,B20=0.37,B22=3.9,"
        "B40=-0.03,B42=-0.1,B44=-0.12,constraints=(0<B44<10,f1.FWHM>1.3)";
    auto fun = FunctionFactory::Instance().createInitialized(funDef);
    TS_ASSERT(fun);
    auto i = fun->parameterIndex("f1.FWHM");
    auto constraint = fun->getConstraint(i);
    TS_ASSERT(constraint);
    if (constraint) {
      TS_ASSERT_EQUALS(constraint->asString(), "1.3<f1.FWHM");
      TS_ASSERT_EQUALS(constraint->getLocalIndex(), 39);
    }
    i = fun->parameterIndex("B44");
    constraint = fun->getConstraint(i);
    TS_ASSERT(constraint);
    if (constraint) {
      TS_ASSERT_EQUALS(constraint->asString(), "0<B44<10");
      TS_ASSERT_EQUALS(constraint->getLocalIndex(), 13);
    }
  }

  void test_calculated_widths() {
    CrystalFieldSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperature", 44.0);

    std::vector<double> x{0.0, 50.0};
    std::vector<double> y{1.0, 2.0};
    fun.setAttributeValue("FWHMX", x);
    fun.setAttributeValue("FWHMY", y);
    auto checkW = [&x, &y](double c) {
      return y.front() +
             (y.back() - y.front()) / (x.back() - x.front()) * (c - x.front());
    };
    fun.buildTargetFunction();
    {
      auto c = fun.getParameter("f0.PeakCentre");
      auto w = fun.getParameter("f0.FWHM");
      TS_ASSERT_EQUALS(w, checkW(c));
      auto ct = getBounds(fun, "f0.FWHM");
      TS_ASSERT_DELTA(ct.first, 0.9, 1e-4);
      TS_ASSERT_DELTA(ct.second, 1.1, 1e-4);
    }
    {
      auto c = fun.getParameter("f1.PeakCentre");
      auto w = fun.getParameter("f1.FWHM");
      TS_ASSERT_EQUALS(w, checkW(c));
      auto ct = getBounds(fun, "f1.FWHM");
      TS_ASSERT_DELTA(ct.first, 1.4865, 1e-4);
      TS_ASSERT_DELTA(ct.second, 1.6865, 1e-4);
    }
    {
      auto c = fun.getParameter("f2.PeakCentre");
      auto w = fun.getParameter("f2.FWHM");
      TS_ASSERT_EQUALS(w, checkW(c));
      auto ct = getBounds(fun, "f2.FWHM");
      TS_ASSERT_DELTA(ct.first, 1.7868, 1e-4);
      TS_ASSERT_DELTA(ct.second, 1.9868, 1e-4);
    }
    {
      auto w = fun.getParameter("f3.FWHM");
      TS_ASSERT_EQUALS(w, 0.0);
    }
  }

  void test_calculate_widths_out_range() {
    CrystalFieldSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperature", 44.0);

    {
      std::vector<double> x{0.0, 10.0};
      std::vector<double> y{1.0, 2.0};
      fun.setAttributeValue("FWHMX", x);
      fun.setAttributeValue("FWHMY", y);
      fun.buildTargetFunction();
      TS_ASSERT_DIFFERS(fun.getParameter("f0.Amplitude"), 0.0);
      TS_ASSERT_EQUALS(fun.getParameter("f1.Amplitude"), 0.0);
      TS_ASSERT_EQUALS(fun.getParameter("f2.Amplitude"), 0.0);
    }
    {
      std::vector<double> x{1.0, 50.0};
      std::vector<double> y{1.0, 2.0};
      fun.setAttributeValue("FWHMX", x);
      fun.setAttributeValue("FWHMY", y);
      TS_ASSERT_EQUALS(fun.getParameter("f0.Amplitude"), 0.0);
      TS_ASSERT_DIFFERS(fun.getParameter("f1.Amplitude"), 0.0);
      TS_ASSERT_DIFFERS(fun.getParameter("f2.Amplitude"), 0.0);
    }
  }

  void test_calculate_widths_different_sizes1() {
    CrystalFieldSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperature", 44.0);

    {
      std::vector<double> x{0.0, 10.0, 50.0};
      std::vector<double> y{1.0, 2.0};
      fun.setAttributeValue("FWHMX", x);
      fun.setAttributeValue("FWHMY", y);
      TS_ASSERT_THROWS(fun.buildTargetFunction(), std::runtime_error);
    }
    {
      std::vector<double> x{0.0, 50.0};
      std::vector<double> y{1.0, 2.0, 3.0};
      fun.setAttributeValue("FWHMX", x);
      fun.setAttributeValue("FWHMY", y);
      TS_ASSERT_THROWS(fun.buildTargetFunction(), std::runtime_error);
    }
    {
      std::vector<double> x;
      std::vector<double> y{1.0, 2.0};
      fun.setAttributeValue("FWHMX", x);
      fun.setAttributeValue("FWHMY", y);
      TS_ASSERT_THROWS(fun.buildTargetFunction(), std::runtime_error);
    }
    {
      std::vector<double> x{0.0, 10.0, 50.0};
      std::vector<double> y;
      fun.setAttributeValue("FWHMX", x);
      fun.setAttributeValue("FWHMY", y);
      TS_ASSERT_THROWS(fun.buildTargetFunction(), std::runtime_error);
    }
  }

  void test_calculated_widths_longer_vectors() {
    CrystalFieldSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperature", 44.0);

    auto wFun = [](double x) { return 2.0 + sin(M_PI * x / 50.0); };
    SimpleChebfun cFun(wFun, 0.0, 50.0);
    std::vector<double> x = cFun.linspace(30);
    std::vector<double> y = cFun(x);
    fun.setAttributeValue("FWHMX", x);
    fun.setAttributeValue("FWHMY", y);
    fun.buildTargetFunction();

    {
      auto c = fun.getParameter("f0.PeakCentre");
      auto w = fun.getParameter("f0.FWHM");
      TS_ASSERT_DELTA(w, wFun(c), 1e-3);
    }
    {
      auto c = fun.getParameter("f1.PeakCentre");
      auto w = fun.getParameter("f1.FWHM");
      TS_ASSERT_DELTA(w, wFun(c), 1e-3);
    }
    {
      auto c = fun.getParameter("f2.PeakCentre");
      auto w = fun.getParameter("f2.FWHM");
      TS_ASSERT_DELTA(w, wFun(c), 1e-3);
    }
    {
      auto w = fun.getParameter("f3.FWHM");
      TS_ASSERT_EQUALS(w, 0.0);
    }
  }

  void test_calculated_widths_gaussian() {
    CrystalFieldSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperature", 44.0);
    fun.setAttributeValue("PeakShape", "Gaussian");

    std::vector<double> x{0.0, 50.0};
    std::vector<double> y{1.0, 2.0};
    fun.setAttributeValue("FWHMX", x);
    fun.setAttributeValue("FWHMY", y);
    auto checkW = [&x, &y](double c) {
      return y.front() +
             (y.back() - y.front()) / (x.back() - x.front()) * (c - x.front());
    };

    fun.buildTargetFunction();
    Gaussian gauss;
    gauss.initialize();
    {
      auto c = fun.getParameter("f0.PeakCentre");
      auto w = fun.getParameter("f0.Sigma");
      gauss.setFwhm(checkW(c));
      TS_ASSERT_EQUALS(w, gauss.getParameter("Sigma"));
      auto ct = getBounds(fun, "f0.Sigma");
      TS_ASSERT_DELTA(ct.first, 0.3821, 1e-4);
      TS_ASSERT_DELTA(ct.second, 0.4671, 1e-4);
    }
    {
      auto c = fun.getParameter("f1.PeakCentre");
      auto w = fun.getParameter("f1.Sigma");
      gauss.setFwhm(checkW(c));
      TS_ASSERT_EQUALS(w, gauss.getParameter("Sigma"));
      auto ct = getBounds(fun, "f1.Sigma");
      TS_ASSERT_DELTA(ct.first, 0.6312, 1e-4);
      TS_ASSERT_DELTA(ct.second, 0.7162, 1e-4);
    }
    {
      auto c = fun.getParameter("f2.PeakCentre");
      auto w = fun.getParameter("f2.Sigma");
      gauss.setFwhm(checkW(c));
      TS_ASSERT_EQUALS(w, gauss.getParameter("Sigma"));
      auto ct = getBounds(fun, "f2.Sigma");
      TS_ASSERT_DELTA(ct.first, 0.7587, 1e-4);
      TS_ASSERT_DELTA(ct.second, 0.8437, 1e-4);
    }
    {
      auto w = fun.getParameter("f3.Sigma");
      TS_ASSERT_EQUALS(w, 0.0);
    }
  }

  void test_calculated_widths_non_default_bounds() {
    CrystalFieldSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperature", 44.0);
    fun.setAttributeValue("FWHMVariation", 1.1);

    std::vector<double> x{0.0, 50.0};
    std::vector<double> y{1.0, 2.0};
    fun.setAttributeValue("FWHMX", x);
    fun.setAttributeValue("FWHMY", y);
    auto checkW = [&x, &y](double c) {
      return y.front() +
             (y.back() - y.front()) / (x.back() - x.front()) * (c - x.front());
    };
    fun.buildTargetFunction();
    {
      auto c = fun.getParameter("f0.PeakCentre");
      auto w = fun.getParameter("f0.FWHM");
      TS_ASSERT_EQUALS(w, checkW(c));
      auto ct = getBounds(fun, "f0.FWHM");
      TS_ASSERT_DELTA(ct.first, 0.0, 1e-4);
      TS_ASSERT_DELTA(ct.second, 2.1, 1e-4);
    }
    {
      auto c = fun.getParameter("f1.PeakCentre");
      auto w = fun.getParameter("f1.FWHM");
      TS_ASSERT_EQUALS(w, checkW(c));
      auto ct = getBounds(fun, "f1.FWHM");
      TS_ASSERT_DELTA(ct.first, 0.4865, 1e-4);
      TS_ASSERT_DELTA(ct.second, 2.6865, 1e-4);
    }
    {
      auto c = fun.getParameter("f2.PeakCentre");
      auto w = fun.getParameter("f2.FWHM");
      TS_ASSERT_EQUALS(w, checkW(c));
      auto ct = getBounds(fun, "f2.FWHM");
      TS_ASSERT_DELTA(ct.first, 0.7868, 1e-4);
      TS_ASSERT_DELTA(ct.second, 2.9868, 1e-4);
    }
    {
      auto w = fun.getParameter("f3.FWHM");
      TS_ASSERT_EQUALS(w, 0.0);
    }
  }

  void test_calculated_widths_update() {
    CrystalFieldSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperature", 44.0);

    std::vector<double> x{0.0, 50.0};
    std::vector<double> y{1.0, 2.0};
    fun.setAttributeValue("FWHMX", x);
    fun.setAttributeValue("FWHMY", y);
    fun.setAttributeValue("FWHMVariation", 0.01);
    auto checkW = [&x, &y](double c) {
      return y.front() +
             (y.back() - y.front()) / (x.back() - x.front()) * (c - x.front());
    };
    fun.buildTargetFunction();
    {
      auto c = fun.getParameter("f1.PeakCentre");
      auto w = fun.getParameter("f1.FWHM");
      TS_ASSERT_EQUALS(w, checkW(c));
      auto ct = getBounds(fun, "f1.FWHM");
      TS_ASSERT_DELTA(ct.first, 1.5765, 1e-4);
      TS_ASSERT_DELTA(ct.second, 1.5965, 1e-4);
    }
    fun.setParameter("B20", 0.57737);
    fun.setParameter("B22", 2.9770);
    {
      auto c = fun.getParameter("f1.PeakCentre");
      auto w = fun.getParameter("f1.FWHM");
      TS_ASSERT_EQUALS(w, checkW(c));
      auto ct = getBounds(fun, "f1.FWHM");
      TS_ASSERT_DELTA(ct.first, 1.6879, 1e-4);
      TS_ASSERT_DELTA(ct.second, 1.7079, 1e-4);
    }
  }

  void test_calculated_widths_update_gaussian() {
    CrystalFieldSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperature", 44.0);
    fun.setAttributeValue("PeakShape", "Gaussian");

    std::vector<double> x{0.0, 50.0};
    std::vector<double> y{1.0, 2.0};
    fun.setAttributeValue("FWHMX", x);
    fun.setAttributeValue("FWHMY", y);
    fun.setAttributeValue("FWHMVariation", 0.01);
    auto checkW = [&x, &y](double c) {
      return y.front() +
             (y.back() - y.front()) / (x.back() - x.front()) * (c - x.front());
    };
    Gaussian gauss;
    gauss.initialize();
    fun.buildTargetFunction();
    {
      auto c = fun.getParameter("f1.PeakCentre");
      auto w = fun.getParameter("f1.Sigma");
      gauss.setFwhm(checkW(c));
      TS_ASSERT_EQUALS(w, gauss.getParameter("Sigma"));
      auto ct = getBounds(fun, "f1.Sigma");
      TS_ASSERT_DELTA(ct.first, 0.6694, 1e-4);
      TS_ASSERT_DELTA(ct.second, 0.6779, 1e-4);
    }
    fun.setParameter("B20", 0.57737);
    fun.setParameter("B22", 2.9770);
    {
      auto c = fun.getParameter("f1.PeakCentre");
      auto w = fun.getParameter("f1.Sigma");
      gauss.setFwhm(checkW(c));
      TS_ASSERT_EQUALS(w, gauss.getParameter("Sigma"));
      auto ct = getBounds(fun, "f1.Sigma");
      TS_ASSERT_DELTA(ct.first, 0.7167, 1e-4);
      TS_ASSERT_DELTA(ct.second, 0.7252, 1e-4);
    }
  }

  void test_monte_carlo() {
    CrystalFieldSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperature", 44.0);
    fun.setAttributeValue("FWHM", 1.0);
    auto ws = createWorkspace(fun, 0, 50, 100);
    auto mc = AlgorithmFactory::Instance().create("EstimateFitParameters", -1);
    mc->initialize();
    mc->setRethrows(true);
    mc->setPropertyValue(
        "Function",
        "name=CrystalFieldSpectrum,Ion=Ce,"
        "Symmetry=C2v,Temperature=44.0,FWHM=1.0,NPeaks=3,FixAllPeaks=1,"
        "constraints=(0<B20<2,1<B22<4,-0.1<B40<0.1,-0.1<B42<0.1,-0.1<B44<0.1)");
    mc->setProperty("InputWorkspace", ws);
    mc->setProperty("NSamples", 1000);
    mc->setProperty("Constraints", "0<f2.PeakCentre<50");
    mc->execute();
    IFunction_sptr func = mc->getProperty("Function");

    auto fit = AlgorithmFactory::Instance().create("Fit", -1);
    fit->initialize();
    fit->setProperty("Function", func);
    fit->setProperty("InputWorkspace", ws);
    fit->execute();
    double chi2 = fit->getProperty("OutputChi2overDoF");
    TS_ASSERT_LESS_THAN(chi2, 100.0);
  }

  void test_change_number_of_fixed_params() {

    std::string funDef =
        "name=CrystalFieldSpectrum,Ion=Ce,Symmetry=C2v,"
        "Temperature=44,FWHM=1.0,B20=0.37737,B22=3.977,"
        "B40=-0.031787,B42=-0.11611,B44=-0.12544, "
        "ties=(B60=0,B62=0,B64=0,B66=0,BmolX=0,BmolY=0,BmolZ=0,"
        "BextX=0,BextY=0,BextZ=0,f2.FWHM=2.1);"
        "name=CrystalFieldSpectrum,Ion=Pr,Symmetry=C2v,"
        "Temperature=44,FWHM=1.0,B20=0.37737,B22=3.977,"
        "B40=-0.031787,B42=-0.11611,B44=-0.12544, "
        "ties=(B60=0,B62=0,B64=0,B66=0,BmolX=0,BmolY=0,BmolZ=0,"
        "BextX=0,BextY=0,BextZ=0)";
    auto fun = FunctionFactory::Instance().createInitialized(funDef);
    auto ws = createWorkspace(*fun, -20, 170, 100);

    funDef = "name=CrystalFieldSpectrum,Ion=Ce,Symmetry=C2v,Temperature=44.0,"
             "ToleranceEnergy=1e-10,ToleranceIntensity=0.1,PeakShape="
             "Lorentzian,FWHM=1.1,B44=-0.125,B40=-0.03,B42=-0.116,ties=(IB63="
             "0,IB62=0,IB61=0,IB66=0,IB65=0,IB64=0,IB41=0,IB43=0,IB42=0,IB44="
             "0,B22=3.977,B21=0,B20=0.37737,IB22=0,IB21=0,BextX=0,BextY=0,"
             "BextZ=0,B66=0,B63=0,B62=0,B61=0,B60=0,B41=0,B43=0,B65=0,B64=0,"
             "BmolZ=0,BmolY=0,BmolX=0);name=CrystalFieldSpectrum,Ion=Pr,"
             "Symmetry=C2v,Temperature=44.0,ToleranceEnergy=1.0,"
             "ToleranceIntensity=6.0,PeakShape=Lorentzian,FWHM=1.1,B44=-0."
             "125,B40=-0.03,B42=-0.116,ties=(IB63=0,IB62=0,IB61=0,IB66=0,"
             "IB65=0,IB64=0,IB41=0,IB43=0,IB42=0,IB44=0,B22=3.977,B21=0,B20="
             "0.37737,IB22=0,IB21=0,BextX=0,BextY=0,BextZ=0,B66=0,B63=0,B62="
             "0,B61=0,B60=0,B41=0,B43=0,B65=0,B64=0,BmolZ=0,BmolY=0,BmolX=0)";
    fun = FunctionFactory::Instance().createInitialized(funDef);
    auto fit = AlgorithmFactory::Instance().create("Fit", -1);
    fit->setRethrows(true);
    fit->initialize();
    fit->setProperty("Function", fun);
    fit->setProperty("InputWorkspace", ws);
    fit->setProperty("Output", "out");
    TS_ASSERT_THROWS_NOTHING(fit->execute());
  }

  void test_ties_in_composite_function() {
    std::string funDef =
        "name=CrystalFieldSpectrum,Ion=Ce,Symmetry=C2v,Temperature=44.0,"
        "ToleranceEnergy=1e-10,ToleranceIntensity=0.1,FixAllPeaks=False,"
        "PeakShape=Lorentzian,FWHM=1.1,B44=-0.12544,B20=0.37737,B22=3.977,B40=-"
        "0.031787,B42=-0.11611;name=CrystalFieldSpectrum,Ion=Pr,Symmetry=C2v,"
        "Temperature="
        "44.0,ToleranceEnergy=1e-10,ToleranceIntensity=0.1,FixAllPeaks=False,"
        "PeakShape=Lorentzian,FWHM=1.1,B44=-0.12544,B20=0.37737,B22=3.977,B40=-"
        "0.031787,B42=-0.11611;ties=(f1.IntensityScaling=2.0*f0."
        "IntensityScaling,f0.f1.FWHM=f1.f2.FWHM/2)";
    auto fun = FunctionFactory::Instance().createInitialized(funDef);
    {
      auto index = fun->parameterIndex("f1.IntensityScaling");
      auto tie = fun->getTie(index);
      TS_ASSERT(tie);
      if (!tie) {
        return;
      }
      TS_ASSERT_EQUALS(tie->asString(),
                       "f1.IntensityScaling=2.0*f0.IntensityScaling");
    }
    {
      auto index = fun->parameterIndex("f0.f1.FWHM");
      auto tie = fun->getTie(index);
      TS_ASSERT(tie);
      if (!tie) {
        return;
      }
      TS_ASSERT_EQUALS(tie->asString(), "f0.f1.FWHM=f1.f2.FWHM/2");
    }
  }

  void test_new_peaks() {
    std::string funDef = "name=CrystalFieldSpectrum,Ion=Ce,Symmetry=C2v,"
                         "Temperature=44.0,FWHM=1.1";
    auto fun = FunctionFactory::Instance().createInitialized(funDef);
    TS_ASSERT_EQUALS(fun->nParams(), 40);
    TS_ASSERT_DELTA(fun->getParameter(34), 310.38, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(35), 0.00, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(36), 1.10, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(37), 0.00, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(38), 0.00, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(39), 1.10, 1e-2);
    TS_ASSERT(fun->isActive(36));
    TS_ASSERT(!fun->isActive(39));

    fun->setParameter("B20", 0.37737);
    fun->setParameter("B22", 3.977);
    fun->setParameter("B40", 0.031787);
    fun->setParameter("B42", -0.11611);

    TS_ASSERT_EQUALS(fun->nParams(), 49);
    TS_ASSERT_DELTA(fun->getParameter(34), 203.87, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(35), 0.00, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(36), 1.10, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(37), 86.29, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(38), 27.04, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(39), 1.10, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(40), 20.08, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(41), 44.24, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(42), 1.1, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(43), 0, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(45), 1.1, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(46), 0, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(48), 1.1, 1e-2);
    TS_ASSERT(fun->isActive(36));
    TS_ASSERT(fun->isActive(39));
    TS_ASSERT(fun->isActive(42));
    TS_ASSERT(!fun->isActive(45));
    TS_ASSERT(!fun->isActive(48));

    fun->setParameter("B20", 0);
    fun->setParameter("B22", 0);
    fun->setParameter("B40", 0);
    fun->setParameter("B42", 0);

    TS_ASSERT_EQUALS(fun->nParams(), 49);
    TS_ASSERT_DELTA(fun->getParameter(34), 310.38, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(35), 0.00, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(36), 1.10, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(37), 0.00, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(39), 1.10, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(40), 0.0, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(42), 1.1, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(43), 0, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(45), 1.1, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(46), 0, 1e-2);
    TS_ASSERT_DELTA(fun->getParameter(48), 1.1, 1e-2);
    TS_ASSERT(fun->isActive(36));
    TS_ASSERT(!fun->isActive(39));
    TS_ASSERT(!fun->isActive(42));
    TS_ASSERT(!fun->isActive(45));
    TS_ASSERT(!fun->isActive(48));
  }

  void test_new_peaks_fixed_peak_width() {
    std::string funDef = "name=CrystalFieldSpectrum,Ion=Ce,Symmetry=C2v,"
                         "Temperature=44.0,FWHM=1.1";
    auto fun = FunctionFactory::Instance().createInitialized(funDef);
    TS_ASSERT_EQUALS(fun->nParams(), 40);
    TS_ASSERT(fun->isActive(36));
    TS_ASSERT(!fun->isActive(39));

    fun->setParameter("B20", 0.37737);
    fun->setParameter("B22", 3.977);
    fun->setParameter("B40", 0.031787);
    fun->setParameter("B42", -0.11611);

    fun->fix(39);

    TS_ASSERT_EQUALS(fun->nParams(), 49);
    TS_ASSERT(fun->isActive(36));
    TS_ASSERT(!fun->isActive(39));
    TS_ASSERT(fun->isActive(42));
    TS_ASSERT(!fun->isActive(45));
    TS_ASSERT(!fun->isActive(48));

    fun->setParameter("B20", 0);
    fun->setParameter("B22", 0);
    fun->setParameter("B40", 0);
    fun->setParameter("B42", 0);

    TS_ASSERT(fun->isActive(36));
    TS_ASSERT(!fun->isActive(39));
    TS_ASSERT(!fun->isActive(42));
    TS_ASSERT(!fun->isActive(45));
    TS_ASSERT(!fun->isActive(48));

    fun->setParameter("B20", 0.37737);
    fun->setParameter("B22", 3.977);
    fun->setParameter("B40", 0.031787);
    fun->setParameter("B42", -0.11611);

    TS_ASSERT(fun->isActive(36));
    TS_ASSERT(!fun->isActive(39));
    TS_ASSERT(fun->isActive(42));
    TS_ASSERT(!fun->isActive(45));
    TS_ASSERT(!fun->isActive(48));
  }

  void test_new_peaks_tied_peak_width() {
    std::string funDef = "name=CrystalFieldSpectrum,Ion=Ce,Symmetry=C2v,"
                         "Temperature=44.0,FWHM=1.1";
    auto fun = FunctionFactory::Instance().createInitialized(funDef);
    TS_ASSERT_EQUALS(fun->nParams(), 40);
    TS_ASSERT(fun->isActive(36));
    TS_ASSERT(!fun->isActive(39));

    fun->setParameter("B20", 0.37737);
    fun->setParameter("B22", 3.977);
    fun->setParameter("B40", 0.031787);
    fun->setParameter("B42", -0.11611);

    fun->tie("f1.FWHM", "f0.FWHM");

    TS_ASSERT_EQUALS(fun->nParams(), 49);
    TS_ASSERT(fun->isActive(36));
    TS_ASSERT(!fun->isActive(39));
    TS_ASSERT(fun->isActive(42));
    TS_ASSERT(!fun->isActive(45));
    TS_ASSERT(!fun->isActive(48));

    fun->setParameter("B20", 0);
    fun->setParameter("B22", 0);
    fun->setParameter("B40", 0);
    fun->setParameter("B42", 0);

    TS_ASSERT(fun->isActive(36));
    TS_ASSERT(!fun->isActive(39));
    TS_ASSERT(!fun->isActive(42));
    TS_ASSERT(!fun->isActive(45));
    TS_ASSERT(!fun->isActive(48));

    fun->setParameter("B20", 0.37737);
    fun->setParameter("B22", 3.977);
    fun->setParameter("B40", 0.031787);
    fun->setParameter("B42", -0.11611);

    TS_ASSERT(fun->isActive(36));
    TS_ASSERT(!fun->isActive(39));
    TS_ASSERT(fun->isActive(42));
    TS_ASSERT(!fun->isActive(45));
    TS_ASSERT(!fun->isActive(48));
  }

private:
  std::pair<double, double> getBounds(API::IFunction &fun,
                                      const std::string &parName) {
    auto ct = fun.getConstraint(fun.parameterIndex(parName));
    if (ct == nullptr) {
      throw std::runtime_error("Parameter " + parName +
                               " doesn't have constraint");
    }
    auto bc = dynamic_cast<Constraints::BoundaryConstraint *>(ct);
    if (ct == nullptr) {
      throw std::runtime_error("Parameter " + parName +
                               " doesn't have boundary constraint");
    }
    return std::make_pair(bc->lower(), bc->upper());
  }

  MatrixWorkspace_sptr createWorkspace(const IFunction &fun, double x0,
                                       double x1, size_t nbins) {
    auto ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, nbins, nbins);
    FunctionDomain1DVector x(x0, x1, nbins);
    FunctionValues y(x);
    fun.function(x, y);
    ws->mutableX(0) = x.toVector();
    ws->mutableY(0) = y.toVector();
    return ws;
  }
};

#endif /*CRYSTALFIELDSPECTRUMTEST_H_*/
