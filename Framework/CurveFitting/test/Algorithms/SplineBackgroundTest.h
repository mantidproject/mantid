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
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidCurveFitting/Algorithms/SplineBackground.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include <cxxtest/TestSuite.h>

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;

class SplineBackgroundTest : public CxxTest::TestSuite {
private:
  struct SinFunction {
    double operator()(double x, int) { return std::sin(x); }
  };

public:
  void testIt() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(SinFunction(), 1, 0.1, 10.1, 0.1, true);
    WorkspaceCreationHelper::addNoise(ws, 0.1);
    // Mask some bins out to test that functionality
    const size_t nbins = 101;
    int toMask = static_cast<int>(0.75 * nbins);
    ws->maskBin(0, toMask - 1);
    ws->maskBin(0, toMask);
    ws->maskBin(0, toMask + 1);
    ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
    const std::string wsName = "SplineBackground_points";
    WorkspaceCreationHelper::storeWS(wsName, ws);

    auto alg = Mantid::API::AlgorithmManager::Instance().create("SplineBackground");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", wsName);
    alg->setPropertyValue("OutputWorkspace", "SplineBackground_out");
    alg->setPropertyValue("WorkspaceIndex", "0");
    alg->execute();

    MatrixWorkspace_sptr outWS = WorkspaceCreationHelper::getWS<MatrixWorkspace>("SplineBackground_out");

    const auto &X = outWS->x(0);
    const auto &Y = outWS->y(0);

    for (size_t i = 0; i < Y.size(); i++) {
      TS_ASSERT_DELTA(Y[i], std::sin(X[i]), 0.2);
    }
    TS_ASSERT(outWS->getAxis(0)->unit() == ws->getAxis(0)->unit());
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);
  }

  void testFittingMultipleSpectra() {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(SinFunction(), 3, 0.1, 10.1, 0.1, true);
    WorkspaceCreationHelper::addNoise(ws, 0.1);
    // Mask some bins out to test that functionality
    const size_t nbins = 101;
    int toMask = static_cast<int>(0.75 * nbins);
    ws->maskBin(0, toMask - 1);
    ws->maskBin(0, toMask);
    ws->maskBin(0, toMask + 1);
    ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
    const std::string wsName = "SplineBackground_points";
    WorkspaceCreationHelper::storeWS(wsName, ws);

    auto alg = Mantid::API::AlgorithmManager::Instance().create("SplineBackground");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", wsName);
    alg->setPropertyValue("OutputWorkspace", "SplineBackground_out");
    alg->setPropertyValue("WorkspaceIndex", "0");
    alg->setPropertyValue("EndWorkspaceIndex", "1");
    alg->execute();

    MatrixWorkspace_sptr outWS = WorkspaceCreationHelper::getWS<MatrixWorkspace>("SplineBackground_out");

    auto nHist = outWS->getNumberHistograms();
    TS_ASSERT_EQUALS(nHist, 2);
    const auto &X = outWS->x(0);
    const auto &Y = outWS->y(0);

    for (size_t i = 0; i < Y.size(); i++) {
      TS_ASSERT_DELTA(Y[i], std::sin(X[i]), 0.2);
    }
    TS_ASSERT(outWS->getAxis(0)->unit() == ws->getAxis(0)->unit());
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(wsName);
  }
};

class SplineBackgroundTestPerformance : public CxxTest::TestSuite {

public:
  void setUp() override {
    constexpr size_t nspec = 1;
    constexpr double xRangeStart = 0.1;
    constexpr double xRangeEnd = 2500.1;
    constexpr double xRangeStep = 0.1;

    ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(SinFunction(), nspec, xRangeStart, xRangeEnd,
                                                                xRangeStep, true);
    WorkspaceCreationHelper::addNoise(ws, 0.1);
    // Mask some bins out to test that functionality
    const size_t nbins = 101;
    int toMask(static_cast<int>(0.75 * nbins));

    ws->maskBin(0, toMask - 1);
    ws->maskBin(0, toMask);
    ws->maskBin(0, toMask + 1);

    ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");

    WorkspaceCreationHelper::storeWS(inputWsName, ws);

    SplineBackgroundAlg = Mantid::API::AlgorithmManager::Instance().create("SplineBackground");
    SplineBackgroundAlg->initialize();
    SplineBackgroundAlg->setPropertyValue("InputWorkspace", inputWsName);
    SplineBackgroundAlg->setPropertyValue("OutputWorkspace", outputWsName);
    SplineBackgroundAlg->setPropertyValue("WorkspaceIndex", "0");

    SplineBackgroundAlg->setRethrows(true);
  }

  void testSplineBackgroundPerformance() { TS_ASSERT_THROWS_NOTHING(SplineBackgroundAlg->execute()); }

  void tearDown() override {
    WorkspaceCreationHelper::removeWS(inputWsName);
    WorkspaceCreationHelper::removeWS(outputWsName);
  }

private:
  IAlgorithm_sptr SplineBackgroundAlg;

  Mantid::DataObjects::Workspace2D_sptr ws;
  const std::string inputWsName = "SplineBackground_points";
  const std::string outputWsName = "SplineBackground_out";

  struct SinFunction {
    double operator()(double x, int) { return std::sin(x); }
  };
};
