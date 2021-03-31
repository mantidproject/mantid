// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidDataHandling/Load.h"

#include "MantidCurveFitting/Algorithms/QENSFitSequential.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::CurveFitting::Algorithms::QENSFitSequential;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::Kernel::make_cow;

class QENSFitSequentialTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QENSFitSequentialTest *createSuite() { return new QENSFitSequentialTest(); }
  static void destroySuite(QENSFitSequentialTest *suite) { delete suite; }

  QENSFitSequentialTest() { FrameworkManager::Instance(); }

  void test_set_valid_fit_function() {
    QENSFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Function", "name=DeltaFunction,Height=1,Centre=0;name="
                                                         "Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0;"));
  }

  void test_empty_function_is_not_allowed() {
    QENSFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Function", ""), const std::invalid_argument &);
  }

  void test_convolution_fit() {
    const int totalBins = 6;
    const int totalHist = 5;
    auto inputWorkspace = createReducedWorkspace(totalBins, totalHist);
    auto resolution = createResolutionWorkspace(totalBins, totalHist, "__QENS_Resolution");

    std::vector<double> startX;
    std::vector<double> endX;
    for (int i = 0; i < 6; i++) {
      // startX and endX for each spectra being fit.
      startX.push_back(0.0);
      endX.push_back(3.0);
    }
    auto outputBaseName = runConvolutionFit(inputWorkspace, resolution, startX, endX);
    testFitOutput(outputBaseName, inputWorkspace->getNumberHistograms());
    AnalysisDataService::Instance().clear();
  }

  void test_convolution_fit_single_startX() {
    const int totalBins = 6;
    const int totalHist = 5;
    auto inputWorkspace = createReducedWorkspace(totalBins, totalHist);
    auto resolution = createResolutionWorkspace(totalBins, totalHist, "__QENS_Resolution");
    std::vector<double> startX;
    startX.push_back(0.0);
    std::vector<double> endX;
    endX.push_back(3.0);
    auto outputBaseName = runConvolutionFit(inputWorkspace, resolution, startX, endX);
    testFitOutput(outputBaseName, inputWorkspace->getNumberHistograms());
    AnalysisDataService::Instance().clear();
  }

  void test_multiple_fit() {
    const int totalBins = 15;
    const int totalHist = 10;

    std::vector<std::string> names = {"first_red", "second_red"};
    std::vector<double> startX;
    std::vector<double> endX;
    for (int i = 0; i < 6; i++) {
      // startX and endX for each spectra being fit.
      startX.push_back(0.0);
      endX.push_back(10.0);
    }
    auto outputBaseName =
        runMultipleFit(createReducedWorkspaces(names, totalBins, totalHist), peakFunction(), startX, endX);
    testFitOutput(outputBaseName, names.size() * 3);
    AnalysisDataService::Instance().clear();
  }

  void test_multiple_fit_single_startX() {
    const int totalBins = 15;
    const int totalHist = 10;

    std::vector<std::string> names = {"first_red", "second_red"};
    std::vector<double> startX;
    startX.push_back(0.0);
    std::vector<double> endX;
    endX.push_back(10.0);
    auto outputBaseName =
        runMultipleFit(createReducedWorkspaces(names, totalBins, totalHist), peakFunction(), startX, endX);
    testFitOutput(outputBaseName, names.size() * 3);
    AnalysisDataService::Instance().clear();
  }

private:
  std::string runConvolutionFit(const MatrixWorkspace_sptr &inputWorkspace, const MatrixWorkspace_sptr &resolution,
                                const std::vector<double> &startX, const std::vector<double> &endX) {
    QENSFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setProperty("InputWorkspace", inputWorkspace);
    alg.setProperty("Function", convolutionFunction(resolution->getName()));
    alg.setProperty("StartX", startX);
    alg.setProperty("EndX", endX);
    alg.setProperty("SpecMin", 0);
    alg.setProperty("SpecMax", static_cast<int>(inputWorkspace->getNumberHistograms() - 1));
    alg.setProperty("ConvolveMembers", true);
    alg.setProperty("Minimizer", "Levenberg-Marquardt");
    alg.setProperty("MaxIterations", 500);
    alg.setProperty("OutputWorkspace", "ReductionWs_conv_1LFixF_s0_to_5_Result");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    return "ReductionWs_conv_1LFixF_s0_to_5";
  }

  std::string runMultipleFit(const std::vector<Mantid::API::MatrixWorkspace_sptr> &workspaces,
                             const std::string &function, const std::vector<double> startX,
                             const std::vector<double> endX) {
    QENSFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setProperty("Input", createMultipleFitInput(workspaces));
    alg.setProperty("Function", function);
    alg.setProperty("StartX", startX);
    alg.setProperty("EndX", endX);
    alg.setProperty("ConvolveMembers", true);
    alg.setProperty("Minimizer", "Levenberg-Marquardt");
    alg.setProperty("MaxIterations", 500);
    alg.setProperty("OutputWorkspace", "MultiQENSFitSequential_Result");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    return "MultiQENSFitSequential";
  }

  void testFitOutput(const std::string &outputBaseName, std::size_t expectedGroupSize) {
    WorkspaceGroup_sptr groupWorkspace;

    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outputBaseName + "_Parameters"));
    TS_ASSERT_THROWS_NOTHING(
        groupWorkspace = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputBaseName + "_Workspaces"));
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputBaseName + "_Result"));
    TS_ASSERT_EQUALS(groupWorkspace->size(), expectedGroupSize);
  }

  std::vector<Mantid::API::MatrixWorkspace_sptr> createReducedWorkspaces(const std::vector<std::string> &names,
                                                                         int totalBins, int totalHist) {
    std::vector<Mantid::API::MatrixWorkspace_sptr> workspaces;

    for (const auto &name : names) {
      workspaces.emplace_back(createReducedWorkspace(totalBins, totalHist));
      AnalysisDataService::Instance().addOrReplace(name, workspaces.back());
    }
    return workspaces;
  }

  std::string createMultipleFitInput(const std::vector<Mantid::API::MatrixWorkspace_sptr> &workspaces) const {
    std::ostringstream input;

    for (const auto &workspace : workspaces)
      input << workspace->getName() << ",i0;" << workspace->getName() << ",i"
            << std::to_string(workspace->getNumberHistograms() / 2) << ";" << workspace->getName() << ",i"
            << std::to_string(workspace->getNumberHistograms() - 1) << ";";
    return input.str();
  }

  std::string peakFunction() const {
    return "name=LinearBackground,A0=0,A1=0;name=Lorentzian,Amplitude=1,"
           "PeakCentre=0,FWHM=0.0175";
  }

  std::string convolutionFunction(const std::string &resolutionName) const {
    return "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);("
           "composite=Convolution,FixResolution=true,NumDeriv=true;name="
           "Resolution,Workspace=" +
           resolutionName +
           ",WorkspaceIndex=0;((composite=ProductFunction,NumDeriv=false;name="
           "Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0.0175)))";
  }

  MatrixWorkspace_sptr createReducedWorkspace(int xlen, int ylen) const {
    auto ws =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(xlen, xlen - 1, false, false, true, "testInst");
    ws->initialize(ylen, xlen, xlen - 1);
    addBinsAndCountsToWorkspace(ws, xlen, xlen - 1, 1.0, 3.0);

    ws->getAxis(0)->setUnit("DeltaE");

    for (int i = 0; i < xlen; ++i)
      ws->setEFixed((i + 1), 0.50);

    auto &run = ws->mutableRun();
    auto timeSeries = new Mantid::Kernel::TimeSeriesProperty<std::string>("TestTimeSeries");
    timeSeries->addValue("2010-09-14T04:20:12", "0.02");
    run.addProperty(timeSeries);
    return ws;
  }

  MatrixWorkspace_sptr createResolutionWorkspace(std::size_t totalBins, std::size_t totalHist,
                                                 const std::string &name) const {
    auto resolution = createWorkspace<Workspace2D>(totalHist + 1, totalBins + 1, totalBins);
    addBinsAndCountsToWorkspace(resolution, totalBins + 1, totalBins, 0.0, 3.0);
    AnalysisDataService::Instance().addOrReplace(name, resolution);
    return resolution;
  }

  void addBinsAndCountsToWorkspace(const Workspace2D_sptr &workspace, std::size_t totalBinEdges,
                                   std::size_t totalCounts, double binValue, double countValue) const {
    BinEdges x1(totalBinEdges, binValue);
    Counts y1(totalCounts, countValue);
    CountStandardDeviations e1(totalCounts, sqrt(countValue));

    int j = 0;
    std::generate(begin(x1), end(x1), [&j] { return 0.5 + 0.75 * j++; });

    for (auto i = 0u; i < workspace->getNumberHistograms(); ++i) {
      workspace->setBinEdges(i, x1);
      workspace->setCounts(i, y1);
      workspace->setCountStandardDeviations(i, e1);
    }
  }
};
