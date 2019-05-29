// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIALTEST_H_
#define MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidDataHandling/Load.h"

#include "MantidCurveFitting/Algorithms/ConvolutionFit.h"
#include "MantidCurveFitting/Algorithms/QENSFitSequential.h"

#include "MantidDataObjects/Workspace2D.h"

#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::CurveFitting;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Counts;
using Mantid::Kernel::make_cow;

using ConvolutionFitSequential =
    Algorithms::ConvolutionFit<Algorithms::QENSFitSequential>;

class ConvolutionFitSequentialTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvolutionFitSequentialTest *createSuite() {
    return new ConvolutionFitSequentialTest();
  }
  static void destroySuite(ConvolutionFitSequentialTest *suite) {
    delete suite;
  }

  ConvolutionFitSequentialTest() { FrameworkManager::Instance(); }

  void test_fit_function_is_valid_for_convolution_fitting() {
    ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    createConvFitResWorkspace(1, 1);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "Function", "name=Convolution;name=Resolution,Workspace=__ConvFit_"
                    "Resolution,WorkspaceIndex=0;"));
    AnalysisDataService::Instance().clear();
  }

  //-------------------------- Failure cases ----------------------------
  void test_empty_function_is_not_allowed() {
    ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Function", ""),
                     const std::invalid_argument &);
  }

  void test_empty_startX_is_not_allowed() {
    ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("StartX", ""),
                     const std::invalid_argument &);
  }

  void test_empty_endX_is_not_allowed() {
    ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("EndX", ""),
                     const std::invalid_argument &);
  }

  void test_empty_specMin_is_not_allowed() {
    ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("SpecMin", ""),
                     const std::invalid_argument &);
  }

  void test_empty_specMax_is_not_allowed() {
    ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("SpecMax", ""),
                     const std::invalid_argument &);
  }

  void test_empty_maxIterations_is_not_allowed() {
    ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("MaxIterations", ""),
                     const std::invalid_argument &);
  }

  void test_spectra_min_or_max_number_can_not_be_negative() {
    ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS(alg.setPropertyValue("SpecMin", "-1"),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS(alg.setPropertyValue("SpecMax", "-1"),
                     const std::invalid_argument &);
  }

  void test_max_iterations_can_not_be_a_negative_number() {
    ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS(alg.setPropertyValue("MaxIterations", "-1"),
                     const std::invalid_argument &);
  }

  void test_fit_function_that_does_not_contain_resolution_is_not_allowed() {
    ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS(
        alg.setProperty("Function", "function=test,name=Convolution"),
        const std::invalid_argument &);
  }

  void test_fit_function_that_does_not_contain_convolution_is_not_allowed() {
    ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS(
        alg.setProperty("Function", "function=test,name=Resolution"),
        const std::invalid_argument &);
  }

  //------------------------- Execution cases ---------------------------
  void test_exec_with_red_file() {
    const int totalBins = 6;
    auto resWs = create2DWorkspace(5, 1);
    auto redWs = create2DWorkspace(totalBins, 5);
    createConvFitResWorkspace(5, totalBins);
    AnalysisDataService::Instance().add("ResolutionWs_", resWs);
    AnalysisDataService::Instance().add("ReductionWs_", redWs);
    ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("InputWorkspace", redWs);
    alg.setProperty("Function",
                    "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
                    "(composite=Convolution,FixResolution=true,NumDeriv=true;"
                    "name=Resolution,Workspace=__ConvFit_Resolution,"
                    "WorkspaceIndex=0;((composite=ProductFunction,NumDeriv="
                    "false;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0."
                    "0175)))");
    alg.setProperty("StartX", 0.0);
    alg.setProperty("EndX", 3.0);
    alg.setProperty("SpecMin", 0);
    alg.setProperty("SpecMax", 5);
    alg.setProperty("ConvolveMembers", true);
    alg.setProperty("Minimizer", "Levenberg-Marquardt");
    alg.setProperty("MaxIterations", 500);
    alg.setProperty("OutputWorkspace",
                    "ReductionWs_conv_1LFixF_s0_to_5_Result");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Retrieve and analyse parameter table - Param table does not require
    // further testing as this is tested in the ProcessIndirectFitParameters
    // Algorithm
    ITableWorkspace_sptr paramTable;
    TS_ASSERT_THROWS_NOTHING(paramTable = getWorkspaceFromADS<ITableWorkspace>(
                                 "ReductionWs_conv_1LFixF_s0_to_5_Parameters"));

    // Retrieve and analyse results table
    WorkspaceGroup_sptr resultGroup;
    TS_ASSERT_THROWS_NOTHING(resultGroup = getWorkspaceFromADS<WorkspaceGroup>(
                                 "ReductionWs_conv_1LFixF_s0_to_5_Result"));
    MatrixWorkspace_sptr resultWs;
    TS_ASSERT_THROWS_NOTHING(resultWs = getMatrixWorkspace(resultGroup, 0));
    TS_ASSERT_EQUALS(resultWs->blocksize(), totalBins);

    // Retrieve and analyse group table
    WorkspaceGroup_sptr groupWs;
    TS_ASSERT_THROWS_NOTHING(groupWs = getWorkspaceFromADS<WorkspaceGroup>(
                                 "ReductionWs_conv_1LFixF_s0_to_5_Workspaces"));

    // Check number of expected Histograms and Histogram deminsions
    int entities = groupWs->getNumberOfEntries();
    TS_ASSERT_EQUALS(entities, redWs->getNumberHistograms());
    auto groupMember =
        groupWs->getItem("ReductionWs_conv_1LFixF_s0_to_5_0_Workspace");
    auto matrixMember =
        boost::dynamic_pointer_cast<MatrixWorkspace>(groupMember);

    TS_ASSERT_EQUALS(matrixMember->blocksize(), resWs->blocksize());

    // Check oringal Log was copied correctly
    auto &memberRun = matrixMember->mutableRun();
    auto &originalRun = redWs->mutableRun();

    TS_ASSERT_EQUALS(memberRun.getLogData().at(1)->value(),
                     originalRun.getLogData().at(1)->value());

    // Check new Log data is present
    auto memberLogs = memberRun.getLogData();

    TS_ASSERT_EQUALS(memberRun.getLogData("background")->value(),
                     "Fixed Linear");
    TS_ASSERT_EQUALS(memberRun.getLogData("convolve_members")->value(), "true");
    TS_ASSERT_EQUALS(memberRun.getLogData("delta_function")->value(), "false");
    TS_ASSERT_EQUALS(memberRun.getLogData("fit_program")->value(),
                     "ConvolutionFit");
    TS_ASSERT_EQUALS(memberRun.getLogData("sample_filename")->value(),
                     "ReductionWs_");
    TS_ASSERT_EQUALS(memberRun.getLogData("lorentzians")->value(), "1");

    AnalysisDataService::Instance().clear();
  }

  void test_exec_with_sqw_file() {
    auto sqwWs = createGenericWorkspace("SqwWs_", true);
    auto resWs = createGenericWorkspace("ResolutionWs_", false);
    auto convFitRes = createGenericWorkspace("__ConvFit_Resolution", false);
    ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("InputWorkspace", sqwWs);
    alg.setProperty("Function",
                    "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
                    "(composite=Convolution,FixResolution=true,NumDeriv=true;"
                    "name=Resolution,Workspace=__ConvFit_Resolution,"
                    "WorkspaceIndex=0;((composite=ProductFunction,NumDeriv="
                    "false;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0."
                    "0175)))");
    alg.setProperty("StartX", 0.0);
    alg.setProperty("EndX", 5.0);
    alg.setProperty("SpecMin", 0);
    alg.setProperty("SpecMax", 0);
    alg.setProperty("ConvolveMembers", true);
    alg.setProperty("Minimizer", "Levenberg-Marquardt");
    alg.setProperty("MaxIterations", 500);
    alg.setProperty("OutputWorkspace", "SqwWs_conv_1LFixF_s0_Result");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Assert that output is in ADS
    TS_ASSERT_THROWS_NOTHING(getWorkspaceFromADS<ITableWorkspace>(
        "SqwWs_conv_1LFixF_s0_Parameters"));

    TS_ASSERT_THROWS_NOTHING(
        getWorkspaceFromADS<WorkspaceGroup>("SqwWs_conv_1LFixF_s0_Result"));

    TS_ASSERT_THROWS_NOTHING(
        getWorkspaceFromADS<WorkspaceGroup>("SqwWs_conv_1LFixF_s0_Workspaces"));

    AnalysisDataService::Instance().clear();
  }

  void test_exec_with_extract_members() {
    std::string runName = "irs26173";
    std::string runSample = "graphite002";
    std::string fileName = runName + "_" + runSample;

    auto resWs = loadWorkspace(fileName + "_res.nxs");
    auto redWs = loadWorkspace(fileName + "_red.nxs");
    createConvFitResWorkspace(redWs->getNumberHistograms(), redWs->blocksize());
    AnalysisDataService::Instance().add("ResolutionWs_", resWs);
    AnalysisDataService::Instance().add(fileName, redWs);

    size_t specMin = 0;
    size_t specMax = 5;

    auto outputName = runName + "_conv_1LFixF_s" + std::to_string(specMin) +
                      "_to_" + std::to_string(specMax) + "_Result";

    ConvolutionFitSequential alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("InputWorkspace", redWs);
    alg.setProperty("Function",
                    "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
                    "(composite=Convolution,FixResolution=true,NumDeriv=true;"
                    "name=Resolution,Workspace=__ConvFit_Resolution,"
                    "WorkspaceIndex=0;((composite=ProductFunction,NumDeriv="
                    "false;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0."
                    "0175)))");
    alg.setProperty("StartX", 0.0);
    alg.setProperty("EndX", 3.0);
    alg.setProperty("SpecMin", boost::numeric_cast<int>(specMin));
    alg.setProperty("SpecMax", boost::numeric_cast<int>(specMax));
    alg.setProperty("ConvolveMembers", true);
    alg.setProperty("ExtractMembers", true);
    alg.setProperty("Minimizer", "Levenberg-Marquardt");
    alg.setProperty("MaxIterations", 500);
    alg.setProperty("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check members group workspace was created
    WorkspaceGroup_const_sptr membersGroupWs;
    TS_ASSERT_THROWS_NOTHING(
        membersGroupWs = getWorkspaceFromADS<WorkspaceGroup>(
            runName + "_conv_1LFixF_s" + std::to_string(specMin) + "_to_" +
            std::to_string(specMax) + "_Members"));

    // Check all members have been extracted into their own workspace and
    // grouped
    // inside the members group workspace.
    std::unordered_set<std::string> members = {
        "Data", "Calc", "Diff", "LinearBackground", "Lorentzian"};
    for (auto i = 0u; i < membersGroupWs->size(); ++i) {
      MatrixWorkspace_const_sptr ws = getMatrixWorkspace(membersGroupWs, i);
      TS_ASSERT(ws->getNumberHistograms() == specMax - specMin + 1);
      std::string name = ws->getName();
      members.erase(name.substr(name.find_last_of('_') + 1));
    }
    TS_ASSERT(members.empty());

    AnalysisDataService::Instance().clear();
  }

  //------------------------ Private Functions---------------------------

  template <typename T = MatrixWorkspace>
  boost::shared_ptr<T> getWorkspaceFromADS(const std::string &name) {
    return AnalysisDataService::Instance().retrieveWS<T>(name);
  }

  MatrixWorkspace_sptr getMatrixWorkspace(WorkspaceGroup_const_sptr group,
                                          std::size_t index) {
    return boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(index));
  }

  MatrixWorkspace_sptr loadWorkspace(const std::string &fileName) {
    Mantid::DataHandling::Load loadAlg;
    loadAlg.setChild(true);
    loadAlg.initialize();
    loadAlg.setProperty("Filename", fileName);
    loadAlg.setProperty("OutputWorkspace", "__temp");
    loadAlg.executeAsChildAlg();
    Workspace_sptr ws = loadAlg.getProperty("OutputWorkspace");
    return boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
  }

  MatrixWorkspace_sptr createGenericWorkspace(const std::string &wsName,
                                              const bool numericAxis) {
    const std::vector<double> xData{1, 2, 3, 4, 5};
    const std::vector<double> yData{0, 1, 3, 1, 0};

    auto createWorkspace =
        AlgorithmManager::Instance().create("CreateWorkspace");
    createWorkspace->initialize();
    if (numericAxis) {
      createWorkspace->setProperty("UnitX", "DeltaE");
      createWorkspace->setProperty("VerticalAxisUnit", "MomentumTransfer");
      createWorkspace->setProperty("VerticalAxisValues", "1");
    } else {
      createWorkspace->setProperty("UnitX", "DeltaE");
      createWorkspace->setProperty("VerticalAxisUnit", "SpectraNumber");
    }
    createWorkspace->setProperty("DataX", xData);
    createWorkspace->setProperty("DataY", yData);
    createWorkspace->setProperty("NSpec", 1);
    createWorkspace->setPropertyValue("OutputWorkspace", wsName);
    createWorkspace->execute();
    return getWorkspaceFromADS(wsName);
  }

  MatrixWorkspace_sptr create2DWorkspace(int xlen, int ylen) {
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        xlen, xlen - 1, false, false, true, "testInst");
    ws->initialize(ylen, xlen, xlen - 1);

    BinEdges x1(xlen, 0.0);
    Counts y1(xlen - 1, 3.0);
    CountStandardDeviations e1(xlen - 1, sqrt(3.0));

    int j = 0;
    std::generate(begin(x1), end(x1), [&j] { return 0.5 + 0.75 * j++; });

    for (int i = 0; i < ylen; i++) {
      ws->setBinEdges(i, x1);
      ws->setCounts(i, y1);
      ws->setCountStandardDeviations(i, e1);
    }

    ws->getAxis(0)->setUnit("DeltaE");

    for (int i = 0; i < xlen; i++) {
      ws->setEFixed((i + 1), 0.50);
    }

    auto &run = ws->mutableRun();
    auto timeSeries =
        new Mantid::Kernel::TimeSeriesProperty<std::string>("TestTimeSeries");
    timeSeries->addValue("2010-09-14T04:20:12", "0.02");
    run.addProperty(timeSeries);
    auto test = run.getLogData("TestTimeSeries")->value();
    return ws;
  }

  void createConvFitResWorkspace(size_t totalHist, size_t totalBins) {
    auto convFitRes =
        createWorkspace<Workspace2D>(totalHist + 1, totalBins + 1, totalBins);
    BinEdges x1(totalBins + 1, 0.0);
    Counts y1(totalBins, 3.0);
    CountStandardDeviations e1(totalBins, sqrt(3.0));

    int j = 0;
    std::generate(begin(x1), end(x1), [&j] { return 0.5 + 0.75 * j++; });

    for (size_t i = 0; i < totalHist; i++) {
      convFitRes->setBinEdges(i, x1);
      convFitRes->setCounts(i, y1);
      convFitRes->setCountStandardDeviations(i, e1);
    }

    AnalysisDataService::Instance().add("__ConvFit_Resolution", convFitRes);
  }
};

#endif /* MANTID_ALGORITHMS_CONVOLUTIONFITSEQUENTIALTEST_H_ */
