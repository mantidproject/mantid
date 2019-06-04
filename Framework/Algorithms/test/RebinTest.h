// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef REBINTEST_H_
#define REBINTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/CreateWorkspace.h"
#include "MantidAlgorithms/MaskBins.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTestHelpers/ParallelAlgorithmCreation.h"
#include "MantidTestHelpers/ParallelRunner.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <numeric>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Counts;

namespace {

std::unique_ptr<Rebin> prepare_rebin(const Parallel::Communicator &comm,
                                     const std::string &storageMode) {
  auto create = ParallelTestHelpers::create<Algorithms::CreateWorkspace>(comm);
  std::vector<double> dataEYX(2000);
  for (size_t i = 0; i < dataEYX.size(); ++i)
    dataEYX[i] = static_cast<double>(i % 2);
  int nspec = 1000;
  create->setProperty<int>("NSpec", nspec);
  create->setProperty<std::vector<double>>("DataX", dataEYX);
  create->setProperty<std::vector<double>>("DataY", dataEYX);
  create->setProperty<std::vector<double>>("DataE", dataEYX);
  create->setProperty("ParallelStorageMode", storageMode);
  create->execute();
  MatrixWorkspace_sptr ws = create->getProperty("OutputWorkspace");
  auto rebin = ParallelTestHelpers::create<Algorithms::Rebin>(comm);
  rebin->setProperty("InputWorkspace", ws);
  return rebin;
}

void run_rebin(const Parallel::Communicator &comm,
               const std::string &storageMode) {
  using namespace Parallel;
  auto rebin = prepare_rebin(comm, storageMode);
  rebin->setProperty("Params", "1,1,3");
  TS_ASSERT_THROWS_NOTHING(rebin->execute());
  MatrixWorkspace_const_sptr ws = rebin->getProperty("OutputWorkspace");
  if (comm.rank() == 0 || fromString(storageMode) != StorageMode::MasterOnly) {
    TS_ASSERT_EQUALS(ws->storageMode(), fromString(storageMode));
  } else {
    TS_ASSERT_EQUALS(ws, nullptr);
  }
}

void run_rebin_params_only_bin_width(const Parallel::Communicator &comm,
                                     const std::string &storageMode) {
  using namespace Parallel;
  auto rebin = prepare_rebin(comm, storageMode);
  rebin->setProperty("Params", "0.5");
  TS_ASSERT_THROWS_NOTHING(rebin->execute());
  MatrixWorkspace_const_sptr ws = rebin->getProperty("OutputWorkspace");
  if (comm.rank() == 0 || fromString(storageMode) != StorageMode::MasterOnly) {
    TS_ASSERT_EQUALS(ws->storageMode(), fromString(storageMode));
  } else {
    TS_ASSERT_EQUALS(ws, nullptr);
  }
}
} // namespace

class RebinTest : public CxxTest::TestSuite {
public:
  double BIN_DELTA;
  int NUMPIXELS, NUMBINS;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RebinTest *createSuite() { return new RebinTest(); }
  static void destroySuite(RebinTest *suite) { delete suite; }

  RebinTest() {
    BIN_DELTA = 2.0;
    NUMPIXELS = 20;
    NUMBINS = 50;
  }

  void testworkspace1D_dist() {
    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    test_in1D->setDistribution(true);
    AnalysisDataService::Instance().add("test_in1D", test_in1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_in1D");
    rebin.setPropertyValue("OutputWorkspace", "test_out");
    // Check it fails if "Params" property not set
    TS_ASSERT_THROWS(rebin.execute(), const std::runtime_error &);
    TS_ASSERT(!rebin.isExecuted());
    // Now set the property
    rebin.setPropertyValue("Params", "1.5,2.0,20,-0.1,30,1.0,35");
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    MatrixWorkspace_sptr rebindata =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_out");
    auto &outX = rebindata->x(0);
    auto &outY = rebindata->y(0);
    auto &outE = rebindata->e(0);

    TS_ASSERT_DELTA(outX[7], 15.5, 0.000001);
    TS_ASSERT_DELTA(outY[7], 3.0, 0.000001);
    TS_ASSERT_DELTA(outE[7], sqrt(4.5) / 2.0, 0.000001);

    TS_ASSERT_DELTA(outX[12], 24.2, 0.000001);
    TS_ASSERT_DELTA(outY[12], 3.0, 0.000001);
    TS_ASSERT_DELTA(outE[12], sqrt(5.445) / 2.42, 0.000001);

    TS_ASSERT_DELTA(outX[17], 32.0, 0.000001);
    TS_ASSERT_DELTA(outY[17], 3.0, 0.000001);
    TS_ASSERT_DELTA(outE[17], sqrt(2.25), 0.000001);
    bool dist = rebindata->isDistribution();
    TS_ASSERT(dist);
    AnalysisDataService::Instance().remove("test_in1D");
    AnalysisDataService::Instance().remove("test_out");
  }

  void testworkspace1D_nondist() {
    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    AnalysisDataService::Instance().add("test_in1D", test_in1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_in1D");
    rebin.setPropertyValue("OutputWorkspace", "test_out");
    rebin.setPropertyValue("Params", "1.5,2.0,20,-0.1,30,1.0,35");
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    MatrixWorkspace_sptr rebindata =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_out");

    auto &outX = rebindata->x(0);
    auto &outY = rebindata->y(0);
    auto &outE = rebindata->e(0);

    TS_ASSERT_DELTA(outX[7], 15.5, 0.000001);
    TS_ASSERT_DELTA(outY[7], 8.0, 0.000001);
    TS_ASSERT_DELTA(outE[7], sqrt(8.0), 0.000001);
    TS_ASSERT_DELTA(outX[12], 24.2, 0.000001);
    TS_ASSERT_DELTA(outY[12], 9.68, 0.000001);
    TS_ASSERT_DELTA(outE[12], sqrt(9.68), 0.000001);
    TS_ASSERT_DELTA(outX[17], 32, 0.000001);
    TS_ASSERT_DELTA(outY[17], 4.0, 0.000001);
    TS_ASSERT_DELTA(outE[17], sqrt(4.0), 0.000001);
    bool dist = rebindata->isDistribution();
    TS_ASSERT(!dist);
    AnalysisDataService::Instance().remove("test_in1D");
    AnalysisDataService::Instance().remove("test_out");
  }

  void testworkspace1D_logarithmic_binning() {
    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    test_in1D->setDistribution(true);
    AnalysisDataService::Instance().add("test_in1D", test_in1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_in1D");
    rebin.setPropertyValue("OutputWorkspace", "test_out");
    // Check it fails if "Params" property not set
    TS_ASSERT_THROWS(rebin.execute(), const std::runtime_error &);
    TS_ASSERT(!rebin.isExecuted());
    // Now set the property
    rebin.setPropertyValue("Params", "1.0,-1.0,1000.0");
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    MatrixWorkspace_sptr rebindata =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_out");
    auto &outX = rebindata->x(0);

    TS_ASSERT_EQUALS(outX.size(), 11);
    TS_ASSERT_DELTA(outX[0], 1.0, 1e-5);
    TS_ASSERT_DELTA(outX[1], 2.0, 1e-5);
    TS_ASSERT_DELTA(outX[2], 4.0, 1e-5); // and so on...
    TS_ASSERT_DELTA(outX[10], 1000.0, 1e-5);

    bool dist = rebindata->isDistribution();
    TS_ASSERT(dist);
    AnalysisDataService::Instance().remove("test_in1D");
    AnalysisDataService::Instance().remove("test_out");
  }

  void testworkspace2D_dist() {
    Workspace2D_sptr test_in2D = Create2DWorkspace(50, 20);
    test_in2D->setDistribution(true);
    AnalysisDataService::Instance().add("test_in2D", test_in2D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_in2D");
    rebin.setPropertyValue("OutputWorkspace", "test_out");
    rebin.setPropertyValue("Params", "1.5,2.0,20,-0.1,30,1.0,35");
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    MatrixWorkspace_sptr rebindata =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_out");

    auto &outX = rebindata->x(5);
    auto &outY = rebindata->y(5);
    auto &outE = rebindata->e(5);
    TS_ASSERT_DELTA(outX[7], 15.5, 0.000001);
    TS_ASSERT_DELTA(outY[7], 3.0, 0.000001);
    TS_ASSERT_DELTA(outE[7], sqrt(4.5) / 2.0, 0.000001);

    TS_ASSERT_DELTA(outX[12], 24.2, 0.000001);
    TS_ASSERT_DELTA(outY[12], 3.0, 0.000001);
    TS_ASSERT_DELTA(outE[12], sqrt(5.445) / 2.42, 0.000001);

    TS_ASSERT_DELTA(outX[17], 32.0, 0.000001);
    TS_ASSERT_DELTA(outY[17], 3.0, 0.000001);
    TS_ASSERT_DELTA(outE[17], sqrt(2.25), 0.000001);
    bool dist = rebindata->isDistribution();
    TS_ASSERT(dist);

    // Test the axes are of the correct type
    TS_ASSERT_EQUALS(rebindata->axes(), 2);
    TS_ASSERT(dynamic_cast<RefAxis *>(rebindata->getAxis(0)));
    TS_ASSERT(dynamic_cast<SpectraAxis *>(rebindata->getAxis(1)));

    AnalysisDataService::Instance().remove("test_in2D");
    AnalysisDataService::Instance().remove("test_out");
  }

  void do_test_EventWorkspace(EventType eventType, bool inPlace,
                              bool PreserveEvents, bool expectOutputEvent) {
    // Two events per bin
    EventWorkspace_sptr test_in =
        WorkspaceCreationHelper::createEventWorkspace2(50, 100);
    test_in->switchEventType(eventType);

    std::string inName("test_inEvent");
    std::string outName("test_inEvent_output");
    if (inPlace)
      outName = inName;

    AnalysisDataService::Instance().addOrReplace(inName, test_in);
    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", inName);
    rebin.setPropertyValue("OutputWorkspace", outName);
    rebin.setPropertyValue("Params", "0.0,4.0,100");
    rebin.setProperty("PreserveEvents", PreserveEvents);
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());

    MatrixWorkspace_sptr outWS;
    EventWorkspace_sptr eventOutWS;
    TS_ASSERT_THROWS_NOTHING(
        outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve(outName)));
    TS_ASSERT(outWS);
    if (!outWS)
      return;

    // Is the output gonna be events?
    if (expectOutputEvent) {
      eventOutWS = boost::dynamic_pointer_cast<EventWorkspace>(outWS);
      TS_ASSERT(eventOutWS);
      if (!eventOutWS)
        return;
      TS_ASSERT_EQUALS(eventOutWS->getNumberEvents(), 50 * 100 * 2);
      // Check that it is the same workspace
      if (inPlace)
        TS_ASSERT(eventOutWS == test_in);
    }

    auto &X = outWS->x(0);
    auto &Y = outWS->y(0);
    auto &E = outWS->e(0);

    TS_ASSERT_EQUALS(X.size(), 26);
    TS_ASSERT_DELTA(X[0], 0.0, 1e-5);
    TS_ASSERT_DELTA(X[1], 4.0, 1e-5);
    TS_ASSERT_DELTA(X[2], 8.0, 1e-5);

    TS_ASSERT_EQUALS(Y.size(), 25);
    TS_ASSERT_DELTA(Y[0], 8.0, 1e-5);
    TS_ASSERT_DELTA(Y[1], 8.0, 1e-5);
    TS_ASSERT_DELTA(Y[2], 8.0, 1e-5);

    TS_ASSERT_EQUALS(E.size(), 25);
    TS_ASSERT_DELTA(E[0], sqrt(8.0), 1e-5);
    TS_ASSERT_DELTA(E[1], sqrt(8.0), 1e-5);

    // Test the axes are of the correct type
    TS_ASSERT_EQUALS(outWS->axes(), 2);
    TS_ASSERT(dynamic_cast<RefAxis *>(outWS->getAxis(0)));
    TS_ASSERT(dynamic_cast<SpectraAxis *>(outWS->getAxis(1)));

    AnalysisDataService::Instance().remove(inName);
    AnalysisDataService::Instance().remove(outName);
  }

  void testEventWorkspace_InPlace_PreserveEvents() {
    do_test_EventWorkspace(TOF, true, true, true);
  }

  void testEventWorkspace_InPlace_PreserveEvents_weighted() {
    do_test_EventWorkspace(WEIGHTED, true, true, true);
  }

  void testEventWorkspace_InPlace_PreserveEvents_weightedNoTime() {
    do_test_EventWorkspace(WEIGHTED_NOTIME, true, true, true);
  }

  void testEventWorkspace_InPlace_NoPreserveEvents() {
    do_test_EventWorkspace(TOF, true, false, false);
  }

  void testEventWorkspace_InPlace_NoPreserveEvents_weighted() {
    do_test_EventWorkspace(WEIGHTED, true, false, false);
  }

  void testEventWorkspace_InPlace_NoPreserveEvents_weightedNoTime() {
    do_test_EventWorkspace(WEIGHTED_NOTIME, true, false, false);
  }

  void testEventWorkspace_NotInPlace_NoPreserveEvents() {
    do_test_EventWorkspace(TOF, false, false, false);
  }

  void testEventWorkspace_NotInPlace_NoPreserveEvents_weighted() {
    do_test_EventWorkspace(WEIGHTED, false, false, false);
  }

  void testEventWorkspace_NotInPlace_NoPreserveEvents_weightedNoTime() {
    do_test_EventWorkspace(WEIGHTED_NOTIME, false, false, false);
  }

  void testEventWorkspace_NotInPlace_PreserveEvents() {
    do_test_EventWorkspace(TOF, false, true, true);
  }

  void testEventWorkspace_NotInPlace_PreserveEvents_weighted() {
    do_test_EventWorkspace(WEIGHTED, false, true, true);
  }

  void testEventWorkspace_NotInPlace_PreserveEvents_weightedNoTime() {
    do_test_EventWorkspace(WEIGHTED_NOTIME, false, true, true);
  }

  void testRebinPointData() {
    Workspace2D_sptr input = Create1DWorkspace(51);
    AnalysisDataService::Instance().add("test_RebinPointDataInput", input);

    Mantid::API::Algorithm_sptr ctpd =
        Mantid::API::AlgorithmFactory::Instance().create("ConvertToPointData",
                                                         1);
    ctpd->initialize();
    ctpd->setPropertyValue("InputWorkspace", "test_RebinPointDataInput");
    ctpd->setPropertyValue("OutputWorkspace", "test_RebinPointDataInput");
    ctpd->execute();

    Mantid::API::Algorithm_sptr reb =
        Mantid::API::AlgorithmFactory::Instance().create("Rebin", 1);
    reb->initialize();
    TS_ASSERT_THROWS_NOTHING(
        reb->setPropertyValue("InputWorkspace", "test_RebinPointDataInput"));
    reb->setPropertyValue("OutputWorkspace", "test_RebinPointDataOutput");
    reb->setPropertyValue("Params", "7,0.75,23");
    TS_ASSERT_THROWS_NOTHING(reb->execute());

    TS_ASSERT(reb->isExecuted());

    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("test_RebinPointDataOutput"));

    TS_ASSERT(!outWS->isHistogramData());
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 1);

    TS_ASSERT_EQUALS(outWS->x(0)[0], 7.3750);
    TS_ASSERT_EQUALS(outWS->x(0)[10], 14.8750);
    TS_ASSERT_EQUALS(outWS->x(0)[20], 22.3750);

    AnalysisDataService::Instance().remove("test_RebinPointDataInput");
    AnalysisDataService::Instance().remove("test_RebinPointDataOutput");
  }

  void testMaskedBinsDist() {
    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    AnalysisDataService::Instance().add("test_Rebin_mask_dist", test_in1D);
    test_in1D->setDistribution(true);
    maskFirstBins("test_Rebin_mask_dist", "test_Rebin_masked_ws", 10.0);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_Rebin_masked_ws");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_masked_ws");
    rebin.setPropertyValue("Params", "1.5,3.0,12,-0.1,30");
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    MatrixWorkspace_sptr rebindata =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_Rebin_masked_ws");
    auto &outX = rebindata->x(0);
    auto &outY = rebindata->y(0);

    MatrixWorkspace_sptr input =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_Rebin_mask_dist");
    auto &inX = input->x(0);
    auto &inY = input->y(0);

    const MatrixWorkspace::MaskList &mask = rebindata->maskedBins(0);

    // turn the mask list into an array like the Y values
    MantidVec weights(outY.size(), 0);
    for (auto it : mask) {
      weights[it.first] = it.second;
    }

    // the degree of masking must be the same as the reduction in the y-value,
    // for distributions, this is the easy case
    for (size_t i = 0; i < outY.size(); ++i) {
      size_t inBin =
          std::lower_bound(inX.begin(), inX.end(), outX[i]) - inX.begin();
      if (inBin < inX.size() - 2) {
        TS_ASSERT_DELTA(outY[i] / inY[inBin], 1 - weights[i], 0.000001);
      }
    }
    // the above formula tests the criterian that must be true for masking,
    // however we need some more specific tests incase outY.empty() or something
    TS_ASSERT_DELTA(outY[1], 0.0, 0.000001)
    TS_ASSERT_DELTA(outY[2], 0.25, 0.000001)
    TS_ASSERT_DELTA(outY[3], 3.0, 0.000001)
    TS_ASSERT_DELTA(weights[2], 1 - (0.25 / 3.0), 0.000001)

    TS_ASSERT(rebindata->isDistribution());
    AnalysisDataService::Instance().remove("test_Rebin_mask_dist");
    AnalysisDataService::Instance().remove("test_Rebin_masked_ws");
  }

  void testMaskedBinsIntegratedCounts() {
    Workspace2D_sptr test_in1D = Create1DWorkspace(51);
    test_in1D->setDistribution(false);
    AnalysisDataService::Instance().add("test_Rebin_mask_raw", test_in1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_Rebin_mask_raw");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_unmasked");
    rebin.setPropertyValue("Params", "1.5,3.0,12,-0.1,30");
    rebin.execute();

    MatrixWorkspace_sptr input =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_Rebin_unmasked");
    auto &inX = input->x(0);
    auto &inY = input->y(0);

    maskFirstBins("test_Rebin_mask_raw", "test_Rebin_masked_ws", 10.0);

    rebin.setPropertyValue("InputWorkspace", "test_Rebin_masked_ws");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_masked_ws");
    rebin.setPropertyValue("Params", "1.5,3.0,12,-0.1,30");
    rebin.execute();
    MatrixWorkspace_sptr masked =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_Rebin_masked_ws");
    auto &outX = masked->x(0);
    auto &outY = masked->y(0);

    const MatrixWorkspace::MaskList &mask = masked->maskedBins(0);

    // turn the mask list into an array like the Y values
    MantidVec weights(outY.size(), 0);
    for (auto it : mask) {
      weights[it.first] = it.second;
    }

    // the degree of masking must be the same as the reduction in the y-value,
    // for distributions, this is the easy case
    for (size_t i = 0; i < outY.size(); ++i) {
      size_t inBin =
          std::lower_bound(inX.begin(), inX.end(), outX[i]) - inX.begin();
      if (inBin < inX.size() - 2) {
        TS_ASSERT_DELTA(outY[i] / inY[inBin], 1 - weights[i], 0.000001);
      }
    }
    // the above formula tests the criterian that must be true for masking,
    // however we need some more specific tests incase outY.empty() or something
    TS_ASSERT_DELTA(outY[1], 0.0, 0.000001)
    TS_ASSERT_DELTA(outY[2], 1.0, 0.000001)
    TS_ASSERT_DELTA(outY[3], 6.0, 0.000001)
    TS_ASSERT_DELTA(weights[2], 1 - (0.25 / 3.0), 0.000001)

    AnalysisDataService::Instance().remove("test_Rebin_masked_ws");
    AnalysisDataService::Instance().remove("test_Rebin_unmasked");
    AnalysisDataService::Instance().remove("test_Rebin_mask_raw");
  }

  void test_FullBinsOnly_Fixed() {
    std::vector<double> xExpected = {0.5, 2.5, 4.5, 6.5};
    std::vector<double> yExpected(3, 8.0);
    std::string params = "2.0";
    do_test_FullBinsOnly(params, yExpected, xExpected);
  }

  void test_FullBinsOnly_Variable() {
    std::vector<double> xExpected = {0.5, 1.5, 2.5, 3.2, 3.9, 4.6, 6.6};
    std::vector<double> yExpected = {4.0, 4.0, 2.8, 2.8, 2.8, 8.0};
    std::string params = "0.5, 1.0, 3.1, 0.7, 5.0, 2.0, 7.25";
    do_test_FullBinsOnly(params, yExpected, xExpected);
  }

  void test_parallel_cloned() {
    ParallelTestHelpers::runParallel(run_rebin,
                                     "Parallel::StorageMode::Cloned");
  }

  void test_parallel_distributed() {
    ParallelTestHelpers::runParallel(run_rebin,
                                     "Parallel::StorageMode::Distributed");
  }

  void test_parallel_master_only() {
    ParallelTestHelpers::runParallel(run_rebin,
                                     "Parallel::StorageMode::MasterOnly");
  }

  void test_parallel_only_bin_width() {
    ParallelTestHelpers::runParallel(run_rebin_params_only_bin_width,
                                     "Parallel::StorageMode::Cloned");
    ParallelTestHelpers::runParallel(run_rebin_params_only_bin_width,
                                     "Parallel::StorageMode::Distributed");
    ParallelTestHelpers::runParallel(run_rebin_params_only_bin_width,
                                     "Parallel::StorageMode::MasterOnly");
  }

private:
  Workspace2D_sptr Create1DWorkspace(int size) {
    auto retVal = createWorkspace<Workspace2D>(1, size, size - 1);
    double j = 1.0;
    for (int i = 0; i < size; i++) {
      retVal->dataX(0)[i] = j * 0.5;
      j += 1.5;
    }
    retVal->setCounts(0, size - 1, 3.0);
    retVal->setCountVariances(0, size - 1, 3.0);
    return retVal;
  }

  Workspace2D_sptr Create2DWorkspace(int xlen, int ylen) {
    BinEdges x1(xlen, HistogramData::LinearGenerator(0.5, 0.75));
    Counts y1(xlen - 1, 3.0);
    CountStandardDeviations e1(xlen - 1, sqrt(3.0));

    auto retVal = createWorkspace<Workspace2D>(ylen, xlen, xlen - 1);

    for (int i = 0; i < ylen; i++) {
      retVal->setBinEdges(i, x1);
      retVal->setCounts(i, y1);
      retVal->setCountStandardDeviations(i, e1);
    }

    return retVal;
  }

  void maskFirstBins(const std::string &in, const std::string &out,
                     double maskBinsTo) {
    MaskBins mask;
    mask.initialize();
    mask.setPropertyValue("InputWorkspace", in);
    mask.setPropertyValue("OutputWorkspace", out);
    mask.setProperty("XMin", 0.0);
    mask.setProperty("XMax", maskBinsTo);
    mask.execute();
  }

  void do_test_FullBinsOnly(const std::string &params,
                            const std::vector<double> &yExpected,
                            const std::vector<double> &xExpected) {
    ScopedWorkspace inWsEntry(Create1DWorkspace(10));
    ScopedWorkspace outWsEntry;

    try {
      Rebin rebin;
      rebin.initialize();
      rebin.setPropertyValue("InputWorkspace", inWsEntry.name());
      rebin.setPropertyValue("OutputWorkspace", outWsEntry.name());
      rebin.setPropertyValue("Params", params);
      rebin.setProperty("FullBinsOnly", true);
      rebin.execute();
    } catch (std::runtime_error &e) {
      TS_FAIL(e.what());
      return;
    }

    auto ws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(outWsEntry.retrieve());

    if (!ws) {
      TS_FAIL("Unable to retrieve result workspace");
      return; // Nothing else to check
    }

    auto &xValues = ws->x(0);
    TS_ASSERT_DELTA(xValues.rawData(), xExpected, 0.001);

    auto &yValues = ws->y(0);
    TS_ASSERT_DELTA(yValues.rawData(), yExpected, 0.001);
  }
};

class RebinTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RebinTestPerformance *createSuite() {
    return new RebinTestPerformance();
  }
  static void destroySuite(RebinTestPerformance *suite) { delete suite; }

  RebinTestPerformance() {
    ws = WorkspaceCreationHelper::create2DWorkspaceBinned(5000, 20000);
  }

  void test_rebin() {
    Rebin rebin;
    rebin.initialize();
    rebin.setProperty("InputWorkspace", ws);
    rebin.setPropertyValue("OutputWorkspace", "out");
    rebin.setPropertyValue("Params", "50,1.77,18801");
    TS_ASSERT(rebin.execute());
  }

private:
  MatrixWorkspace_sptr ws;
};
#endif /* REBINTEST */
