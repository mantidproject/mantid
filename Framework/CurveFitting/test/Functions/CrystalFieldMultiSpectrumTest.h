// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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

  void test_evaluate_1() {
    auto funStr = "name=CrystalFieldSpectrum,Ion=Ce,Temperature=44,"
                  "ToleranceIntensity=0.001,B20=0.37737,B22=3.9770,"
                  "B40=-0.031787,B42=-0.11611,B44=-0.12544,"
                  "f0.FWHM=1.6,f1.FWHM=2.0,f2.FWHM=2.3";
    auto ws = createWorkspace();
    auto alg = AlgorithmFactory::Instance().create("EvaluateFunction", -1);
    alg->initialize();
    alg->setPropertyValue("Function", funStr);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("OutputWorkspace", "out");
    alg->execute();

    auto out =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 3);
    TS_ASSERT_DELTA(out->readY(1)[0], 1.094 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[1], 0.738 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[2], 0.373 * c_mbsr, 0.001 * c_mbsr);
    AnalysisDataService::Instance().clear();
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
    TS_ASSERT_DELTA(out->readY(1)[0], 1.094 * 2.0 * c_mbsr, 0.002 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[1], 0.738 * 2.0 * c_mbsr, 0.002 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[2], 0.373 * 2.0 * c_mbsr, 0.002 * c_mbsr);
    out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "Workspace_1");
    TS_ASSERT(out);
    TS_ASSERT_EQUALS(out->getNumberHistograms(), 3);
    TS_ASSERT_DELTA(out->readY(1)[0], 1.094 * 3.3 * c_mbsr, 0.003 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[1], 0.738 * 3.3 * c_mbsr, 0.003 * c_mbsr);
    TS_ASSERT_DELTA(out->readY(1)[2], 0.373 * 3.3 * c_mbsr, 0.003 * c_mbsr);
    AnalysisDataService::Instance().clear();
  }

  void test_evaluate_physprops() {
    auto funStr = "name=CrystalFieldMultiSpectrum,Ion=Ce,Temperatures=(44,"
                  "50,1,10,1),ToleranceIntensity=0.001,B20=0.37737,B22=3.9770,"
                  "B40=-0.031787,B42=-0.11611,B44=-0.12544,"
                  "PhysicalProperties=(0,1,2,3,4)," // INS, Cp, chi, M(H), M(T)
                  "Hdir3=(1,1,1), Hmag4=1, Unit4=cgs, Lambda2=0.0000001,"
                  "f0.f1.FWHM=1.6,f0.f2.FWHM=2.0,f0.f3.FWHM=2.3";
    auto ws = createWorkspace();
    auto alg = AlgorithmFactory::Instance().create("EvaluateFunction", -1);
    alg->initialize();
    alg->setPropertyValue("Function", funStr);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("InputWorkspace_1", ws);
    alg->setProperty("InputWorkspace_2", ws);
    alg->setProperty("InputWorkspace_3", ws);
    alg->setProperty("InputWorkspace_4", ws);
    alg->setProperty("OutputWorkspace", "out");
    alg->execute();

    // Test the INS spectrum
    auto out0 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "Workspace_0");
    TS_ASSERT(out0);
    TS_ASSERT_EQUALS(out0->getNumberHistograms(), 3);
    TS_ASSERT_DELTA(out0->readY(1)[0], 1.094 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out0->readY(1)[1], 0.738 * c_mbsr, 0.001 * c_mbsr);
    TS_ASSERT_DELTA(out0->readY(1)[2], 0.373 * c_mbsr, 0.001 * c_mbsr);
    // Test the heat capacity calculation
    auto out1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "Workspace_1");
    TS_ASSERT(out1);
    TS_ASSERT_EQUALS(out1->getNumberHistograms(), 3);
    TS_ASSERT_DELTA(out1->readY(1)[50], 0.006, 0.001);
    TS_ASSERT_DELTA(out1->readY(1)[60], 0.032, 0.001);
    TS_ASSERT_DELTA(out1->readY(1)[70], 0.103, 0.001);
    // Test the susceptibility calculation
    auto out2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "Workspace_2");
    TS_ASSERT(out2);
    TS_ASSERT_EQUALS(out2->getNumberHistograms(), 3);
    // Susceptibility default outputs in cgs units.
    TS_ASSERT_DELTA(out2->readY(1)[50], 0.00236231, 0.0000001);
    TS_ASSERT_DELTA(out2->readY(1)[60], 0.00233006, 0.0000001);
    TS_ASSERT_DELTA(out2->readY(1)[70], 0.00230932, 0.0000001);
    // Test the magnetisation calculation
    auto out3 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "Workspace_3");
    TS_ASSERT(out3);
    TS_ASSERT_EQUALS(out3->getNumberHistograms(), 3);
    TS_ASSERT_DELTA(out3->readY(1)[1], 0.05754, 0.0001);
    TS_ASSERT_DELTA(out3->readY(1)[5], 0.28307, 0.0001);
    TS_ASSERT_DELTA(out3->readY(1)[10], 0.53932, 0.0001);
    // Test the moment vs temperature calculation
    auto out4 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "Workspace_4");
    TS_ASSERT(out4);
    TS_ASSERT_EQUALS(out4->getNumberHistograms(), 3);
    // SI and cgs susceptibility differ by factor of 10.
    // Dataset 2 in cgs, dataset 4 in SI.
    TS_ASSERT_DELTA(out4->readY(1)[51], out2->readY(1)[51], 1e-4);
    TS_ASSERT_DELTA(out4->readY(1)[61], out2->readY(1)[61], 1e-4);
    TS_ASSERT_DELTA(out4->readY(1)[71], out2->readY(1)[71], 1e-4);
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
      return y[i].front() + (y[i].back() - y[i].front()) /
                                (x[i].back() - x[i].front()) *
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

  void test_multispectrum_malformed_resmod() {
    std::string fun =
        "name=CrystalFieldMultiSpectrum,Ion=Ce,FWHMX0=(1,2),FWHMY0=(2,3),"
        "FWHMX1=(0,1),FWHMY2=(5,6),Temperatures=(44,50),B20=0.37737";
    auto alg = AlgorithmFactory::Instance().create("EvaluateFunction", -1);
    alg->initialize();
    TS_ASSERT_THROWS(alg->setPropertyValue("Function", fun),
                     const std::invalid_argument &);
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
        "name=CrystalFieldMultiSpectrum,Ion=Ce,FixAllPeaks=1,"
        "Symmetry=C2v,Temperatures=(44.0, 50.0),FWHMs=(1.0, 1.0),NPeaks=3,"
        "constraints=(0<B20<0.5,3<B22<4,-0.1<B40<0.0,-0.1<B42<0.0,-0.1<B44<0."
        "0)");
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
      ws->mutableX(i) = x.toVector();
      auto n = x.size();
      auto from = y.getPointerToCalculated(i * n);
      ws->mutableY(i).assign(from, from + n);
    }
    return ws;
  }
};

#endif /*CRYSTALFIELDMULTISPECTRUMTEST_H_*/
