// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FFT_DERIVATIVE_TEST_H_
#define FFT_DERIVATIVE_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/FFTDerivative.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid;
using namespace Mantid::API;

class FFTDerivativeTest : public CxxTest::TestSuite {
public:
  void testGaussianDerivative() {
    const int N = 100;

    createWS(N, 0, "exp");

    IAlgorithm *fft = Mantid::API::FrameworkManager::Instance().createAlgorithm(
        "FFTDerivative");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace", "FFTDerivative_WS_exp");
    fft->setPropertyValue("OutputWorkspace", "FFTDerivative_out");
    fft->execute();

    MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("FFTDerivative_out"));

    auto &X = fWS->x(0);
    auto &Y = fWS->y(0);

    TS_ASSERT_EQUALS(Y.size(), 100);

    for (size_t i = 0; i < Y.size(); ++i) {
      double xx = X[i] - 5.1;
      TS_ASSERT_DELTA(Y[i], (-4 * xx * exp(-(xx * xx) * 2)), 0.000001);
    }

    FrameworkManager::Instance().deleteWorkspace("FFTDerivative_WS_exp");
    FrameworkManager::Instance().deleteWorkspace("FFTDerivative_out");
  }

  void testGaussianSecondOrderDerivative() {
    const int N = 100;

    createWS(N, 0, "exp");

    IAlgorithm *fft = Mantid::API::FrameworkManager::Instance().createAlgorithm(
        "FFTDerivative");
    fft->initialize();
    fft->setPropertyValue("InputWorkspace", "FFTDerivative_WS_exp");
    fft->setPropertyValue("OutputWorkspace", "FFTDerivative_out");
    fft->setPropertyValue("Order", "2");
    fft->execute();

    MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("FFTDerivative_out"));

    auto &X = fWS->x(0);
    auto &Y = fWS->y(0);

    TS_ASSERT_EQUALS(Y.size(), 100);

    for (size_t i = 0; i < Y.size(); ++i) {
      double xx = X[i] - 5.1;
      double ex = exp(-(xx * xx) * 2);
      TS_ASSERT_DELTA(Y[i], (16 * xx * xx * ex - 4 * ex), 0.000001);
    }

    FrameworkManager::Instance().deleteWorkspace("FFTDerivative_WS_exp");
    FrameworkManager::Instance().deleteWorkspace("FFTDerivative_out");
  }

private:
  MatrixWorkspace_sptr createWS(int n, int dn, const std::string &name) {
    FrameworkManager::Instance();
    Mantid::DataObjects::Workspace2D_sptr ws =
        boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
            WorkspaceFactory::Instance().create("Workspace2D", 1, n + dn, n));

    const double dX = 10.0 / (n - 1);
    const double x0 = 0.;
    const double c = 5.1;
    auto &X = ws->mutableX(0);
    auto &Y = ws->mutableY(0);
    auto &E = ws->mutableE(0);

    for (int i = 0; i < n; i++) {
      double x = x0 + dX * (i);
      X[i] = x;
      Y[i] = exp(-(x - c) * (x - c) * 2);
      E[i] = 1.;
    }

    if (dn > 0)
      X[n] = X[n - 1] + dX;

    AnalysisDataService::Instance().add("FFTDerivative_WS_" + name, ws);

    return ws;
  }
};

#endif /*FFT_DERIVATIVE_TEST_H_*/
