#ifndef CRYSTALFIELDSPECTRUMTEST_H_
#define CRYSTALFIELDSPECTRUMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Functions/CrystalFieldSpectrum.h"
#include "MantidCurveFitting/Algorithms/Fit.h"

#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid;
using namespace Mantid::API;
//using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::CurveFitting::Functions;


class CrystalFieldSpectrumTest : public CxxTest::TestSuite {
public:

  void xtest_function() {
    std::cerr << "\n\nTest function\n\n";
    CrystalFieldSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperature", 44.0);
    fun.setAttributeValue("ToleranceIntensity", 0.001);
    fun.buildSpectrumFunction();
    std::cerr << fun.nParams() << ' ' << fun.nAttributes() << std::endl;
    auto attNames = fun.getAttributeNames();
    auto parNames = fun.getParameterNames();
    TS_ASSERT_EQUALS(fun.nAttributes(), attNames.size());
    TS_ASSERT_EQUALS(fun.nParams(), parNames.size());

    //std::cerr << "\nAttributes:" << std::endl;
    //for(auto &name: attNames) {
    //  std::cerr << name << std::endl;
    //}
    //std::cerr << "\nParameters:" << std::endl;
    //for(size_t i = 0; i < parNames.size(); ++i) {
    //  auto &name = parNames[i];
    //  std::cerr << i << ' ' << fun.parameterIndex(name) << ' ' << name << ' ' << fun.isFixed(i) << fun.isActive(i) << std::endl;
    //}
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

    std::cerr << fun.getParameter("f0.PeakCentre") << ' ' << fun.getParameter("f0.Amplitude") << ' ' << fun.getParameter("f0.FWHM") << std::endl;
    std::cerr << fun.getParameter("f1.PeakCentre") << ' ' << fun.getParameter("f1.Amplitude") << ' ' << fun.getParameter("f1.FWHM") << std::endl;
    std::cerr << fun.getParameter("f2.PeakCentre") << ' ' << fun.getParameter("f2.Amplitude") << ' ' << fun.getParameter("f2.FWHM") << std::endl;
  }

  void xtest_evaluate() {
    auto fun = makeFunction();
    fun->setParameter("f0.FWHM", 2.0);
    fun->setParameter("f1.FWHM", 2.0);
    fun->setParameter("f2.FWHM", 2.0);
    FunctionDomain1DVector x(0.0, 50.0, 100);
    FunctionValues y(x);
    fun->function(x, y);
    for(size_t i = 0; i < x.size(); ++i) {
      std::cerr << x[i] << ' ' << y[i] << std::endl;
    }
  }

  void test_factory() {
    std::string funDef =
        "name=CrystalFieldSpectrum,Ion=Ce,Symmetry=C2v,"
        "Temperature=44,ToleranceIntensity=0.002, B20=0.37,B22=3.9,"
        "B40=-0.03,B42=-0.1,B44=-0.12, "
        "f0.FWHM=2.2,f1.FWHM=1.8,f2.FWHM=2.1, "
        "ties=(B60=0,B62=0,B64=0,B66=0,BmolX=0,BmolY=0,BmolZ=0,"
        "BextX=0,BextY=0,BextZ=0)";
    auto fun = FunctionFactory::Instance().createInitialized(funDef);
    TS_ASSERT(fun);
    std::cerr << fun->asString() << std::endl;
  }

private:

  IFunction_sptr makeFunction() {
    auto fun = boost::shared_ptr<CrystalFieldSpectrum>(new CrystalFieldSpectrum);
    fun->setParameter("B20", 0.37737);
    fun->setParameter("B22", 3.9770);
    fun->setParameter("B40", -0.031787);
    fun->setParameter("B42", -0.11611);
    fun->setParameter("B44", -0.12544);
    fun->setAttributeValue("Ion", "Ce");
    fun->setAttributeValue("Temperature", 44.0);
    fun->setAttributeValue("ToleranceIntensity", 0.001);
    fun->buildSpectrumFunction();
    return fun;
  }
};

#endif /*CRYSTALFIELDSPECTRUMTEST_H_*/

