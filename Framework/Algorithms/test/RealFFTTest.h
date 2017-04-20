#ifndef REALFFT_TEST_H_
#define REALFFT_TEST_H_

#include <cmath>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/FFT.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"

using namespace Mantid;
using namespace Mantid::API;

// Anonymous namespace to share methods with Performance test
namespace {
void setupWorkspaces(int N, double dX) {
  FrameworkManager::Instance();
  Mantid::DataObjects::Workspace2D_sptr ws =
      boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
          WorkspaceFactory::Instance().create("Workspace2D", 1, N, N));

  Mantid::DataObjects::Workspace2D_sptr ws1 =
      boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
          WorkspaceFactory::Instance().create("Workspace2D", 1, N + 1, N));

  auto &X = ws->mutableX(0);
  auto &Y = ws->mutableY(0);
  auto &E = ws->mutableE(0);

  int n2 = N / 2;
  for (int k = 0; k <= n2; k++) {
    int i = n2 - k;
    if (i >= 0) {
      X[i] = -dX * (k);
      Y[i] = exp(-X[i] * X[i] * 3.);
      E[i] = 1.;
    }
    i = n2 + k;
    if (i < N) {
      X[i] = dX * (k);
      Y[i] = exp(-X[i] * X[i] * 3.);
      E[i] = 1.;
    }
  }

  auto &X1 = ws1->mutableX(0);

  std::copy(X.cbegin(), X.cend(), X1.begin());
  X1.back() = X.back() + dX;
  ws1->mutableY(0) = Y;
  ws1->mutableE(0) = E;

  AnalysisDataService::Instance().add("RealFFT_WS", ws);
  AnalysisDataService::Instance().add("RealFFT_WS_hist", ws1);
}

void deleteWorkspacesFromADS() {
  FrameworkManager::Instance().deleteWorkspace("RealFFT_WS");
  FrameworkManager::Instance().deleteWorkspace("RealFFT_WS_hist");
  FrameworkManager::Instance().deleteWorkspace("RealFFT_WS_forward");
  FrameworkManager::Instance().deleteWorkspace("RealFFT_WS_backward");
}

void doTestForward(const int N, const double XX, bool performance = false) {
  IAlgorithm *fft =
      Mantid::API::FrameworkManager::Instance().createAlgorithm("RealFFT");
  fft->initialize();
  fft->setPropertyValue("InputWorkspace", "RealFFT_WS");
  fft->setPropertyValue("OutputWorkspace", "RealFFT_WS_forward");
  fft->setPropertyValue("WorkspaceIndex", "0");
  fft->execute();

  if (!performance) {
    MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("RealFFT_WS_forward"));

    const auto &X = fWS->x(0);
    const auto &Yr = fWS->y(0);
    const auto &Yi = fWS->y(1);

    double h = sqrt(M_PI / 3);
    double a = M_PI * M_PI / 3;
    double dx = 1 / (XX);

    for (int i = 0; i < N / 4; i++) {
      double x = X[i];
      double tmp = sqrt(Yr[i] * Yr[i] + Yi[i] * Yi[i]);
      // std::cerr<<x<<' '<<tmp<<' '<<h*exp(-a*x*x)<<'\n';
      TS_ASSERT_DELTA(x, dx * i, 0.00001);
      TS_ASSERT_DELTA(tmp / (h * exp(-a * x * x)), 1., 0.001);
      TS_ASSERT_DELTA(Yi[i], 0.0, 0.00001);
    }
  }
}

void doTestBackward(const int N, const double dX, bool performance = false) {
  IAlgorithm *fft =
      Mantid::API::FrameworkManager::Instance().createAlgorithm("RealFFT");
  fft->initialize();
  fft->setPropertyValue("InputWorkspace", "RealFFT_WS_forward");
  fft->setPropertyValue("OutputWorkspace", "RealFFT_WS_backward");
  fft->setPropertyValue("Transform", "Backward");
  fft->execute();

  if (!performance) {
    MatrixWorkspace_sptr WS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("RealFFT_WS"));

    MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("RealFFT_WS_backward"));

    const auto &Y0 = WS->y(0);

    const auto &X = fWS->x(0);
    const auto &Y = fWS->y(0);

    for (int i = 0; i < N; i++) {
      TS_ASSERT_DELTA(X[i], dX * i, 0.00001);
      TS_ASSERT_DELTA(Y[i], Y0[i], 0.00001);
    }
  }
}

void doTestForwardHistogram(const int N, const double XX,
                            bool performance = false) {
  IAlgorithm *fft =
      Mantid::API::FrameworkManager::Instance().createAlgorithm("RealFFT");
  fft->initialize();
  fft->setPropertyValue("InputWorkspace", "RealFFT_WS_hist");
  fft->setPropertyValue("OutputWorkspace", "RealFFT_WS_forward_hist");
  fft->setPropertyValue("WorkspaceIndex", "0");
  fft->execute();

  MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve("RealFFT_WS_forward_hist"));

  if (!performance) {
    const auto &X = fWS->x(0);
    const auto &Yr = fWS->y(0);
    const auto &Yi = fWS->y(1);

    double h = sqrt(M_PI / 3);
    double a = M_PI * M_PI / 3;
    double dx = 1 / (XX);

    for (int i = 0; i < N / 4; i++) {
      double x = X[i];
      double tmp = sqrt(Yr[i] * Yr[i] + Yi[i] * Yi[i]);
      // std::cerr<<x<<' '<<tmp<<' '<<h*exp(-a*x*x)<<'\n';
      TS_ASSERT_DELTA(x, dx * i, 0.00001);
      TS_ASSERT_DELTA(tmp / (h * exp(-a * x * x)), 1., 0.001);
      TS_ASSERT_DELTA(Yi[i], 0.0, 0.00001);
    }
  }
}

void doTestBackwardHistogram(const int N, const double dX,
                             bool performance = false) {
  IAlgorithm *fft =
      Mantid::API::FrameworkManager::Instance().createAlgorithm("RealFFT");
  fft->initialize();
  fft->setPropertyValue("InputWorkspace", "RealFFT_WS_forward_hist");
  fft->setPropertyValue("OutputWorkspace", "RealFFT_WS_backward_hist");
  fft->setPropertyValue("Transform", "Backward");
  fft->execute();

  MatrixWorkspace_sptr WS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve("RealFFT_WS"));

  MatrixWorkspace_sptr fWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve("RealFFT_WS_backward_hist"));

  if (!performance) {
    const auto &Y0 = WS->y(0);
    const auto &X = fWS->x(0);
    const auto &Y = fWS->y(0);

    for (int i = 0; i < N; i++) {
      TS_ASSERT_DELTA(X[i], dX * i, 0.00001);
      TS_ASSERT_DELTA(Y[i], Y0[i], 0.00001);
    }
  }
}
}

class RealFFTTest : public CxxTest::TestSuite {
public:
  static RealFFTTest *createSuite() { return new RealFFTTest(); }
  static void destroySuite(RealFFTTest *suite) { delete suite; }

  RealFFTTest() : N(116), dX(0.3), XX(N * dX) { setupWorkspaces(N, dX); }
  ~RealFFTTest() override { deleteWorkspacesFromADS(); }

  void testForward() { doTestForward(N, XX); }
  void testBackward() { doTestBackward(N, dX); }
  void testForwardHistogram() { doTestForwardHistogram(N, XX); }
  void testBackwardHistogram() { doTestBackwardHistogram(N, dX); }

private:
  const int N;
  const double dX, XX;
};

class RealFFTTestPerformance : public CxxTest::TestSuite {
public:
  static RealFFTTestPerformance *createSuite() {
    return new RealFFTTestPerformance();
  }
  static void destroySuite(RealFFTTestPerformance *suite) { delete suite; }

  RealFFTTestPerformance() : N(116), dX(0.3), XX(N * dX) {
    setupWorkspaces(N, dX);
  }
  ~RealFFTTestPerformance() override { deleteWorkspacesFromADS(); }

  void testForward() { doTestForward(N, XX, performance); }
  void testBackward() { doTestBackward(N, dX, performance); }
  void testForwardHistogram() { doTestForwardHistogram(N, XX, performance); }
  void testBackwardHistogram() { doTestBackwardHistogram(N, dX, performance); }

private:
  const bool performance = true;
  const int N;
  const double dX, XX;
};

#endif /*REALFFT_TEST_H_*/
