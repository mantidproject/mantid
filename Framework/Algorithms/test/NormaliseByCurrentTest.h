// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef NORMALISEBYCURRENTTEST_H_
#define NORMALISEBYCURRENTTEST_H_

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/NormaliseByCurrent.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

namespace {
MatrixWorkspace_const_sptr doTest(MatrixWorkspace_sptr inWS,
                                  std::string wsNameOut, double expectedY,
                                  double expectedE, bool calcPcharge = false,
                                  bool performance = false) {
  NormaliseByCurrent norm1;
  if (!norm1.isInitialized())
    norm1.initialize();

  const auto &Y = inWS->y(0);
  double initValue = Y[0];
  bool checkNormFactor = true;
  if (initValue <= 0) {
    checkNormFactor = false;
  }
  double normFactor = initValue / expectedY;

  TS_ASSERT_THROWS_NOTHING(norm1.setProperty("InputWorkspace", inWS));
  TS_ASSERT_THROWS_NOTHING(
      norm1.setPropertyValue("OutputWorkspace", wsNameOut));
  TS_ASSERT_THROWS_NOTHING(
      norm1.setProperty("RecalculatePCharge", calcPcharge));

  TS_ASSERT_THROWS_NOTHING(norm1.execute());
  TS_ASSERT(norm1.isExecuted());

  MatrixWorkspace_const_sptr output;
  TS_ASSERT_THROWS_NOTHING(
      output =
          AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(
              wsNameOut));

  if (!performance) {

    for (size_t i = 0; i < output->getNumberHistograms(); i++) {
      const auto &inX = inWS->x(i);
      const auto &X = output->x(i);
      const auto &Y = output->y(i);
      const auto &E = output->e(i);
      for (size_t j = 0; j < Y.size(); j++) {
        TS_ASSERT_EQUALS(X[j], inX[j]);
        TS_ASSERT_EQUALS(Y[j], expectedY);
        TS_ASSERT_DELTA(E[j], expectedE, 1e-5);
      }
    }

    TS_ASSERT_EQUALS(output->YUnit(), "Counts");
    TS_ASSERT_EQUALS(output->YUnitLabel(), "Counts per microAmp.hour");
    Kernel::Property *normLog(nullptr);
    TS_ASSERT_THROWS_NOTHING(
        normLog = output->run().getProperty("NormalizationFactor"));
    Kernel::PropertyWithValue<double> *pFactor =
        dynamic_cast<Kernel::PropertyWithValue<double> *>(normLog);
    TS_ASSERT(pFactor);

    if (checkNormFactor) {
      double realFactor = (*pFactor)();
      TS_ASSERT_DELTA(realFactor, normFactor, 1.e-5);
    }
  }
  return output;
}

MatrixWorkspace_const_sptr doTest(std::string wsNameIn, std::string wsNameOut,
                                  const double pCharge, double expectedY,
                                  double expectedE, bool performance = false) {
  MatrixWorkspace_sptr inWS =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsNameIn);

  // Now set the charge
  inWS->mutableRun().setProtonCharge(pCharge);
  inWS->getAxis(0)->unit() =
      Mantid::Kernel::UnitFactory::Instance().create("TOF");
  inWS->setYUnit("Counts");

  return doTest(inWS, wsNameOut, expectedY, expectedE, true, performance);
}

/// Helper method to add necessary log values to simulate multi-period data.
/// The algorithm uses these logs to determien how to normalise by the
/// current.
void addMultiPeriodLogsTo(MatrixWorkspace_sptr ws, int period,
                          const std::string &protonCharges) {
  ArrayProperty<double> *chargeProp =
      new ArrayProperty<double>("proton_charge_by_period", protonCharges);
  PropertyWithValue<int> *nperiodsProp =
      new PropertyWithValue<int>("nperiods", 3);
  PropertyWithValue<int> *currentPeriodProp =
      new PropertyWithValue<int>("current_period", period);

  ws->mutableRun().addLogData(chargeProp);
  ws->mutableRun().addLogData(nperiodsProp);
  ws->mutableRun().addLogData(currentPeriodProp);
}

void addPChargeLogTo(MatrixWorkspace_sptr ws, const double pChargeAccum) {
  auto pchargeLog =
      Kernel::make_unique<Kernel::TimeSeriesProperty<double>>("proton_charge");

  const Types::Core::DateAndTime runstart(20000000000);
  const int64_t pulsedt = 100 * 1000 * 1000;
  const size_t numpulses = 100;
  const double pCharge = pChargeAccum / static_cast<double>(numpulses);

  for (int64_t pid = 0; pid < static_cast<int64_t>(numpulses); pid++) {
    const int64_t pulsetime_i64 = pulsedt + runstart.totalNanoseconds();
    const Types::Core::DateAndTime pulsetime(pulsetime_i64);
    pchargeLog->addValue(pulsetime, pCharge);
  } // FOR each pulse

  ws->mutableRun().addLogData(pchargeLog.release());
  // ws->mutableRun().integrateProtonCharge(); // TODO
}
} // namespace
class NormaliseByCurrentTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NormaliseByCurrentTest *createSuite() {
    return new NormaliseByCurrentTest();
  }
  static void destroySuite(NormaliseByCurrentTest *suite) { delete suite; }

  void test_name() { TS_ASSERT_EQUALS(norm.name(), "NormaliseByCurrent"); }

  void test_version() { TS_ASSERT_EQUALS(norm.version(), 1); }

  void test_init() {
    TS_ASSERT_THROWS_NOTHING(norm.initialize());
    TS_ASSERT(norm.isInitialized());
  }

  void test_notInitialized() {
    if (!norm.isInitialized())
      norm.initialize();

    // Check it fails if properties haven't been set
    TS_ASSERT_THROWS(norm.execute(), const std::runtime_error &);
    TS_ASSERT(!norm.isExecuted());
  }

  void test_exec() {
    AnalysisDataService::Instance().add(
        "normIn", WorkspaceCreationHelper::create2DWorkspaceBinned(10, 3, 1));
    doTest("normIn", "normOut", 2.0, 1.0, 0.5 * M_SQRT2);
    AnalysisDataService::Instance().remove("normIn");
    AnalysisDataService::Instance().remove("normOut");
  }

  void test_multiPeriodData() {
    const std::string protonChargeByPeriod = "2.0, 4.0, 8.0";

    // Note that CreateWorkspace123 creates uniform signal value of 2.0, and
    // uniform error value of 3.0.

    MatrixWorkspace_sptr a =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    a->setYUnit("Counts");
    addMultiPeriodLogsTo(a, 1, protonChargeByPeriod);

    MatrixWorkspace_sptr b =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    b->setYUnit("Counts");
    addMultiPeriodLogsTo(b, 2, protonChargeByPeriod);

    MatrixWorkspace_sptr c =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    c->setYUnit("Counts");
    addMultiPeriodLogsTo(c, 3, protonChargeByPeriod);

    // Check that normalisation has used the protonChargeByPeriod data. i.e Yout
    // = Yin/2.0, Yout = Yin/4.0, ... for each period workspace.
    doTest(a, "period1", 1.00, 1.500); // 2/2, 3/2
    doTest(b, "period2", 0.50, 0.750); // 2/4, 3/4
    doTest(c, "period3", 0.25, 0.375); // 2/8, 3/8
  }

  void testTreatAsSinglePeriodWithoutNPERIODS_Log() {
    const std::string protonChargeByPeriod = "2.0, 4.0, 8.0";

    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    ws->setYUnit("Counts");
    addMultiPeriodLogsTo(ws, 1, protonChargeByPeriod); // If this worked, we
                                                       // would be normalising
                                                       // by a charge of 2.0
    ws->mutableRun().setProtonCharge(10); // This is what will be used when the
                                          // period information is deemed
                                          // unavailable
    ws->mutableRun().removeLogData("nperiods"); // nperiods data is now gone!

    doTest(ws, "period1", 0.2, 0.3); // 2/10, 3/10.
  }

  void testTreatAsSinglePeriodWithOnlyOneInNPERIODS_Log() {
    const std::string protonChargeByPeriod = "2.0, 4.0, 8.0";

    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    ws->setYUnit("Counts");
    addMultiPeriodLogsTo(ws, 1, protonChargeByPeriod); // If this worked, we
                                                       // would be normalising
                                                       // by a charge of 2.0
    ws->mutableRun().setProtonCharge(10); // This is what will be used when the
                                          // period information is deemed
                                          // unavailable
    ws->mutableRun()
        .getLogData("nperiods")
        ->setValue("1"); // nperiods now indicates SINGLE-period data.
    doTest(ws, "period1", 0.2, 0.3); // 2/10, 3/10.
  }

  void testTreatAsMultiPeriodWithMoreThanOneInNPERIODS_Log() {
    const std::string protonChargeByPeriod = "2.0, 4.0";

    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    ws->setYUnit("Counts");
    addMultiPeriodLogsTo(ws, 1, protonChargeByPeriod); // If this worked, we
                                                       // would be normalising
                                                       // by a charge of 2.0
    ws->mutableRun().setProtonCharge(10); // This is what will be used when the
                                          // period information is deemed
                                          // unavailable
    ws->mutableRun()
        .getLogData("nperiods")
        ->setValue("2");           // nperiods now indicates MULTI-period data.
    doTest(ws, "period1", 1, 1.5); // 2/2, 3/2.
  }

  void testThrowsWithoutCURRENT_PERIOD_Log() {
    const std::string protonChargeByPeriod = "2.0, 4.0, 8.0";
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    ws->setYUnit("Counts");
    addMultiPeriodLogsTo(ws, 1, protonChargeByPeriod); // If this worked, we
                                                       // would be normalising
                                                       // by a charge of 2.0
    ws->mutableRun().removeLogData(
        "current_period"); // current_period log data is now gone!

    NormaliseByCurrent alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "testws");
    TSM_ASSERT("Should not execute to completion without current_period if "
               "it's supposed to be multiperiod data.",
               !alg.execute());
    TSM_ASSERT("Should not execute to completion without current_period if "
               "it's supposed to be multiperiod data.",
               !alg.isExecuted());
  }

  void testThrowsWithoutPROTON_CHARGE_BY_PERIOD_Log() {
    const std::string protonChargeByPeriod = "2.0, 4.0, 8.0";
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    ws->setYUnit("Counts");
    addMultiPeriodLogsTo(ws, 1, protonChargeByPeriod); // If this worked, we
                                                       // would be normalising
                                                       // by a charge of 2.0
    ws->mutableRun().removeLogData(
        "proton_charge_by_period"); // proton_charge_by_period log data is now
                                    // gone!

    NormaliseByCurrent alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "testws");
    TSM_ASSERT("Should not execute to completion without "
               "proton_charge_by_period if it's supposed to be multiperiod "
               "data.",
               !alg.execute());
    TSM_ASSERT("Should not execute to completion without "
               "proton_charge_by_period if it's supposed to be multiperiod "
               "data.",
               !alg.isExecuted());
  }

  void test_execInPlace() {
    AnalysisDataService::Instance().add(
        "normIn", WorkspaceCreationHelper::create2DWorkspaceBinned(10, 3, 1));
    doTest("normIn", "normIn", 2.0, 1.0, 0.5 * M_SQRT2);
    AnalysisDataService::Instance().remove("normIn");
  }

  void test_execEvent() {
    AnalysisDataService::Instance().add(
        "normInEvent",
        WorkspaceCreationHelper::createEventWorkspace(10, 3, 100, 0.0, 1.0, 2));

    EventWorkspace_const_sptr outputEvent;
    outputEvent = boost::dynamic_pointer_cast<const EventWorkspace>(
        doTest("normInEvent", "normOutEvent", 2.0, 1.0, 0.5 * M_SQRT2));
    // Output is an event workspace
    TS_ASSERT(outputEvent);

    AnalysisDataService::Instance().remove("normInEvent");
    AnalysisDataService::Instance().remove("normOutEvent");
  }

  void test_execEventFunnyPCharge() {
    auto wksp =
        WorkspaceCreationHelper::createEventWorkspace(10, 3, 100, 0.0, 1.0, 2);
    addPChargeLogTo(wksp, 2.); // pcharge intentionally doesn't match
    AnalysisDataService::Instance().add("normInEvent", wksp);

    // intentionally set the wrong `gd_prtn_chrg` to stress getting the right
    // answer
    EventWorkspace_const_sptr outputEvent;
    outputEvent = boost::dynamic_pointer_cast<const EventWorkspace>(
        doTest("normInEvent", "normOutEvent", 100.0, 1.0, 0.5 * M_SQRT2));
    // Output is an event workspace
    TS_ASSERT(outputEvent);

    AnalysisDataService::Instance().remove("normInEvent");
    AnalysisDataService::Instance().remove("normOutEvent");
  }

  void test_execEventInPlace() {
    AnalysisDataService::Instance().add(
        "normInEvent",
        WorkspaceCreationHelper::createEventWorkspace(10, 3, 100, 0.0, 1.0, 2));

    EventWorkspace_const_sptr outputEvent;
    outputEvent = boost::dynamic_pointer_cast<const EventWorkspace>(
        doTest("normInEvent", "normInEvent", 2.0, 1.0, 0.5 * M_SQRT2));
    // Output is an event workspace
    TS_ASSERT(outputEvent);

    AnalysisDataService::Instance().remove("normInEvent");
  }

  void test_execZero() {
    AnalysisDataService::Instance().add(
        "normIn", WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1));

    NormaliseByCurrent norm1;
    norm1.initialize();

    TS_ASSERT_THROWS_NOTHING(
        norm1.setPropertyValue("InputWorkspace", "normIn"));
    TS_ASSERT_THROWS_NOTHING(
        norm1.setPropertyValue("OutputWorkspace", "normOut"));

    // Set the charge to zero
    MatrixWorkspace_sptr input =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("normIn");
    input->mutableRun().setProtonCharge(0.0);
    input->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("TOF");
    input->setYUnit("Counts");

    TS_ASSERT_THROWS_NOTHING(norm1.execute());
    TS_ASSERT(!norm1.isExecuted());

    AnalysisDataService::Instance().remove("normIn");
    AnalysisDataService::Instance().remove("normOut");
  }

private:
  NormaliseByCurrent norm;
};

class NormaliseByCurrentTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NormaliseByCurrentTestPerformance *createSuite() {
    return new NormaliseByCurrentTestPerformance();
  }
  static void destroySuite(NormaliseByCurrentTestPerformance *suite) {
    delete suite;
  }

  /// Set up all the test workspaces
  NormaliseByCurrentTestPerformance(int nHist = 1000, int nBins = 3000,
                                    int nPixels = 200) {
    // test_execPerformance
    AnalysisDataService::Instance().add(
        execWSIn,
        WorkspaceCreationHelper::create2DWorkspaceBinned(nHist, nBins, 1));

    // test_execInPlacePerformance
    AnalysisDataService::Instance().add(
        execInPlaceWSIn,
        WorkspaceCreationHelper::create2DWorkspaceBinned(nHist, nBins, 1));

    // test_execEventPerformance
    AnalysisDataService::Instance().add(
        execEventWSIn, WorkspaceCreationHelper::createEventWorkspace(
                           nHist, nBins, nPixels, 0.0, 1.0, 2));

    // test_execEventInPlacePerformance
    AnalysisDataService::Instance().add(
        execEventInPlaceWSIn, WorkspaceCreationHelper::createEventWorkspace(
                                  nHist, nBins, nPixels, 0.0, 1.0, 2));

    // test_multiPeriodDataPerformance
    const std::string protonChargeByPeriod = "2.0, 4.0, 8.0";

    // Note that CreateWorkspace123 creates uniform signal value of 2.0, and
    // uniform error value of 3.0.
    multiPeriodWS1 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins, 1);
    multiPeriodWS1->setYUnit("Counts");
    addMultiPeriodLogsTo(multiPeriodWS1, 1, protonChargeByPeriod);

    multiPeriodWS2 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins, 1);
    multiPeriodWS2->setYUnit("Counts");
    addMultiPeriodLogsTo(multiPeriodWS2, 2, protonChargeByPeriod);

    multiPeriodWS3 =
        WorkspaceCreationHelper::create2DWorkspace123(nHist, nBins, 1);
    multiPeriodWS3->setYUnit("Counts");
    addMultiPeriodLogsTo(multiPeriodWS3, 3, protonChargeByPeriod);
  }

  ~NormaliseByCurrentTestPerformance() {
    // test_execPerformance
    AnalysisDataService::Instance().remove(execWSIn);
    AnalysisDataService::Instance().remove(execWSOut);

    // test_execInPlacePerformance
    AnalysisDataService::Instance().remove(execInPlaceWSIn);

    // test_execEventPerformance
    AnalysisDataService::Instance().remove(execEventWSIn);
    AnalysisDataService::Instance().remove(execEventWSOut);

    // test_execEventInPlacePerformance
    AnalysisDataService::Instance().remove(execEventInPlaceWSIn);
  }

  void test_execPerformance() {
    doTest(execWSIn, execWSOut, 1.0, 0.5 * M_SQRT2, false, performance);
  }
  void test_execInPlacePerformance() {
    doTest(execInPlaceWSIn, execInPlaceWSIn, 1.0, 0.5 * M_SQRT2, false,
           performance);
  }

  void test_execEventPerformance() {
    EventWorkspace_const_sptr outputEvent;
    outputEvent = boost::dynamic_pointer_cast<const EventWorkspace>(doTest(
        execEventWSIn, execEventWSOut, 1.0, 0.5 * M_SQRT2, false, performance));
  }
  void test_execEventInPlacePerformance() {
    EventWorkspace_const_sptr outputEvent;
    outputEvent = boost::dynamic_pointer_cast<const EventWorkspace>(
        doTest(execEventInPlaceWSIn, execEventInPlaceWSIn, 1.0, 0.5 * M_SQRT2,
               false, performance));
  }
  void test_multiPeriodDataPerformance() {

    // Check that normalisation has used the protonChargeByPeriod data. i.e Yout
    // = Yin/2.0, Yout = Yin/4.0, ... for each period workspace.
    doTest(multiPeriodWS1, "period1", 1.00, 1.500, false,
           performance); // 2/2, 3/2
    doTest(multiPeriodWS2, "period2", 0.50, 0.750, false,
           performance); // 2/4, 3/4
    doTest(multiPeriodWS3, "period3", 0.25, 0.375, false,
           performance); // 2/8, 3/8
  }

private:
  const std::string execWSIn = "execWSIn";
  const std::string execWSOut = "execWSOut";
  const std::string execInPlaceWSIn = "execInPlaceWSIn";
  const std::string execEventWSIn = "execEventWSIn";
  const std::string execEventWSOut = "execEventWSOut";
  const std::string execEventInPlaceWSIn = "execEventInPlaceWSIn";

  MatrixWorkspace_sptr multiPeriodWS1;
  MatrixWorkspace_sptr multiPeriodWS2;
  MatrixWorkspace_sptr multiPeriodWS3;

  const bool performance = true;
};
#endif /*NORMALISEBYCURRENTTEST_H_*/
