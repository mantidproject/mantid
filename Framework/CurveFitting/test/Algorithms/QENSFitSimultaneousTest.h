// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_QENSFITSIMULTANEOUSTEST_H_
#define MANTID_ALGORITHMS_QENSFITSIMULTANEOUSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidDataHandling/Load.h"

#include "MantidCurveFitting/Algorithms/QENSFitSimultaneous.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::CurveFitting::Algorithms::QENSFitSimultaneous;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Counts;
using Mantid::Kernel::make_cow;

class QENSFitSimultaneousTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QENSFitSimultaneousTest *createSuite() {
    return new QENSFitSimultaneousTest();
  }
  static void destroySuite(QENSFitSimultaneousTest *suite) { delete suite; }

  QENSFitSimultaneousTest() { FrameworkManager::Instance(); }

  void test_set_valid_fit_function() {
    QENSFitSimultaneous alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "Function", "name=DeltaFunction,Height=1,Centre=0;name="
                    "Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0;"));
  }

  void test_empty_function_is_not_allowed() {
    QENSFitSimultaneous alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Function", ""),
                     const std::invalid_argument &);
  }

  void test_single_dataset_fit() {

    const int totalBins = 6;
    const int totalHist = 5;
    auto inputWorkspace = createReducedWorkspace(totalBins, totalHist);
    auto resolution =
        createResolutionWorkspace(totalBins, totalHist, "__QENS_Resolution");

    auto outputBaseName = runConvolutionFit(inputWorkspace, resolution);
    testFitOutput(outputBaseName, 1);
    AnalysisDataService::Instance().clear();
  }

  void test_multiple_dataset_fit() {
    const int totalBins = 15;
    const int totalHist = 10;

    std::vector<std::string> names = {"first_red", "second_red"};
    auto function =
        FunctionFactory::Instance().createInitialized(peakFunction());
    auto outputBaseName = runMultiDatasetFit(
        createReducedWorkspaces(names, totalBins, totalHist), function);
    testFitOutput(outputBaseName, names.size());
    AnalysisDataService::Instance().clear();
  }

private:
  std::string runConvolutionFit(MatrixWorkspace_sptr inputWorkspace,
                                MatrixWorkspace_sptr resolution) {
    QENSFitSimultaneous alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setProperty("Function", convolutionFunction(resolution->getName()));
    alg.setProperty("InputWorkspace", inputWorkspace);
    alg.setProperty("StartX", 0.0);
    alg.setProperty("EndX", 3.0);
    alg.setProperty("WorkspaceIndex", 0);
    alg.setProperty("ConvolveMembers", true);
    alg.setProperty("Minimizer", "Levenberg-Marquardt");
    alg.setProperty("MaxIterations", 500);
    alg.setProperty("OutputWorkspace",
                    "ReductionWs_conv_1LFixF_s0_to_5_Result");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    return "ReductionWs_conv_1LFixF_s0_to_5";
  }

  std::string runMultiDatasetFit(
      const std::vector<Mantid::API::MatrixWorkspace_sptr> &workspaces,
      IFunction_sptr function) {
    QENSFitSimultaneous alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setProperty("Function",
                    createMultiDomainFunction(function, workspaces.size()));
    setMultipleInput(alg, workspaces, 0.0, 10.0);
    alg.setProperty("ConvolveMembers", true);
    alg.setProperty("Minimizer", "Levenberg-Marquardt");
    alg.setProperty("MaxIterations", 500);
    alg.setProperty("OutputWorkspace", "MultiQENSFitSequential_Result");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    return "MultiQENSFitSequential";
  }

  void testFitOutput(const std::string &outputBaseName,
                     std::size_t expectedGroupSize) {
    WorkspaceGroup_sptr groupWorkspace;

    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            outputBaseName + "_Parameters"));
    TS_ASSERT_THROWS_NOTHING(
        groupWorkspace =
            AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
                outputBaseName + "_Workspaces"));
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            outputBaseName + "_Result"));

    TS_ASSERT_EQUALS(groupWorkspace->size(), expectedGroupSize);
  }

  std::vector<Mantid::API::MatrixWorkspace_sptr>
  createReducedWorkspaces(const std::vector<std::string> &names, int totalBins,
                          int totalHist) {
    std::vector<Mantid::API::MatrixWorkspace_sptr> workspaces;

    for (const auto &name : names) {
      workspaces.emplace_back(createReducedWorkspace(totalBins, totalHist));
      AnalysisDataService::Instance().addOrReplace(name, workspaces.back());
    }
    return workspaces;
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
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        xlen, xlen - 1, false, false, true, "testInst");
    ws->initialize(ylen, xlen, xlen - 1);
    addBinsAndCountsToWorkspace(ws, xlen, xlen - 1, 1.0, 3.0);

    ws->getAxis(0)->setUnit("DeltaE");

    for (int i = 0; i < xlen; i++)
      ws->setEFixed((i + 1), 0.50);

    auto &run = ws->mutableRun();
    auto timeSeries =
        new Mantid::Kernel::TimeSeriesProperty<std::string>("TestTimeSeries");
    timeSeries->addValue("2010-09-14T04:20:12", "0.02");
    run.addProperty(timeSeries);
    return ws;
  }

  MatrixWorkspace_sptr
  createResolutionWorkspace(std::size_t totalBins, std::size_t totalHist,
                            const std::string &name) const {
    auto resolution =
        createWorkspace<Workspace2D>(totalHist + 1, totalBins + 1, totalBins);
    addBinsAndCountsToWorkspace(resolution, totalBins + 1, totalBins, 0.0, 3.0);
    AnalysisDataService::Instance().addOrReplace(name, resolution);
    return resolution;
  }

  void addBinsAndCountsToWorkspace(Workspace2D_sptr workspace,
                                   std::size_t totalBinEdges,
                                   std::size_t totalCounts, double binValue,
                                   double countValue) const {
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

  void setMultipleInput(IAlgorithm &fitAlgorithm,
                        const std::vector<MatrixWorkspace_sptr> &workspaces,
                        double startX, double endX) {
    fitAlgorithm.setProperty("InputWorkspace", workspaces[0]);
    fitAlgorithm.setProperty("WorkspaceIndex", 0);
    fitAlgorithm.setProperty("StartX", startX);
    fitAlgorithm.setProperty("EndX", endX);

    for (auto i = 1u; i < workspaces.size(); ++i) {
      const auto suffix = "_" + std::to_string(i);
      fitAlgorithm.setProperty("InputWorkspace" + suffix, workspaces[i]);
      fitAlgorithm.setProperty("WorkspaceIndex" + suffix, 0);
      fitAlgorithm.setProperty("StartX" + suffix, startX);
      fitAlgorithm.setProperty("EndX" + suffix, endX);
    }
  }

  IFunction_sptr createMultiDomainFunction(IFunction_sptr function,
                                           std::size_t numberOfDomains) {
    auto multiDomainFunction = boost::make_shared<MultiDomainFunction>();

    for (auto i = 0u; i < numberOfDomains; ++i) {
      multiDomainFunction->addFunction(function);
      multiDomainFunction->setDomainIndex(i, i);
    }
    return multiDomainFunction;
  }
};

#endif /* MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIALTEST_H_ */
