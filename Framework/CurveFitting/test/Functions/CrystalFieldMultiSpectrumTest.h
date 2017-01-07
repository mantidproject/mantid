#ifndef CRYSTALFIELDMULTISPECTRUMTEST_H_
#define CRYSTALFIELDMULTISPECTRUMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/JointDomain.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/Functions/CrystalFieldMultiSpectrum.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::CurveFitting::Functions;

class CrystalFieldMultiSpectrumTest : public CxxTest::TestSuite {
public:
  // Conversion factor from barn to milibarn/steradian
  const double c_mbsr = 79.5774715459;

  void test_function() {
    CrystalFieldMultiSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    std::vector<double> temps(1, 44);
    fun.setAttributeValue("Temperatures", temps);
    fun.setAttributeValue("ToleranceIntensity", 0.001);
    std::vector<double> fwhs(1, 1.5);
    fun.setAttributeValue("FWHMs", fwhs);
    fun.buildTargetFunction();
    auto attNames = fun.getAttributeNames();
    auto parNames = fun.getParameterNames();
    TS_ASSERT_EQUALS(fun.nAttributes(), attNames.size());
    TS_ASSERT_EQUALS(fun.nParams(), parNames.size());

    auto i = fun.parameterIndex("f0.f1.Amplitude");
    TS_ASSERT(fun.isFixed(i));
    TS_ASSERT(!fun.isActive(i));
    i = fun.parameterIndex("f0.f1.PeakCentre");
    TS_ASSERT(fun.isFixed(i));
    TS_ASSERT(!fun.isActive(i));
    i = fun.parameterIndex("f0.f1.FWHM");
    TS_ASSERT(!fun.isFixed(i));
    TS_ASSERT(fun.isActive(i));
    i = fun.parameterIndex("f0.f2.Amplitude");
    TS_ASSERT(fun.isFixed(i));
    TS_ASSERT(!fun.isActive(i));
    i = fun.parameterIndex("f0.f2.PeakCentre");
    TS_ASSERT(fun.isFixed(i));
    TS_ASSERT(!fun.isActive(i));
    i = fun.parameterIndex("f0.f2.FWHM");
    TS_ASSERT(!fun.isFixed(i));
    TS_ASSERT(fun.isActive(i));
    i = fun.parameterIndex("f0.f3.Amplitude");
    TS_ASSERT(fun.isFixed(i));
    TS_ASSERT(!fun.isActive(i));
    i = fun.parameterIndex("f0.f3.PeakCentre");
    TS_ASSERT(fun.isFixed(i));
    TS_ASSERT(!fun.isActive(i));
    i = fun.parameterIndex("f0.f3.FWHM");
    TS_ASSERT(!fun.isFixed(i));
    TS_ASSERT(fun.isActive(i));

    TS_ASSERT_DELTA(fun.getParameter("f0.f0.A0"), 0.0, 1e-3);

    TS_ASSERT_DELTA(fun.getParameter("f0.f1.PeakCentre"), 0.0, 1e-3);
    TS_ASSERT_DELTA(fun.getParameter("f0.f1.Amplitude"), 2.749 * c_mbsr,
                    1e-3 * c_mbsr);
    TS_ASSERT_DELTA(fun.getParameter("f0.f1.FWHM"), 1.5, 1e-3);

    TS_ASSERT_DELTA(fun.getParameter("f0.f2.PeakCentre"), 29.3261, 1e-3);
    TS_ASSERT_DELTA(fun.getParameter("f0.f2.Amplitude"), 0.7204 * c_mbsr,
                    1e-3 * c_mbsr);
    TS_ASSERT_DELTA(fun.getParameter("f0.f2.FWHM"), 1.5, 1e-3);

    TS_ASSERT_DELTA(fun.getParameter("f0.f3.PeakCentre"), 44.3412, 1e-3);
    TS_ASSERT_DELTA(fun.getParameter("f0.f3.Amplitude"), 0.4298 * c_mbsr,
                    1e-3 * c_mbsr);
    TS_ASSERT_DELTA(fun.getParameter("f0.f3.FWHM"), 1.5, 1e-3);
  }

  void test_evaluate() {
    auto funStr = "name=CrystalFieldMultiSpectrum,Ion=Ce,Temperatures=(44, "
                  "50),ToleranceIntensity=0.001,B20=0.37737,B22=3.9770,"
                  "B40=-0.031787,B42=-0.11611,B44=-0.12544,"
                  "f0.f1.FWHM=1.6,f0.f2.FWHM=2.0,f0.f3.FWHM=2.3,f1.f1.FWHM=1.6,"
                  "f1.f2.FWHM=2.0,f1.f3.FWHM=2.3";
    auto ws = createWorkspace();
    auto alg = AlgorithmFactory::Instance().create("EvaluateFunction", -1);
    alg->initialize();
    alg->setPropertyValue("Function", funStr);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("InputWorkspace_1", ws);
    alg->setProperty("OutputWorkspace", "out");
    alg->execute();

    auto out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "Workspace_0");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 3);
    TS_ASSERT_DELTA(out->readY(1)[0], 1.094 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[1], 0.738 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[2], 0.373 * c_mbsr, 0.001 * c_mbsr);
    out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "Workspace_1");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 3);
    TS_ASSERT_DELTA(out->readY(1)[0], 1.094 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[1], 0.738 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[2], 0.373 * c_mbsr, 0.001 * c_mbsr);
    AnalysisDataService::Instance().clear();
  }

  void test_evaluate_scaling() {
    auto funStr = "name=CrystalFieldMultiSpectrum,Ion=Ce,Temperatures=(44, "
                  "50),ToleranceIntensity=0.001,B20=0.37737,B22=3.9770,"
                  "B40=-0.031787,B42=-0.11611,B44=-0.12544,"
                  "IntensityScaling0=2.0,IntensityScaling1=3.3,"
                  "f0.f1.FWHM=1.6,f0.f2.FWHM=2.0,f0.f3.FWHM=2.3,f1.f1.FWHM=1.6,"
                  "f1.f2.FWHM=2.0,f1.f3.FWHM=2.3";
    auto ws = createWorkspace();
    auto alg = AlgorithmFactory::Instance().create("EvaluateFunction", -1);
    alg->initialize();
    alg->setPropertyValue("Function", funStr);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("InputWorkspace_1", ws);
    alg->setProperty("OutputWorkspace", "out");
    alg->execute();

    auto out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "Workspace_0");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 3);
    TS_ASSERT_DELTA(out->readY(1)[0], 1.094 * 2.0 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[1], 0.738 * 2.0 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[2], 59.5010, 0.001);
    out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "Workspace_1");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 3);
    TS_ASSERT_DELTA(out->readY(1)[0], 1.094 * 3.3 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[1], 0.738 * 3.3 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[2], 98.1627, 0.001);
    AnalysisDataService::Instance().clear();
  }

  void test_simple_background() {
    auto funStr = "name=CrystalFieldMultiSpectrum,Ion=Ce,Temperatures=(44, "
                  "50),ToleranceIntensity=0.001,"
                  "Background=FlatBackground,"
                  "B20=0.37737,B22=3.9770,"
                  "B40=-0.031787,B42=-0.11611,B44=-0.12544,"
                  "f0.f0.A0=1.0,f1.f0.A0=2.0,"
                  "f0.f1.FWHM=1.6,f0.f2.FWHM=2.0,f0.f3.FWHM=2.3,f1.f1.FWHM=1.6,"
                  "f1.f2.FWHM=2.0,f1.f3.FWHM=2.3";
    auto fun = FunctionFactory::Instance().createInitialized(funStr);
    TS_ASSERT_EQUALS(fun->getParameter("f0.f0.A0"), 1.0);
    TS_ASSERT_EQUALS(fun->getParameter("f1.f0.A0"), 2.0);
  }

  void test_composite_background() {
    auto funStr = "name=CrystalFieldMultiSpectrum,Ion=Ce,Temperatures=(44, "
                  "50),ToleranceIntensity=0.001,"
                  "Background=\"name=Gaussian;name=FlatBackground\","
                  "B20=0.37737,B22=3.9770,"
                  "B40=-0.031787,B42=-0.11611,B44=-0.12544,"
                  "f0.f0.f0.Sigma=0.1,f1.f0.f0.Sigma=0.2,"
                  "f0.f0.f1.A0=1.0,f1.f0.f1.A0=2.0,"
                  "f0.f1.FWHM=1.6,f0.f2.FWHM=2.0,f0.f3.FWHM=2.3,f1.f1.FWHM=1.6,"
                  "f1.f2.FWHM=2.0,f1.f3.FWHM=2.3";
    auto fun = FunctionFactory::Instance().createInitialized(funStr);
    TS_ASSERT_EQUALS(fun->getParameter("f0.f0.f0.Sigma"), 0.1);
    TS_ASSERT_EQUALS(fun->getParameter("f1.f0.f0.Sigma"), 0.2);
  }

  void test_composite_multispectral() {
    std::string fun1 =
        "name=CrystalFieldMultiSpectrum,Ion=Ce,Temperatures=(44, "
        "50),ToleranceIntensity=0.001,B20=0.37737,B22=3.9770,"
        "B40=-0.031787,B42=-0.11611,B44=-0.12544,"
        "f0.f1.FWHM=1.6,f0.f2.FWHM=2.0,f0.f3.FWHM=2.3,f1.f1.FWHM=1.6,"
        "f1.f2.FWHM=2.0,f1.f3.FWHM=2.3";
    std::string fun2 =
        "name=CrystalFieldMultiSpectrum,Ion=Pr,Temperatures=(44, "
        "50),ToleranceIntensity=0.001,B20=0.37737,B22=3.9770,"
        "B40=-0.031787,B42=-0.11611,B44=-0.12544,"
        "f0.f1.FWHM=1.6,f0.f2.FWHM=2.0,f0.f3.FWHM=2.3,f1.f1.FWHM=1.6,"
        "f1.f2.FWHM=2.0,f1.f3.FWHM=2.3";
    auto fun = fun1 + ";" + fun2;

    auto ws = createWorkspace();
    auto alg = AlgorithmFactory::Instance().create("EvaluateFunction", -1);
    alg->initialize();
    alg->setPropertyValue("Function", fun);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("InputWorkspace_1", ws);
    alg->setProperty("OutputWorkspace", "out");
    alg->execute();
    TS_ASSERT(alg->isExecuted());

    auto out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "Workspace_0");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 3);
    TS_ASSERT_DELTA(out->readY(1)[0], 2.9202 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[1], 2.4691 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[2], 1.3817 * c_mbsr, 0.001 * c_mbsr);
    out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "Workspace_1");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 3);
    TS_ASSERT_DELTA(out->readY(1)[0], 2.9192 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[1], 2.4647 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[2], 1.3791 * c_mbsr, 0.001 * c_mbsr);
    AnalysisDataService::Instance().clear();
  }

  void test_calculate_widths() {
    CrystalFieldMultiSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperatures", std::vector<double>{44.0, 50.0});

    std::vector<double> x[2] = {{0.0, 50.0}, {0.0, 50.0}};
    std::vector<double> y[2] = {{1.0, 2.0}, {3.0, 4.0}};
    auto checkW = [&x, &y](size_t i, double c) {
      return y[i].front() +
             (y[i].back() - y[i].front()) / (x[i].back() - x[i].front()) *
                 (c - x[i].front());
    };
    fun.setAttributeValue("FWHMX0", x[0]);
    fun.setAttributeValue("FWHMY0", y[0]);
    fun.setAttributeValue("FWHMX1", x[1]);
    fun.setAttributeValue("FWHMY1", y[1]);
    fun.buildTargetFunction();
    {
      std::string prefix("f0.f1.");
      auto c = fun.getParameter(prefix + "PeakCentre");
      auto w = fun.getParameter(prefix + "FWHM");
      TS_ASSERT_EQUALS(w, checkW(0, c));
      auto ct = getBounds(fun, prefix + "FWHM");
      TS_ASSERT_DELTA(ct.first, 0.9, 1e-4);
      TS_ASSERT_DELTA(ct.second, 1.1, 1e-4);
    }
    {
      std::string prefix("f0.f2.");
      auto c = fun.getParameter(prefix + "PeakCentre");
      auto w = fun.getParameter(prefix + "FWHM");
      TS_ASSERT_EQUALS(w, checkW(0, c));
      auto ct = getBounds(fun, prefix + "FWHM");
      TS_ASSERT_DELTA(ct.first, 1.4865, 1e-4);
      TS_ASSERT_DELTA(ct.second, 1.6865, 1e-4);
    }
    {
      std::string prefix("f0.f3.");
      auto c = fun.getParameter(prefix + "PeakCentre");
      auto w = fun.getParameter(prefix + "FWHM");
      TS_ASSERT_EQUALS(w, checkW(0, c));
      auto ct = getBounds(fun, prefix + "FWHM");
      TS_ASSERT_DELTA(ct.first, 1.7868, 1e-4);
      TS_ASSERT_DELTA(ct.second, 1.9868, 1e-4);
    }
    {
      std::string prefix("f0.f4.");
      auto w = fun.getParameter(prefix + "FWHM");
      TS_ASSERT_EQUALS(w, 0.0);
    }
    {
      std::string prefix("f1.f1.");
      auto c = fun.getParameter(prefix + "PeakCentre");
      auto w = fun.getParameter(prefix + "FWHM");
      TS_ASSERT_EQUALS(w, checkW(1, c));
      auto ct = getBounds(fun, prefix + "FWHM");
      TS_ASSERT_DELTA(ct.first, 2.9, 1e-4);
      TS_ASSERT_DELTA(ct.second, 3.1, 1e-4);
    }
    {
      std::string prefix("f1.f2.");
      auto c = fun.getParameter(prefix + "PeakCentre");
      auto w = fun.getParameter(prefix + "FWHM");
      TS_ASSERT_EQUALS(w, checkW(1, c));
      auto ct = getBounds(fun, prefix + "FWHM");
      TS_ASSERT_DELTA(ct.first, 3.4865, 1e-4);
      TS_ASSERT_DELTA(ct.second, 3.6865, 1e-4);
    }
    {
      std::string prefix("f1.f3.");
      auto c = fun.getParameter(prefix + "PeakCentre");
      auto w = fun.getParameter(prefix + "FWHM");
      TS_ASSERT_EQUALS(w, checkW(1, c));
      auto ct = getBounds(fun, prefix + "FWHM");
      TS_ASSERT_DELTA(ct.first, 3.7868, 1e-4);
      TS_ASSERT_DELTA(ct.second, 3.9868, 1e-4);
    }
    {
      std::string prefix("f1.f4.");
      auto w = fun.getParameter(prefix + "FWHM");
      TS_ASSERT_EQUALS(w, 0.0);
    }
  }

  void test_underdefinded() {
    CrystalFieldMultiSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    TS_ASSERT_THROWS_NOTHING(fun.buildTargetFunction());
  }

  void test_monte_carlo() {
    CrystalFieldMultiSpectrum fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Temperatures", std::vector<double>{44.0, 50.0});
    fun.setAttributeValue("FWHMs", std::vector<double>{1.0, 1.5});
    auto ws = createWorkspace(fun, 0, 50, 100);

    auto mc = AlgorithmFactory::Instance().create("EstimateFitParameters", -1);
    mc->initialize();
    mc->setRethrows(true);
    mc->setPropertyValue(
        "Function",
        "name=CrystalFieldMultiSpectrum,Ion=Ce,"
        "Symmetry=C2v,Temperatures=(44.0, 50.0),FWHMs=(1.0, 1.0),NPeaks=3,"
        "constraints=(0<B20<2,1<B22<4,-0.1<B40<0.1,-0.1<B42<0.1,-0.1<B44<0.1)");
    mc->setProperty("InputWorkspace", ws);
    mc->setProperty("WorkspaceIndex", 0);
    mc->setProperty("InputWorkspace_1", ws);
    mc->setProperty("WorkspaceIndex_1", 1);
    mc->setProperty("NSamples", 1000);
    mc->setProperty("Constraints", "0<f0.f2.PeakCentre<50,0<f0.f3.PeakCentre<"
                                   "50,0<f1.f2.PeakCentre<50,0<f1.f3."
                                   "PeakCentre<50");
    mc->execute();
    IFunction_sptr func = mc->getProperty("Function");
    auto fit = AlgorithmFactory::Instance().create("Fit", -1);
    fit->initialize();
    fit->setProperty("Function", func);
    fit->setProperty("InputWorkspace", ws);
    fit->setProperty("WorkspaceIndex", 0);
    fit->setProperty("InputWorkspace_1", ws);
    fit->setProperty("WorkspaceIndex_1", 1);
    fit->execute();
    double chi2 = fit->getProperty("OutputChi2overDoF");
    TS_ASSERT_LESS_THAN(chi2, 100.0);
  }

  void test_ties_in_composite_function() {
    std::string funDef =
        "name=CrystalFieldMultiSpectrum,Ion=Ce,Symmetry=C2v,Temperatures=(44.0,"
        "50),FWHMs=(1.1,0.9),B44=-0.115325956893,B40=0.0844136192563,B42=-0."
        "459507287606,B22=4.36779676967;name=CrystalFieldMultiSpectrum,Ion=Pr,"
        "Symmetry=C2v,Temperatures=(44.0,50),FWHMs=(1.1,0.9),B44=-0."
        "115325956893,B40=0.0844136192563,B42=-0.459507287606,B22=4."
        "36779676967;ties=(f1.IntensityScaling0=2.0*f0.IntensityScaling0,f1."
        "IntensityScaling1=2.0*f0.IntensityScaling1,f0.f0.f1.FWHM=f1.f0.f1."
        "FWHM/2)";
    auto fun = FunctionFactory::Instance().createInitialized(funDef);
    {
      auto index = fun->parameterIndex("f1.IntensityScaling0");
      auto tie = fun->getTie(index);
      TS_ASSERT(tie);
      if (!tie) {
        return;
      }
      TS_ASSERT_EQUALS(tie->asString(),
                       "f1.IntensityScaling0=2.0*f0.IntensityScaling0");
    }
    {
      auto index = fun->parameterIndex("f1.IntensityScaling1");
      auto tie = fun->getTie(index);
      TS_ASSERT(tie);
      if (!tie) {
        return;
      }
      TS_ASSERT_EQUALS(tie->asString(),
                       "f1.IntensityScaling1=2.0*f0.IntensityScaling1");
    }
    {
      auto index = fun->parameterIndex("f0.f0.f1.FWHM");
      auto tie = fun->getTie(index);
      TS_ASSERT(tie);
      if (!tie) {
        return;
      }
      TS_ASSERT_EQUALS(tie->asString(), "f0.f0.f1.FWHM=f1.f0.f1.FWHM/2");
    }
  }

private:
  Workspace_sptr createWorkspace() {
    auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 100, 100);
    double dx = 55.0 / 99;
    for (size_t i = 0; i < 100; ++i) {
      ws->dataX(0)[i] = dx * static_cast<double>(i);
    }
    return ws;
  }

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
    auto nSpec = fun.getNumberDomains();
    auto ws =
        WorkspaceFactory::Instance().create("Workspace2D", nSpec, nbins, nbins);
    JointDomain domain;
    for (size_t i = 0; i < nSpec; ++i) {
      auto x = FunctionDomain_sptr(new FunctionDomain1DVector(x0, x1, nbins));
      domain.addDomain(x);
    }
    FunctionValues y(domain);
    fun.function(domain, y);
    for (size_t i = 0; i < nSpec; ++i) {
      auto x = static_cast<const FunctionDomain1DVector &>(domain.getDomain(i));
      ws->dataX(i) = x.toVector();
      auto n = x.size();
      auto from = y.getPointerToCalculated(i * n);
      ws->dataY(i).assign(from, from + n);
    }
    return ws;
  }
};

#endif /*CRYSTALFIELDMULTISPECTRUMTEST_H_*/
