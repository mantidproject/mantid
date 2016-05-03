#ifndef CRYSTALFIELDSPECTRUMTEST_H_
#define CRYSTALFIELDSPECTRUMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidCurveFitting/Functions/CrystalFieldSpectrum.h"
#include "MantidCurveFitting/Algorithms/Fit.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::CurveFitting::Functions;

class CrystalFieldSpectrumTest : public CxxTest::TestSuite {
public:
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
    fun.buildSpectrumFunction();
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
    TS_ASSERT_DELTA(fun.getParameter("f0.Amplitude"), 2.749, 1e-3);
    TS_ASSERT_DELTA(fun.getParameter("f0.FWHM"), 1.5, 1e-3);

    TS_ASSERT_DELTA(fun.getParameter("f1.PeakCentre"), 29.3261, 1e-3);
    TS_ASSERT_DELTA(fun.getParameter("f1.Amplitude"), 0.7204, 1e-3);
    TS_ASSERT_DELTA(fun.getParameter("f1.FWHM"), 1.5, 1e-3);

    TS_ASSERT_DELTA(fun.getParameter("f2.PeakCentre"), 44.3412, 1e-3);
    TS_ASSERT_DELTA(fun.getParameter("f2.Amplitude"), 0.4298, 1e-3);
    TS_ASSERT_DELTA(fun.getParameter("f2.FWHM"), 1.5, 1e-3);
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
    fun->setAttributeValue("ToleranceIntensity", 0.001);
    fun->buildSpectrumFunction();
    fun->setParameter("f0.FWHM", 2.0);
    fun->setParameter("f1.FWHM", 20.0);
    fun->setParameter("f2.FWHM", 20.0);
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
    fun->setAttributeValue("ToleranceIntensity", 0.001);
    fun->setAttributeValue("PeakShape", "Gaussian");
    fun->buildSpectrumFunction();
    fun->setParameter("f0.Sigma", 10.0);
    fun->setParameter("f1.Sigma", 2.0);
    fun->setParameter("f2.Sigma", 3.0);
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
        height2 + ",Sigma=2.0;"
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
    TS_ASSERT(tie);
    if (tie) {
      TS_ASSERT_EQUALS(tie->asString(), "f2.FWHM=2.1")
    }
    i = fun->parameterIndex("B60");
    tie = fun->getTie(i);
    TS_ASSERT(tie);
    if (tie) {
      TS_ASSERT_EQUALS(tie->asString(), "B60=0")
    }
    i = fun->parameterIndex("BmolY");
    tie = fun->getTie(i);
    TS_ASSERT(tie);
    if (tie) {
      TS_ASSERT_EQUALS(tie->asString(), "BmolY=0")
    }

    size_t nTies = 0;
    for (size_t i = 0; i < fun->nParams(); ++i) {
      auto tie = fun->getTie(i);
      if (tie) {
        ++nTies;
      }
    }
    TS_ASSERT_EQUALS(nTies, 11);
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
      TS_ASSERT_EQUALS(constraint->asString(), "1.3<FWHM");
      TS_ASSERT_EQUALS(constraint->getIndex(), 2);
    }
    i = fun->parameterIndex("B44");
    constraint = fun->getConstraint(i);
    TS_ASSERT(constraint);
    if (constraint) {
      TS_ASSERT_EQUALS(constraint->asString(), "0<B44<10");
      TS_ASSERT_EQUALS(constraint->getIndex(), 13);
    }
  }
};

#endif /*CRYSTALFIELDSPECTRUMTEST_H_*/
