// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/UnitFactory.h"
#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/PDFFourierTransform2.h"
#include <algorithm>
#include <cmath>
#include <numeric>

using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid;

namespace {
/**
 * Create Workspace from 0 to N*dx
 */
Mantid::API::MatrixWorkspace_sptr createWS(size_t n, double dx, const std::string &name, const std::string &unitlabel,
                                           const bool withBadValues = false) {

  Mantid::DataObjects::Workspace2D_sptr ws = std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
      Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", 1, n, n));

  auto &X = ws->mutableX(0);
  auto &Y = ws->mutableY(0);
  auto &E = ws->mutableE(0);

  for (size_t i = 0; i < n; i++) {
    X[i] = double(i) * dx;
    Y[i] = X[i] + 1.0;
    E[i] = sqrt(fabs(X[i]));
  }

  if (withBadValues) {
    Y[0] = std::numeric_limits<double>::quiet_NaN();
    Y[Y.size() - 1] = std::numeric_limits<double>::quiet_NaN();
  }

  ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create(unitlabel);

  Mantid::API::AnalysisDataService::Instance().add(name, ws);

  return ws;
}
} // namespace

class PDFFourierTransform2Test : public CxxTest::TestSuite {
public:
  void test_Init() {
    PDFFourierTransform2 alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Execute() {

    API::Workspace_sptr ws = createWS(20, 0.1, "TestInput1", "MomentumTransfer");

    PDFFourierTransform2 pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("Direction", "Forward");
    pdfft.setProperty("OutputWorkspace", "PDFGofR");
    pdfft.setProperty("SofQType", "S(Q)");
    pdfft.setProperty("Rmax", 20.0);
    pdfft.setProperty("DeltaR", 0.01);
    pdfft.setProperty("Qmin", 0.0);
    pdfft.setProperty("Qmax", 30.0);
    pdfft.setProperty("PDFType", "G(r)");

    pdfft.execute();

    TS_ASSERT(pdfft.isExecuted());
  }

  void test_CheckResult() {

    API::Workspace_sptr ws = createWS(20, 0.1, "CheckResult", "MomentumTransfer");

    // 1. Run PDFFT
    PDFFourierTransform2 pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("Direction", "Forward");
    pdfft.setProperty("OutputWorkspace", "PDFGofR");
    pdfft.setProperty("SofQType", "S(Q)");
    pdfft.setProperty("Rmax", 20.0);
    pdfft.setProperty("DeltaR", 0.01);
    pdfft.setProperty("Qmin", 0.0);
    pdfft.setProperty("Qmax", 30.0);
    pdfft.setProperty("PDFType", "G(r)");

    pdfft.execute();

    DataObjects::Workspace2D_sptr pdfws =
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(API::AnalysisDataService::Instance().retrieve("PDFGofR"));
    const auto &R = pdfws->x(0);
    const auto &GofR = pdfws->y(0);
    const auto &pdfUnit = pdfws->getAxis(0)->unit();

    TS_ASSERT_DELTA(R[0], 0.01, 0.0001);
    TS_ASSERT_DELTA(R[249], 2.5, 0.0001);
    TS_ASSERT_DELTA(GofR[0], 0.0200, 0.0001);
    TS_ASSERT_DELTA(GofR[249], -0.4928, 0.0001);
    TS_ASSERT_EQUALS(pdfUnit->caption(), "Atomic Distance");
  }

  void test_CheckNan() {

    API::Workspace_sptr ws = createWS(20, 0.1, "CheckNan", "MomentumTransfer", true);

    // 1. Run PDFFT
    PDFFourierTransform2 pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("Direction", "Forward");
    pdfft.setProperty("OutputWorkspace", "PDFGofR");
    pdfft.setProperty("SofQType", "S(Q)");
    pdfft.setProperty("Rmax", 20.0);
    pdfft.setProperty("DeltaR", 0.01);
    pdfft.setProperty("Qmin", 0.0);
    pdfft.setProperty("Qmax", 30.0);
    pdfft.setProperty("PDFType", "G(r)");

    pdfft.execute();

    DataObjects::Workspace2D_sptr pdfws =
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(API::AnalysisDataService::Instance().retrieve("PDFGofR"));
    const auto &R = pdfws->x(0);
    const auto &GofR = pdfws->y(0);

    TS_ASSERT_DELTA(R[0], 0.01, 0.0001);
    TS_ASSERT_DELTA(R[249], 2.5, 0.0001);
    // make sure that nan didn' slip in
    TS_ASSERT(std::find_if(GofR.begin(), GofR.end(), [](const double d) { return std::isnan(d); }) == GofR.end());
  }

  void test_filter() {
    API::MatrixWorkspace_sptr ws = createWS(200, 0.1, "filter", "MomentumTransfer");
    auto &SofQ = ws->mutableY(0);
    for (size_t i = 0; i < SofQ.size(); i++) {
      SofQ[i] = 1.0;
    }

    // 1. Run PDFFT
    PDFFourierTransform2 pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("Direction", "Forward");
    pdfft.setProperty("OutputWorkspace", "PDFGofR");
    pdfft.setProperty("SofQType", "S(Q)-1");
    pdfft.setProperty("Qmax", 20.0);
    pdfft.setProperty("PDFType", "G(r)");
    pdfft.setProperty("Filter", true);

    pdfft.execute();

    DataObjects::Workspace2D_sptr pdfws =
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(API::AnalysisDataService::Instance().retrieve("PDFGofR"));
    const auto GofR = pdfws->y(0);

    TS_ASSERT(GofR[0] > 40.0);
    for (size_t i = 1; i < GofR.size(); i++) {
      TS_ASSERT_LESS_THAN(fabs(GofR[i]), 0.2);
    }

    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void test_reverse() {
    API::Workspace_sptr ws = createWS(20, 0.1, "CheckResult", "AtomicDistance");

    // 1. Run PDFFT
    PDFFourierTransform2 pdfft;
    pdfft.initialize();
    pdfft.setProperty("InputWorkspace", ws);
    pdfft.setProperty("Direction", "Backward");
    pdfft.setProperty("OutputWorkspace", "SofQ");
    pdfft.setProperty("SofQType", "S(Q)");
    pdfft.setProperty("Qmax", 20.0);
    pdfft.setProperty("DeltaQ", 0.01);
    pdfft.setProperty("Rmin", 0.0);
    pdfft.setProperty("Rmax", 30.0);
    pdfft.setProperty("PDFType", "G(r)");

    pdfft.execute();

    DataObjects::Workspace2D_sptr sofqws =
        std::dynamic_pointer_cast<DataObjects::Workspace2D>(API::AnalysisDataService::Instance().retrieve("SofQ"));
    const auto &Q = sofqws->x(0);
    const auto &SofQ = sofqws->y(0);
    const auto &SofQUnit = sofqws->getAxis(0)->unit();

    TS_ASSERT_DELTA(Q[0], 0.01, 0.0001);
    TS_ASSERT_DELTA(Q[249], 2.5, 0.0001);
    TS_ASSERT_DELTA(SofQ[0], 5.1875, 0.0001);
    TS_ASSERT_DELTA(SofQ[249], 1.1068, 0.0001);
    TS_ASSERT_EQUALS(SofQUnit->caption(), "q");
  }
};

class PDFFourierTransform2TestPerformance : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PDFFourierTransform2TestPerformance *createSuite() { return new PDFFourierTransform2TestPerformance(); }

  static void destroySuite(PDFFourierTransform2TestPerformance *suite) { delete suite; }

  void setUp() override {
    ws = createWS(2000000, 0.1, "inputWS", "MomentumTransfer");
    pdfft = std::make_shared<PDFFourierTransform2>();
    pdfft->initialize();
    pdfft->setProperty("InputWorkspace", ws);
    pdfft->setProperty("OutputWorkspace", "outputWS");
    pdfft->setProperty("SofQType", "S(Q)");
    pdfft->setProperty("Rmax", 20.0);
    pdfft->setProperty("DeltaR", 0.01);
    pdfft->setProperty("Qmin", 0.0);
    pdfft->setProperty("Qmax", 30.0);
    pdfft->setProperty("PDFType", "G(r)");
  }

  void tearDown() override { Mantid::API::AnalysisDataService::Instance().remove("outputWS"); }

  void testPerformanceWS() { pdfft->execute(); }

private:
  Mantid::API::MatrixWorkspace_sptr ws;
  Mantid::API::IAlgorithm_sptr pdfft;
};
