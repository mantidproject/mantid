// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/CreateWorkspace.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidAlgorithms/MaskBins.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidHistogramData/LinearGenerator.h"

#include <numeric>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;

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

  /* some basic validation tests */

  void test_make_three_params_default() {
    const std::string IN_WKSP("creates_three_params_in");
    const std::string OUT_WKSP("creates_three_params_out");

    // test that if only a binwidth is given, will create a full set of rebin params
    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    AnalysisDataService::Instance().add(IN_WKSP, test_in1D);

    const double WIDTH(2.0);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin.setPropertyValue("OutputWorkspace", OUT_WKSP);
    rebin.setProperty("Params", std::vector<double>{WIDTH});
    // verify that with the single rebin param, the algo will successfuly execute
    TS_ASSERT_THROWS_NOTHING(rebin.execute());
    TS_ASSERT(rebin.isExecuted());

    // ensure that the properties are not changed in default mode
    // the "Params" property should still have a single value, 2.0
    std::vector<double> rbParams = rebin.getProperty("Params");
    TS_ASSERT_EQUALS(rbParams.size(), 1);
    TS_ASSERT_EQUALS(2.0, rbParams[0]);

    AnalysisDataService::Instance().remove(IN_WKSP);
    AnalysisDataService::Instance().remove(OUT_WKSP);
  }

  void test_failure_bad_log_binning_endpoints() {
    // ensure that rebinning will fail if log binning from negative to positive
    // this fails fast, due to the rebin param property validator
    Rebin rebin;
    rebin.initialize();
    TS_ASSERT_THROWS_ANYTHING(rebin.setPropertyValue("Params", "-1.0,-1.0,1000.0"));
    TS_ASSERT(!rebin.isExecuted());
  }

  void test_failure_bad_rebin_params() {
    // ensure that rebinning will fail if log binning from negative to positive
    // this fails fast, due to the rebin param property validator
    Rebin rebin;
    rebin.initialize();
    TS_ASSERT_THROWS_ANYTHING(rebin.setProperty("Params", std::vector<double>{-1.0, -1.0, 10.0}));
    TS_ASSERT_THROWS_ANYTHING(rebin.setProperty("Params", std::vector<double>{100.0, -1.0, 10.0}));
    TS_ASSERT_THROWS_ANYTHING(rebin.setProperty("Params", std::vector<double>{100.0, 1.0, 10.0}));
    TS_ASSERT_THROWS_ANYTHING(rebin.setProperty("Params", std::vector<double>{1.0, 10.0}));
    TS_ASSERT_THROWS_ANYTHING(rebin.setProperty("Params", std::vector<double>{1.0, -1.0, 10.0, 12.0}));
    TS_ASSERT_THROWS_ANYTHING(rebin.setProperty("Params", std::vector<double>{1.0, 0.0, 2.0}));
    TS_ASSERT_THROWS_ANYTHING(rebin.setProperty("Params", std::vector<double>{0.0}));
    TS_ASSERT_THROWS_ANYTHING(rebin.setProperty("Params", std::vector<double>{}));
    TS_ASSERT(!rebin.isExecuted());
  }

  void test_failure_mixed_power_and_log() {
    Rebin rebin;
    rebin.initialize();
    rebin.setProperty("Params", std::vector<double>{1.0, -1.0, 10.0});
    rebin.setProperty("Power", 0.5);
    auto errmsgs = rebin.validateInputs();
    auto errmsg = errmsgs.find("Params");
    TS_ASSERT(errmsg != errmsgs.end());
    TS_ASSERT(errmsg->second.substr(0, 20) == "Provided width value");
  }

  /* execution tests */

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
    MatrixWorkspace_sptr rebindata = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_out");
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
    MatrixWorkspace_sptr rebindata = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_out");

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
    MatrixWorkspace_sptr rebindata = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_out");
    auto &outX = rebindata->x(0);

    TS_ASSERT_EQUALS(outX.size(), 11);
    TS_ASSERT_DELTA(outX[0], 1.0, 1e-5);
    TS_ASSERT_DELTA(outX[1], 2.0, 1e-5);
    TS_ASSERT_DELTA(outX[2], 4.0, 1e-5); // and so on...
    TS_ASSERT_DELTA(outX[10], 1000.0, 1e-5);

    bool dist = rebindata->isDistribution();
    TS_ASSERT(dist);

    TS_ASSERT(checkBinWidthMonotonic(rebindata, Monotonic::INCREASE));

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
    MatrixWorkspace_sptr rebindata = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_out");

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

  void do_test_EventWorkspace(EventType eventType, bool inPlace, bool PreserveEvents, bool expectOutputEvent) {
    // Two events per bin
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::createEventWorkspace2(50, 100);
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
        outWS = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outName)));
    TS_ASSERT(outWS);
    if (!outWS)
      return;

    // Is the output gonna be events?
    if (expectOutputEvent) {
      eventOutWS = std::dynamic_pointer_cast<EventWorkspace>(outWS);
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

  void testEventWorkspace_InPlace_PreserveEvents() { do_test_EventWorkspace(TOF, true, true, true); }

  void testEventWorkspace_InPlace_PreserveEvents_weighted() { do_test_EventWorkspace(WEIGHTED, true, true, true); }

  void testEventWorkspace_InPlace_PreserveEvents_weightedNoTime() {
    do_test_EventWorkspace(WEIGHTED_NOTIME, true, true, true);
  }

  void testEventWorkspace_InPlace_NoPreserveEvents() { do_test_EventWorkspace(TOF, true, false, false); }

  void testEventWorkspace_InPlace_NoPreserveEvents_weighted() { do_test_EventWorkspace(WEIGHTED, true, false, false); }

  void testEventWorkspace_InPlace_NoPreserveEvents_weightedNoTime() {
    do_test_EventWorkspace(WEIGHTED_NOTIME, true, false, false);
  }

  void testEventWorkspace_NotInPlace_NoPreserveEvents() { do_test_EventWorkspace(TOF, false, false, false); }

  void testEventWorkspace_NotInPlace_NoPreserveEvents_weighted() {
    do_test_EventWorkspace(WEIGHTED, false, false, false);
  }

  void testEventWorkspace_NotInPlace_NoPreserveEvents_weightedNoTime() {
    do_test_EventWorkspace(WEIGHTED_NOTIME, false, false, false);
  }

  void testEventWorkspace_NotInPlace_PreserveEvents() { do_test_EventWorkspace(TOF, false, true, true); }

  void testEventWorkspace_NotInPlace_PreserveEvents_weighted() { do_test_EventWorkspace(WEIGHTED, false, true, true); }

  void testEventWorkspace_NotInPlace_PreserveEvents_weightedNoTime() {
    do_test_EventWorkspace(WEIGHTED_NOTIME, false, true, true);
  }

  void testRebinPointData() {
    Workspace2D_sptr input = Create1DWorkspace(51);
    AnalysisDataService::Instance().add("test_RebinPointDataInput", input);

    Mantid::API::Algorithm_sptr ctpd = Mantid::API::AlgorithmFactory::Instance().create("ConvertToPointData", 1);
    ctpd->initialize();
    ctpd->setPropertyValue("InputWorkspace", "test_RebinPointDataInput");
    ctpd->setPropertyValue("OutputWorkspace", "test_RebinPointDataInput");
    ctpd->execute();

    Mantid::API::Algorithm_sptr reb = Mantid::API::AlgorithmFactory::Instance().create("Rebin", 1);
    reb->initialize();
    TS_ASSERT_THROWS_NOTHING(reb->setPropertyValue("InputWorkspace", "test_RebinPointDataInput"));
    reb->setPropertyValue("OutputWorkspace", "test_RebinPointDataOutput");
    reb->setPropertyValue("Params", "7,0.75,23");
    TS_ASSERT_THROWS_NOTHING(reb->execute());

    TS_ASSERT(reb->isExecuted());

    MatrixWorkspace_sptr outWS = std::dynamic_pointer_cast<MatrixWorkspace>(
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
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_Rebin_masked_ws");
    auto &outX = rebindata->x(0);
    auto &outY = rebindata->y(0);

    MatrixWorkspace_sptr input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_Rebin_mask_dist");
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
      size_t inBin = std::lower_bound(inX.begin(), inX.end(), outX[i]) - inX.begin();
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

    MatrixWorkspace_sptr input = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_Rebin_unmasked");
    auto &inX = input->x(0);
    auto &inY = input->y(0);

    maskFirstBins("test_Rebin_mask_raw", "test_Rebin_masked_ws", 10.0);

    rebin.setPropertyValue("InputWorkspace", "test_Rebin_masked_ws");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_masked_ws");
    rebin.setPropertyValue("Params", "1.5,3.0,12,-0.1,30");
    rebin.execute();
    MatrixWorkspace_sptr masked = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_Rebin_masked_ws");
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
      size_t inBin = std::lower_bound(inX.begin(), inX.end(), outX[i]) - inX.begin();
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

  void test_reverseLogSimple() {
    // Test UseReverseLogarithmic alone
    Workspace2D_sptr test_1D = Create1DWorkspace(51);
    test_1D->setDistribution(false);
    AnalysisDataService::Instance().add("test_Rebin_revLog", test_1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("Params", "1, -1, 37");
    rebin.setProperty("UseReverseLogarithmic", true);
    TS_ASSERT_THROWS_NOTHING(rebin.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_Rebin_revLog");
    auto &outX = out->x(0);

    TS_ASSERT_EQUALS(outX.size(), 6);
    TS_ASSERT_DELTA(outX[0], 1, 1e-5);
    TS_ASSERT_DELTA(outX[1], 22, 1e-5);
    TS_ASSERT_DELTA(outX[2], 30, 1e-5);
    TS_ASSERT_DELTA(outX[3], 34, 1e-5);
    TS_ASSERT_DELTA(outX[4], 36, 1e-5);
    TS_ASSERT_DELTA(outX[5], 37, 1e-5);

    TS_ASSERT(checkBinWidthMonotonic(out, Monotonic::DECREASE));

    AnalysisDataService::Instance().remove("test_Rebin_revLog");
  }

  void test_reverseLogDiffStep() {
    // Test UseReverseLog with a different step than the usual -1
    Workspace2D_sptr test_1D = Create1DWorkspace(51);
    test_1D->setDistribution(false);
    AnalysisDataService::Instance().add("test_Rebin_revLog", test_1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("Params", "1, -2, 42");
    rebin.setProperty("UseReverseLogarithmic", true);
    TS_ASSERT_THROWS_NOTHING(rebin.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_Rebin_revLog");
    auto &outX = out->x(0);

    TS_ASSERT_EQUALS(outX.size(), 4);
    TS_ASSERT_DELTA(outX[0], 1, 1e-5);
    TS_ASSERT_DELTA(outX[1], 34, 1e-5);
    TS_ASSERT_DELTA(outX[2], 40, 1e-5);
    TS_ASSERT_DELTA(outX[3], 42, 1e-5);

    TS_ASSERT(checkBinWidthMonotonic(out, Monotonic::DECREASE));

    AnalysisDataService::Instance().remove("test_Rebin_revLog");
  }

  void test_reverseLogEdgeCase() {
    // Check the case where the parameters given are so that the edges of the bins fall perfectly, and so no padding is
    // needed
    Workspace2D_sptr test_1D = Create1DWorkspace(51);
    test_1D->setDistribution(false);
    AnalysisDataService::Instance().add("test_Rebin_revLog", test_1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("Params", "1, -1, 16");
    rebin.setProperty("UseReverseLogarithmic", true);
    TS_ASSERT_THROWS_NOTHING(rebin.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_Rebin_revLog");
    auto &outX = out->x(0);

    TS_ASSERT_EQUALS(outX.size(), 5);
    TS_ASSERT_DELTA(outX[0], 1, 1e-5)
    TS_ASSERT_DELTA(outX[1], 9, 1e-5);
    TS_ASSERT_DELTA(outX[2], 13, 1e-5);
    TS_ASSERT_DELTA(outX[3], 15, 1e-5);
    TS_ASSERT_DELTA(outX[4], 16, 1e-5);

    TS_ASSERT(checkBinWidthMonotonic(out, Monotonic::DECREASE));

    AnalysisDataService::Instance().remove("test_Rebin_revLog");
  }

  void test_reverseLogAgainst() {
    // Test UseReverseLogarithmic with a linear spacing before it
    Workspace2D_sptr test_1D = Create1DWorkspace(51);
    test_1D->setDistribution(false);
    AnalysisDataService::Instance().add("test_Rebin_revLog", test_1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("Params", "1, 2, 5, -1, 100");
    rebin.setProperty("UseReverseLogarithmic", true);
    TS_ASSERT_THROWS_NOTHING(rebin.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_Rebin_revLog");
    auto &outX = out->x(0);

    TS_ASSERT_EQUALS(outX.size(), 7); // 2 lin + 4 log
    TS_ASSERT_DELTA(outX[0], 1, 1e-5);
    TS_ASSERT_DELTA(outX[1], 3, 1e-5);
    TS_ASSERT_DELTA(outX[2], 5, 1e-5);
    TS_ASSERT_DELTA(outX[3], 65, 1e-5);
    TS_ASSERT_DELTA(outX[4], 85, 1e-5);
    TS_ASSERT_DELTA(outX[5], 95, 1e-5);
    TS_ASSERT_DELTA(outX[6], 100, 1e-5);

    AnalysisDataService::Instance().remove("test_Rebin_revLog");
  }

  void test_reverseLogBetween() {
    // Test UseReverseLogarithmic between 2 linear binnings

    Workspace2D_sptr test_1D = Create1DWorkspace(51);
    test_1D->setDistribution(false);
    AnalysisDataService::Instance().add("test_Rebin_revLog", test_1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("Params", "1, 2, 5, -1, 100, 2, 110");
    rebin.setProperty("UseReverseLogarithmic", true);
    TS_ASSERT_THROWS_NOTHING(rebin.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_Rebin_revLog");
    auto &outX = out->x(0);

    TS_ASSERT_EQUALS(outX.size(), 12);
    TS_ASSERT_DELTA(outX[0], 1, 1e-5);
    TS_ASSERT_DELTA(outX[1], 3, 1e-5);
    TS_ASSERT_DELTA(outX[2], 5, 1e-5);
    TS_ASSERT_DELTA(outX[3], 65, 1e-5);
    TS_ASSERT_DELTA(outX[4], 85, 1e-5);
    TS_ASSERT_DELTA(outX[5], 95, 1e-5);
    TS_ASSERT_DELTA(outX[6], 100, 1e-5);
    TS_ASSERT_DELTA(outX[7], 102, 1e-5);
    TS_ASSERT_DELTA(outX[11], 110, 1e-5);

    AnalysisDataService::Instance().remove("test_Rebin_revLog");
  }

  void test_reverseLogFullBinsOnly() {
    // Test UseReverseLogarithmic with the FullBinsOnly option checked. It should not change anything from the non
    // checked version.
    Workspace2D_sptr test_1D = Create1DWorkspace(51);
    test_1D->setDistribution(false);
    AnalysisDataService::Instance().add("test_Rebin_revLog", test_1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("Params", "1, -1, 37");
    rebin.setProperty("UseReverseLogarithmic", true);
    rebin.setProperty("FullBinsOnly", true);
    TS_ASSERT_THROWS_NOTHING(rebin.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_Rebin_revLog");
    auto &outX = out->x(0);

    TS_ASSERT_EQUALS(outX.size(), 6);
    TS_ASSERT_DELTA(outX[0], 1, 1e-5);
    TS_ASSERT_DELTA(outX[1], 22, 1e-5);
    TS_ASSERT_DELTA(outX[2], 30, 1e-5);
    TS_ASSERT_DELTA(outX[3], 34, 1e-5);
    TS_ASSERT_DELTA(outX[4], 36, 1e-5);
    TS_ASSERT_DELTA(outX[5], 37, 1e-5);

    TS_ASSERT(checkBinWidthMonotonic(out, Monotonic::DECREASE));

    AnalysisDataService::Instance().remove("test_Rebin_revLog");
  }

  void test_reverseLogIncompleteFirstBin() {
    // Test UseReverseLogarithmic with a first bin that is incomplete, but still bigger than the next one so not merged.
    Workspace2D_sptr test_1D = Create1DWorkspace(51);
    test_1D->setDistribution(false);
    AnalysisDataService::Instance().add("test_Rebin_revLog", test_1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("Params", "1, -1, 60");
    rebin.setProperty("UseReverseLogarithmic", true);
    rebin.setProperty("FullBinsOnly", true);
    TS_ASSERT_THROWS_NOTHING(rebin.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_Rebin_revLog");
    auto &outX = out->x(0);

    TS_ASSERT_EQUALS(outX.size(), 7);
    TS_ASSERT_DELTA(outX[0], 1, 1e-5);
    TS_ASSERT_DELTA(outX[1], 29, 1e-5);
    TS_ASSERT_DELTA(outX[2], 45, 1e-5);
    TS_ASSERT_DELTA(outX[3], 53, 1e-5);
    TS_ASSERT_DELTA(outX[4], 57, 1e-5);
    TS_ASSERT_DELTA(outX[5], 59, 1e-5);
    TS_ASSERT_DELTA(outX[6], 60, 1e-5);

    TS_ASSERT(checkBinWidthMonotonic(out, Monotonic::DECREASE));

    AnalysisDataService::Instance().remove("test_Rebin_revLog");
  }

  void test_inversePowerSquareRoot() {
    // Test InversePower in a simple case of square root sum
    Workspace2D_sptr test_1D = Create1DWorkspace(51);
    test_1D->setDistribution(false);
    AnalysisDataService::Instance().add("test_Rebin_revLog", test_1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("Params", "1, 1, 10");
    rebin.setPropertyValue("Power", "0.5");
    TS_ASSERT_THROWS_NOTHING(rebin.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_Rebin_revLog");
    auto &outX = out->x(0);

    TS_ASSERT_EQUALS(outX.size(), 28);
    TS_ASSERT_DELTA(outX[0], 1, 1e-5);
    TS_ASSERT_DELTA(outX[1], 2, 1e-5);
    TS_ASSERT_DELTA(outX[2], 2.707106781, 1e-5);
    TS_ASSERT_DELTA(outX[3], 3.28445705, 1e-5);
    TS_ASSERT_DELTA(outX[27], 10, 1e-5);

    TS_ASSERT(checkBinWidthMonotonic(out, Monotonic::DECREASE));

    AnalysisDataService::Instance().remove("test_Rebin_revLog");
  }

  void test_inversePowerHarmonic() {
    // Test InversePower in a simple case of harmonic serie
    Workspace2D_sptr test_1D = Create1DWorkspace(51);
    test_1D->setDistribution(false);
    AnalysisDataService::Instance().add("test_Rebin_revLog", test_1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("Params", "1, 1, 5");
    rebin.setPropertyValue("Power", "1");
    TS_ASSERT_THROWS_NOTHING(rebin.execute());

    MatrixWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_Rebin_revLog");
    auto &outX = out->x(0);

    TS_ASSERT_EQUALS(outX.size(), 31);
    TS_ASSERT_DELTA(outX[0], 1, 1e-5);
    TS_ASSERT_DELTA(outX[1], 2, 1e-5);
    TS_ASSERT_DELTA(outX[2], 2.5, 1e-5);
    TS_ASSERT_DELTA(outX[3], 2.8333333, 1e-5);
    TS_ASSERT_DELTA(outX[30], 5, 1e-5);

    TS_ASSERT(checkBinWidthMonotonic(out, Monotonic::DECREASE, true));

    AnalysisDataService::Instance().remove("test_Rebin_revLog");
  }

  void test_inversePowerValidateHarmonic() {
    // Test that the validator which forbid creating more than 10000 bins works in a harmonic series case
    Workspace2D_sptr test_1D = Create1DWorkspace(51);
    test_1D->setDistribution(false);
    AnalysisDataService::Instance().add("test_Rebin_revLog", test_1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("Params", "1, 1, 100");
    rebin.setPropertyValue("Power", "1");
    TS_ASSERT_THROWS(rebin.execute(), const std::runtime_error &);
    AnalysisDataService::Instance().remove("test_Rebin_revLog");
  }

  void test_inversePowerValidateInverseSquareRoot() {
    // Test that the validator which forbid breating more than 10000 bins works in an inverse square root case
    // We test both because they rely on different formula to compute the expected number of bins.
    Workspace2D_sptr test_1D = Create1DWorkspace(51);
    test_1D->setDistribution(false);
    AnalysisDataService::Instance().add("test_Rebin_revLog", test_1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("OutputWorkspace", "test_Rebin_revLog");
    rebin.setPropertyValue("Params", "1, 1, 1000");
    rebin.setPropertyValue("Power", "0.5");
    TS_ASSERT_THROWS(rebin.execute(), const std::runtime_error &);
    AnalysisDataService::Instance().remove("test_Rebin_revLog");
  }

  //__/__/__/__/__/__/__/__/__/__/__/__/__/__/__/__/__//
  //          TESTS FOR BINNING MODE BEHAVIOR         //
  //__/__/__/__/__/__/__/__/__/__/__/__/__/__/__/__/__//

  void test_failure_bad_binning_modeset() {
    // ensure failure if bad binning mode is set
    // fast failure because of string list validator
    Rebin rebin;
    rebin.initialize();
    TS_ASSERT_THROWS_ANYTHING(
        rebin.setProperty("BinningMode", "FunkyCrosswaysBinning")); // this is not a known binning mode
  }

  void test_failure_power_modeset_without_power() {
    // Test Power binning mode will fail if no power is given
    Rebin rebin;
    rebin.initialize();
    rebin.setProperty("Params", std::vector<double>{1.2, 1.2, 12.22});
    rebin.setPropertyValue("BinningMode", "Power");
    auto errmsgs = rebin.validateInputs();
    auto errmsg = errmsgs.find("Power");
    TS_ASSERT(errmsg != errmsgs.end());
    TS_ASSERT(errmsg->second.substr(0, 35) == "The binning mode was set to 'Power'");
  }

  void test_failure_bad_log_binning_endpoints_modeset() {
    // ensure failure with log binning from neg to pos with modeset
    Rebin rebin;
    rebin.initialize();
    auto ws1 = Create1DWorkspace(10);
    AnalysisDataService::Instance().add("ws1", ws1);
    rebin.setRethrows(true);
    rebin.setProperty("InputWorkspace", "ws1");
    rebin.setProperty("Params", std::vector<double>{-1.0, 1.0, 1000.0});
    rebin.setProperty("BinningMode", "Logarithmic");
    auto errmsgs = rebin.validateInputs();
    TS_ASSERT(!errmsgs.empty());
    auto errmsg = errmsgs.find("Params");
    TS_ASSERT(errmsg != errmsgs.end());
    TS_ASSERT(errmsg->second.substr(0, 33) == "Cannot create logarithmic binning");
    AnalysisDataService::Instance().remove("ws1");
  }

  void do_test_property_unchanged(
      // check that certain properties are unchanged by the execution of the algo
      std::string property, bool propValue, std::string binMode, double step, double power) {

    const std::string IN_WKSP("properties_unchanged_in");
    const std::string OUT_WKSP("properties_unchanged_out");

    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    AnalysisDataService::Instance().add(IN_WKSP, test_in1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin.setPropertyValue("OutputWorkspace", OUT_WKSP);
    rebin.setProperty("Params", std::vector<double>{step});
    rebin.setProperty("Power", power);
    if (binMode == "Power")
      rebin.setProperty("Power", 0.5);
    rebin.setProperty("BinningMode", binMode);
    rebin.setProperty(property, propValue);
    // verify that with the single rebin param, the algo will successfully execute
    TS_ASSERT_THROWS_NOTHING(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    TS_ASSERT_EQUALS(propValue, bool(rebin.getProperty(property)));
    AnalysisDataService::Instance().remove(IN_WKSP);
    AnalysisDataService::Instance().remove(OUT_WKSP);
  }

  void test_some_properties_always_unchanged() {
    // these are properties which should not be changed anywhere inside algo
    std::vector<std::string> props{"PreserveEvents", "FullBinsOnly", "IgnoreBinErrors"};
    std::vector<bool> propV{false, true};
    // try in all binning modes
    std::vector<std::string> binModes{"Linear", "Logarithmic", "ReverseLogarithmic", "Power"};
    std::vector<double> steps{1.0, -1.0};
    std::vector<double> powers{0.0, 0.5};

    // run it every way possible
    for (size_t ip = 0; ip < props.size(); ip++) {
      for (size_t ib = 0; ib < binModes.size(); ib++) {
        for (size_t is = 0; is < steps.size(); is++) {
          for (size_t j = 0; j < powers.size(); j++) {
            for (size_t k = 0; k < propV.size(); k++) {
              do_test_property_unchanged(props[ip], propV[k], binModes[ib], steps[is], powers[j]);
            }
          }
        }
      }
    }
  }

  void do_test_changes_usereverselogarithmic(std::string binMode, double step, bool rvrs) {
    const std::string IN_WKSP("reset_reverse_log_in");
    const std::string OUT_WKSP("reset_reverse_log_out");

    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    AnalysisDataService::Instance().add(IN_WKSP, test_in1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin.setPropertyValue("OutputWorkspace", OUT_WKSP);
    rebin.setProperty("Params", std::vector<double>{1.2, step, 12.22});
    rebin.setProperty("UseReverseLogarithmic", rvrs);
    rebin.setProperty("BinningMode", binMode);
    if (binMode == "Power")
      rebin.setProperty("Power", 0.5);
    auto errmsgs = rebin.validateInputs();
    TS_ASSERT(errmsgs.empty());
    // exactly one of these will be true, the other false
    bool validatedRvrs = rebin.getProperty("UseReverseLogarithmic");
    TS_ASSERT_DIFFERS(rvrs, validatedRvrs);
    // test successful execution
    TS_ASSERT_THROWS_NOTHING(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    // ensure nothing changed in execution
    bool stillRvrs = rebin.getProperty("UseReverseLogarithmic");
    TS_ASSERT_EQUALS(validatedRvrs, stillRvrs);

    AnalysisDataService::Instance().remove(IN_WKSP);
    AnalysisDataService::Instance().remove(OUT_WKSP);
  }

  void test_all_binning_modes_change_usereverselogarithmic() {
    std::vector<std::string> binModes{"Linear", "Logarithmic", "ReverseLogarithmic", "Power"};
    std::vector<double> steps{1.2, -1.2, -1.2, 1.2};
    std::vector<bool> useRvrsLog{true, true, false, true};

    for (size_t i = 0; i < binModes.size(); i++) {
      do_test_changes_usereverselogarithmic(binModes[i], steps[i], useRvrsLog[i]);
    }
  }

  void test_reverse_log_doesnt_change_usereverselogarithmic() {
    // this additional test ensures the ReverseLog binning mode doesn't
    // accidentally unset the "UseReverseLogarithmic" flag

    const std::string IN_WKSP("no_reset_reverse_log_in");
    const std::string OUT_WKSP("no_reset_reverse_log_out");

    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    AnalysisDataService::Instance().add(IN_WKSP, test_in1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin.setPropertyValue("OutputWorkspace", OUT_WKSP);
    rebin.setProperty("Params", std::vector<double>{1.2, -1.2, 12.22});
    rebin.setProperty("UseReverseLogarithmic", true);
    rebin.setProperty("BinningMode", "ReverseLogarithmic");
    auto errmsgs = rebin.validateInputs();
    TS_ASSERT(errmsgs.empty());
    // assert this is still true
    TS_ASSERT(bool(rebin.getProperty("UseReverseLogarithmic")));
    // test successful execution
    TS_ASSERT_THROWS_NOTHING(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    // ensure nothing changed in execution
    TS_ASSERT(bool(rebin.getProperty("UseReverseLogarithmic")));

    AnalysisDataService::Instance().remove(IN_WKSP);
    AnalysisDataService::Instance().remove(OUT_WKSP);
  }

  void do_test_changes_rebin_params(std::string binMode, double step, double good_step, bool only_step = false) {

    const std::string IN_WKSP("change_rebin_params_in");
    const std::string OUT_WKSP("change_rebin_params_out");

    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    AnalysisDataService::Instance().add(IN_WKSP, test_in1D);

    double start, stop;
    std::vector<double> params;
    if (only_step) {
      MatrixWorkspace_sptr inputdata = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(IN_WKSP);
      auto &outX = inputdata->x(0);
      start = outX[0];
      stop = outX[outX.size() - 1];
      params.push_back(step);
    } else {
      start = 1.0;
      stop = 10.0;
      params.insert(params.begin(), {start, step, stop});
    }
    TS_ASSERT_DIFFERS(params.size(), 0);
    TS_ASSERT_EQUALS(params.size() % 2, 1);

    // check that algo will change rebin params
    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin.setPropertyValue("OutputWorkspace", OUT_WKSP);
    rebin.setProperty("Params", params);
    rebin.setPropertyValue("BinningMode", binMode);
    if (binMode == "Power")
      rebin.setProperty("Power", 0.5);
    auto errmsgs = rebin.validateInputs();
    TS_ASSERT(errmsgs.empty());

    std::vector<double> rbParams = rebin.getProperty("Params");
    TS_ASSERT_EQUALS(rbParams.size(), 3);
    TS_ASSERT_EQUALS(rbParams[0], start);
    TS_ASSERT_EQUALS(rbParams[1], good_step);
    TS_ASSERT_EQUALS(rbParams[2], stop);

    // ensure execution, and nothing changes in execution
    TS_ASSERT_THROWS_NOTHING(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    std::vector<double> rebinAgainParams = rebin.getProperty("Params");
    TS_ASSERT_EQUALS(rbParams, rebinAgainParams);

    AnalysisDataService::Instance().remove(IN_WKSP);
    AnalysisDataService::Instance().remove(OUT_WKSP);
  }

  void test_all_binning_modes_change_rebin_params() {
    std::vector<std::string> binModes{"Linear", "Logarithmic", "ReverseLogarithmic", "Power"};
    std::vector<double> steps{-1.0, 1.0, 1.0, -1.0};
    std::vector<double> goodSteps{1.0, -1.0, -1.0, 1.0};

    for (size_t i = 0; i < binModes.size(); i++) {
      for (char only_step = 0; only_step < 2; only_step++) {
        do_test_changes_rebin_params(binModes[i], steps[i], goodSteps[i], only_step);
      }
    }
  }

  void do_test_changes_power(std::string binMode, double step) {
    // check that algo run in binning  mode will change power

    const std::string IN_WKSP("changes_power_in");
    const std::string OUT_WKSP("changes_power_out");

    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    AnalysisDataService::Instance().add(IN_WKSP, test_in1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin.setPropertyValue("OutputWorkspace", OUT_WKSP);
    rebin.setProperty("Params", std::vector<double>{1.0, step, 10.0});
    rebin.setProperty("Power", 0.5);
    rebin.setPropertyValue("BinningMode", binMode);
    auto errmsgs = rebin.validateInputs();
    TS_ASSERT(errmsgs.empty());
    // ensure that power was unset
    TS_ASSERT(double(rebin.getProperty("Power")) == 0.0);

    // ensure execution, and nothing changes in execution
    TS_ASSERT_THROWS_NOTHING(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    TS_ASSERT(double(rebin.getProperty("Power")) == 0.0);

    AnalysisDataService::Instance().remove(IN_WKSP);
    AnalysisDataService::Instance().remove(OUT_WKSP);
  }

  void test_all_binning_modes_change_power() {
    std::vector<std::string> binningModes{"Linear", "Logarithmic", "ReverseLogarithmic"};
    std::vector<double> steps{1.0, -1.0, -1.0};
    for (size_t i = 0; i < binningModes.size(); i++) {
      do_test_changes_power(binningModes[i], steps[i]);
    }
  }

  void test_default_linear_and_modeset_equal_everywhere() {
    // Check that linear binning mode equals to original behavior

    const std::string IN_WKSP("default_modeset_linear_in");
    const std::string OUT_WKSP1("default_equal_out");
    const std::string OUT_WKSP2("modeset_equal_out");

    const double START = 1.0, STEP = 1.0, STOP = 10.0;

    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    test_in1D->setDistribution(true);
    AnalysisDataService::Instance().add(IN_WKSP, test_in1D);

    // first run in the default manner for comparison
    Rebin rebin_default;
    rebin_default.initialize();
    rebin_default.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin_default.setPropertyValue("OutputWorkspace", OUT_WKSP1);
    rebin_default.setProperty("Params", std::vector<double>{START, STEP, STOP});
    TS_ASSERT_THROWS_NOTHING(rebin_default.execute());
    TS_ASSERT(rebin_default.isExecuted());

    // retrieve binned axis
    MatrixWorkspace_sptr rebindata_default = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUT_WKSP1);
    auto &outX_default = rebindata_default->x(0);

    // now run again using linear binning mode
    Rebin rebin_again;
    rebin_again.initialize();
    rebin_again.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin_again.setPropertyValue("OutputWorkspace", OUT_WKSP2);
    // set the params as wrong as possible
    rebin_again.setProperty("Params", std::vector<double>{START, -STEP, STOP});
    rebin_again.setProperty("Power", 0.5);
    rebin_again.setProperty("UseReverseLogarithmic", true);
    // set the binning mode and run
    rebin_again.setPropertyValue("BinningMode", "Linear");
    TS_ASSERT_THROWS_NOTHING(rebin_again.execute());
    TS_ASSERT(rebin_again.isExecuted());

    // retrieve binned axis
    MatrixWorkspace_sptr rebindata_again = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUT_WKSP2);
    auto &outX_again = rebindata_again->x(0);

    // verify that all bin edges are the same
    TS_ASSERT_EQUALS(outX_default.size(), outX_again.size());
    for (size_t i = 0; i < outX_default.size(); i++) {
      TS_ASSERT_EQUALS(outX_default[i], outX_again[i]);
    }

    AnalysisDataService::Instance().remove(IN_WKSP);
    AnalysisDataService::Instance().remove(OUT_WKSP1);
    AnalysisDataService::Instance().remove(OUT_WKSP2);
  }

  void test_default_log_and_modeset_equal_everywhere() {
    // Check that linear binning mode equals to original behavior

    const std::string IN_WKSP("default_modeset_log_in");
    const std::string OUT_WKSP1("default_equal_out");
    const std::string OUT_WKSP2("modeset_equal_out");

    const double START = 1.0, STEP = -1.0, STOP = 10.0;

    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    test_in1D->setDistribution(true);
    AnalysisDataService::Instance().add(IN_WKSP, test_in1D);

    // first run in the default manner for comparison
    Rebin rebin_default;
    rebin_default.initialize();
    rebin_default.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin_default.setPropertyValue("OutputWorkspace", OUT_WKSP1);
    rebin_default.setProperty("Params", std::vector<double>{START, STEP, STOP});
    TS_ASSERT_THROWS_NOTHING(rebin_default.execute());
    TS_ASSERT(rebin_default.isExecuted());

    // retrieve binned axis
    MatrixWorkspace_sptr rebindata_default = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUT_WKSP1);
    auto &outX_default = rebindata_default->x(0);

    // now run again using log mode
    Rebin rebin_again;
    rebin_again.initialize();
    rebin_again.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin_again.setPropertyValue("OutputWorkspace", OUT_WKSP2);
    // set the params as wrong as possible
    rebin_again.setProperty("Params", std::vector<double>{START, -STEP, STOP});
    rebin_again.setProperty("Power", 0.5);
    rebin_again.setProperty("UseReverseLogarithmic", true);
    // set the binning mode and run
    rebin_again.setPropertyValue("BinningMode", "Logarithmic");
    TS_ASSERT_THROWS_NOTHING(rebin_again.execute());
    TS_ASSERT(rebin_again.isExecuted());

    // retrieve binned axis
    MatrixWorkspace_sptr rebindata_again = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUT_WKSP2);
    auto &outX_again = rebindata_again->x(0);

    // verify that all bin edges are the same
    TS_ASSERT_EQUALS(outX_default.size(), outX_again.size());
    for (size_t i = 0; i < outX_default.size(); i++) {
      TS_ASSERT_EQUALS(outX_default[i], outX_again[i]);
    }

    // ensure the log bin steps are monotonically increasing
    TS_ASSERT(checkBinWidthMonotonic(rebindata_default, Monotonic::INCREASE, true));
    TS_ASSERT(checkBinWidthMonotonic(rebindata_again, Monotonic::INCREASE, true));

    AnalysisDataService::Instance().remove(IN_WKSP);
    AnalysisDataService::Instance().remove(OUT_WKSP1);
    AnalysisDataService::Instance().remove(OUT_WKSP2);
  }

  void test_default_reverse_log_and_modeset_equal_everywhere() {
    // Check that reverse logarithmic binning mode equals original behavior

    const std::string IN_WKSP("default_modeset_equal_in");
    const std::string OUT_WKSP1("default_equal_out");
    const std::string OUT_WKSP2("modeset_equal_out");

    const double START = 1.0, STEP = -1.0, STOP = 10.0;

    Workspace2D_sptr test_in1D = Create1DWorkspace(50);
    test_in1D->setDistribution(true);
    AnalysisDataService::Instance().add(IN_WKSP, test_in1D);

    // run once in the default manner
    Rebin rebin_default;
    rebin_default.initialize();
    rebin_default.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin_default.setPropertyValue("OutputWorkspace", OUT_WKSP1);
    rebin_default.setProperty("Params", std::vector<double>{START, STEP, STOP});
    rebin_default.setProperty("UseReverseLogarithmic", true);
    TS_ASSERT_THROWS_NOTHING(rebin_default.execute());
    TS_ASSERT(rebin_default.isExecuted());

    // retrieve rebinned axis
    MatrixWorkspace_sptr rebindata_default = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUT_WKSP1);
    auto &outX_default = rebindata_default->x(0);

    // now run again but with positive step, using logarithmic mode
    Rebin rebin_again;
    rebin_again.initialize();
    rebin_again.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin_again.setPropertyValue("OutputWorkspace", OUT_WKSP2);
    // set the params aswrong as possible
    rebin_again.setProperty("Params", std::vector<double>{START, -STEP, STOP});
    rebin_again.setProperty("UseReverseLogarithmic", false);
    rebin_again.setProperty("Power", "0.5");
    // set the binning mode and run
    rebin_again.setProperty("BinningMode", "ReverseLogarithmic");
    TS_ASSERT_THROWS_NOTHING(rebin_again.execute());
    TS_ASSERT(rebin_again.isExecuted());

    // retrieve rebinned axis
    MatrixWorkspace_sptr rebindata_again = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUT_WKSP2);
    auto &outX_again = rebindata_again->x(0);

    // verify that all bin edges are the same
    TS_ASSERT_EQUALS(outX_default.size(), outX_again.size());
    for (size_t i = 0; i < outX_default.size(); i++) {
      TS_ASSERT_EQUALS(outX_default[i], outX_again[i]);
    }

    // verify the reverse log bin steps are monotonically decreasing
    TS_ASSERT(checkBinWidthMonotonic(rebindata_default, Monotonic::DECREASE));
    TS_ASSERT(checkBinWidthMonotonic(rebindata_again, Monotonic::DECREASE));

    AnalysisDataService::Instance().remove(IN_WKSP);
    AnalysisDataService::Instance().remove(OUT_WKSP1);
    AnalysisDataService::Instance().remove(OUT_WKSP2);
  }

  void test_default_power_and_modeset_equal_everywhere() {
    // Test that modeset to power mode equals default behavior

    const std::string IN_WKSP("default_modeset_equal_in");
    const std::string OUT_WKSP1("default_equal_out");
    const std::string OUT_WKSP2("oldman_rebinagain");

    const double START = 1.0, STEP = 1.0, STOP = 10.0;

    Workspace2D_sptr test_1D = Create1DWorkspace(51);
    test_1D->setDistribution(false);
    AnalysisDataService::Instance().add(IN_WKSP, test_1D);

    // run once the default way
    Rebin rebin_once;
    rebin_once.initialize();
    rebin_once.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin_once.setPropertyValue("OutputWorkspace", OUT_WKSP1);
    rebin_once.setProperty("Params", std::vector<double>{START, STEP, STOP});
    rebin_once.setProperty("Power", 0.5);
    TS_ASSERT_THROWS_NOTHING(rebin_once.execute());

    // retrieve rebinned axis
    MatrixWorkspace_sptr out_once = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUT_WKSP1);
    auto &outX_once = out_once->x(0);

    // run again by setting the binning mode
    Rebin rebin_again;
    rebin_again.initialize();
    rebin_again.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin_again.setPropertyValue("OutputWorkspace", OUT_WKSP2);
    // set params as wrong as possible
    rebin_again.setProperty("Params",
                            std::vector<double>{START, -STEP, STOP}); // set step to negative, just to confuse it
    rebin_again.setProperty("Power", 0.5);
    rebin_again.setProperty("UseReverseLogarithmic", true);
    // set binning mode and execute
    rebin_again.setPropertyValue("BinningMode", "Power");
    TS_ASSERT_THROWS_NOTHING(rebin_again.execute());

    // retrieve rebinned axis
    MatrixWorkspace_sptr out_again = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUT_WKSP2);
    auto &outX_again = out_again->x(0);

    TS_ASSERT_EQUALS(outX_once.size(), outX_again.size());
    for (size_t i = 0; i < outX_once.size(); i++) {
      TS_ASSERT_EQUALS(outX_once[i], outX_again[i]);
    }

    TS_ASSERT(checkBinWidthMonotonic(out_once, Monotonic::DECREASE));
    TS_ASSERT(checkBinWidthMonotonic(out_again, Monotonic::DECREASE));

    AnalysisDataService::Instance().remove(IN_WKSP);
    AnalysisDataService::Instance().remove(OUT_WKSP1);
    AnalysisDataService::Instance().remove(OUT_WKSP2);
  }

  void do_test_ignores_power(std::string binMode, double step) {
    // Test that if the mode is set to linear, will NOT produce power binning,
    // even if the power is set to nonzero

    const std::string IN_WKSP("default_modeset_equal_in");
    const std::string OUT_WKSP1("default_equal_out");
    const std::string OUT_WKSP2("modeset_equal_out");
    const std::string OUT_WKSP3("oldman_rebinagain");

    const std::vector<double> PARAMS{step};

    Workspace2D_sptr test_1D = Create1DWorkspace(51);
    test_1D->setDistribution(false);
    AnalysisDataService::Instance().add(IN_WKSP, test_1D);

    // run once in the default method
    Rebin rebin_once;
    rebin_once.initialize();
    rebin_once.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin_once.setPropertyValue("OutputWorkspace", OUT_WKSP1);
    rebin_once.setProperty("Params", PARAMS);
    rebin_once.setProperty("Power", 0.0);
    if (binMode == "ReverseLogarithmic")
      rebin_once.setProperty("USeReverseLogarithmic", true);
    TS_ASSERT_THROWS_NOTHING(rebin_once.execute());

    MatrixWorkspace_sptr out_once = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUT_WKSP1);
    auto &outX_once = out_once->x(0);

    // run again, with the power set
    Rebin rebin_twice;
    rebin_twice.initialize();
    rebin_twice.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin_twice.setPropertyValue("OutputWorkspace", OUT_WKSP2);
    rebin_twice.setProperty("Params", PARAMS);
    rebin_twice.setProperty("Power", 0.5);
    rebin_twice.setProperty("BinningMode", binMode);
    TS_ASSERT_THROWS_NOTHING(rebin_twice.execute());

    // the algo validation will remove the power if Linear binning is set
    TS_ASSERT_EQUALS(double(rebin_twice.getProperty("Power")), 0.0);

    MatrixWorkspace_sptr out_twice = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUT_WKSP2);
    auto &outX_twice = out_twice->x(0);

    // run again, with binmode set, but no power set
    Rebin rebin_thrice;
    rebin_thrice.initialize();
    rebin_thrice.setPropertyValue("InputWorkspace", IN_WKSP);
    rebin_thrice.setPropertyValue("OutputWorkspace", OUT_WKSP3);
    rebin_thrice.setProperty("Params", PARAMS);
    rebin_thrice.setProperty("Power", 0.0);
    rebin_thrice.setProperty("BinningMode", binMode);
    TS_ASSERT_THROWS_NOTHING(rebin_thrice.execute());

    MatrixWorkspace_sptr out_thrice = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(OUT_WKSP3);
    auto &outX_thrice = out_thrice->x(0);

    TS_ASSERT_EQUALS(outX_once.size(), outX_twice.size());
    TS_ASSERT_EQUALS(outX_once.size(), outX_thrice.size());
    for (size_t i = 0; i < outX_once.size(); i++) {
      TS_ASSERT_EQUALS(outX_once[i], outX_twice[i]);
      TS_ASSERT_EQUALS(outX_once[i], outX_thrice[i]);
    }

    AnalysisDataService::Instance().remove(IN_WKSP);
    AnalysisDataService::Instance().remove(OUT_WKSP1);
    AnalysisDataService::Instance().remove(OUT_WKSP2);
    AnalysisDataService::Instance().remove(OUT_WKSP3);
  }

  void test_all_bin_modes_ignore_power() {
    std::vector<std::string> binModes{"Linear", "Logarithmic", "ReverseLogarithmic"};
    std::vector<double> steps{1.0, -1.0, -1.0};

    for (size_t i = 0; i < binModes.size(); i++) {
      do_test_ignores_power(binModes[i], steps[i]);
    }
  }

  void test_histogram_unsorted_linear() { do_test_unsorted(false, "0.0,4.0,100"); }

  void test_histogram_unsorted_linear2() { do_test_unsorted(false, "4.0"); }

  void test_histogram_unsorted_varying_step() { do_test_unsorted(true, "0.0,2.0,50,4.0,100"); }

  void test_histogram_unsorted_log() { do_test_unsorted(false, "1.0,-1,100"); }

  void test_histogram_unsorted_reverse_log() { do_test_unsorted(true, "1.0,-1,100", true); }

  void test_histogram_unsorted_power() { do_test_unsorted(true, "1.0,0.5,100", false, 0.5); }

  void do_test_unsorted(bool expectSorted, std::string params, bool useReverseLogarithmic = false, double power = 0) {
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::createEventWorkspace2(50, 100);

    Rebin rebin;
    rebin.initialize();
    rebin.setProperty("InputWorkspace", test_in);
    rebin.setPropertyValue("OutputWorkspace", "output");
    rebin.setProperty("Params", params);
    rebin.setProperty("Power", power);
    rebin.setProperty("UseReverseLogarithmic", useReverseLogarithmic);
    rebin.setProperty("PreserveEvents", false);
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());

    TS_ASSERT_EQUALS(test_in->getSpectrum(0).isSortedByTof(), expectSorted);
  }

  void test_group_workspace_validation() {
    auto ws1 = Create1DWorkspace(10);
    auto ws2 = Create1DWorkspace(10);
    AnalysisDataService::Instance().add("ws1", ws1);
    AnalysisDataService::Instance().add("ws2", ws2);
    const std::string groupWsName = "group";
    GroupWorkspaces group;
    group.initialize();
    group.setProperty("InputWorkspaces", std::vector<std::string>{"ws1", "ws2"});
    group.setProperty("OutputWorkspace", groupWsName);
    group.execute();
    Rebin rebin;
    rebin.initialize();
    rebin.setProperty("InputWorkspace", groupWsName);
    rebin.setPropertyValue("OutputWorkspace", "output");
    rebin.setProperty("Params", "1,1,10");
    auto errors = rebin.validateInputs();
    TS_ASSERT_EQUALS(0, errors.size());
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    AnalysisDataService::Instance().remove("ws1");
    AnalysisDataService::Instance().remove("ws2");
  }

private:
  Workspace2D_sptr Create1DWorkspace(int size) {
    auto retVal = createWorkspace<Workspace2D>(1, size, size - 1);
    double j = 0.5;
    for (int i = 0; i < size; i++) {
      retVal->dataX(0)[i] = j;
      j += 0.75;
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

  void maskFirstBins(const std::string &in, const std::string &out, double maskBinsTo) {
    MaskBins mask;
    mask.initialize();
    mask.setPropertyValue("InputWorkspace", in);
    mask.setPropertyValue("OutputWorkspace", out);
    mask.setProperty("XMin", 0.0);
    mask.setProperty("XMax", maskBinsTo);
    mask.execute();
  }

  void do_test_FullBinsOnly(const std::string &params, const std::vector<double> &yExpected,
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

    auto ws = std::dynamic_pointer_cast<MatrixWorkspace>(outWsEntry.retrieve());

    if (!ws) {
      TS_FAIL("Unable to retrieve result workspace");
      return; // Nothing else to check
    }

    auto &xValues = ws->x(0);
    TS_ASSERT_DELTA(xValues.rawData(), xExpected, 0.001);

    auto &yValues = ws->y(0);
    TS_ASSERT_DELTA(yValues.rawData(), yExpected, 0.001);
  }

  enum class Monotonic : bool { INCREASE = false, DECREASE = true };
  bool checkBinWidthMonotonic(MatrixWorkspace_sptr ws, Monotonic monotonic = Monotonic::INCREASE,
                              bool ignoreLastBin = false) {
    size_t binEdgesTotal = ws->blocksize();
    if (ignoreLastBin)
      binEdgesTotal--;
    auto binEdges = ws->binEdges(0);

    std::function<bool(double, double)> checkBinSizeChange;
    switch (monotonic) {
    case Monotonic::INCREASE:
      checkBinSizeChange = [](double lastBinSize, double currentBinSize) { return currentBinSize > lastBinSize; };
      break;
    case Monotonic::DECREASE:
      checkBinSizeChange = [](double lastBinSize, double currentBinSize) { return currentBinSize < lastBinSize; };
      break;
    }

    double lastBinSize = binEdges[1] - binEdges[0];
    bool allMonotonic = true;
    for (size_t i = 1; i < binEdgesTotal && allMonotonic; i++) {
      double currentBinSize = binEdges[i + 1] - binEdges[i];
      allMonotonic &= checkBinSizeChange(lastBinSize, currentBinSize);
      lastBinSize = currentBinSize;
    }
    return allMonotonic;
  }
};

class RebinTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RebinTestPerformance *createSuite() { return new RebinTestPerformance(); }
  static void destroySuite(RebinTestPerformance *suite) { delete suite; }

  RebinTestPerformance() { ws = WorkspaceCreationHelper::create2DWorkspaceBinned(5000, 20000); }

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
