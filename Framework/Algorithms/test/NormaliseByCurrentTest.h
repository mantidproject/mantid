#ifndef NORMALISEBYCURRENTTEST_H_
#define NORMALISEBYCURRENTTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/NormaliseByCurrent.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class NormaliseByCurrentTest : public CxxTest::TestSuite {
private:
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

public:
  void testName() { TS_ASSERT_EQUALS(norm.name(), "NormaliseByCurrent"); }

  void testVersion() { TS_ASSERT_EQUALS(norm.version(), 1); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(norm.initialize());
    TS_ASSERT(norm.isInitialized());
  }

  void testNotInitialized() {
    if (!norm.isInitialized())
      norm.initialize();

    // Check it fails if properties haven't been set
    TS_ASSERT_THROWS(norm.execute(), std::runtime_error);
    TS_ASSERT(!norm.isExecuted());
  }

  MatrixWorkspace_const_sptr doTest(MatrixWorkspace_sptr inWS,
                                    std::string wsNameOut, double expectedY,
                                    double expectedE) {
    NormaliseByCurrent norm1;
    if (!norm1.isInitialized())
      norm1.initialize();

    const MantidVec &Y = inWS->readY(0);
    double initValue = Y[0];
    bool checkNormFactor = true;
    if (initValue <= 0) {
      checkNormFactor = false;
    }
    double normFactor = initValue / expectedY;

    TS_ASSERT_THROWS_NOTHING(norm1.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING(
        norm1.setPropertyValue("OutputWorkspace", wsNameOut));

    TS_ASSERT_THROWS_NOTHING(norm1.execute());
    TS_ASSERT(norm1.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output =
            AnalysisDataService::Instance().retrieveWS<const MatrixWorkspace>(
                wsNameOut));

    for (size_t i = 0; i < output->getNumberHistograms(); i++) {
      const MantidVec &inX = inWS->readX(i);
      const MantidVec &X = output->readX(i);
      const MantidVec &Y = output->dataY(i);
      const MantidVec &E = output->dataE(i);
      for (size_t j = 0; j < Y.size(); j++) {
        TS_ASSERT_EQUALS(X[j], inX[j]);
        TS_ASSERT_EQUALS(Y[j], expectedY);
        TS_ASSERT_DELTA(E[j], expectedE, 1e-5);
      }
    }

    TS_ASSERT_EQUALS(output->YUnit(), "Counts");
    TS_ASSERT_EQUALS(output->YUnitLabel(), "Counts per microAmp.hour");
    Kernel::Property *normLog(NULL);
    TS_ASSERT_THROWS_NOTHING(
        normLog = output->run().getProperty("NormalizationFactor"));
    Kernel::PropertyWithValue<double> *pFactor =
        dynamic_cast<Kernel::PropertyWithValue<double> *>(normLog);
    TS_ASSERT(pFactor);

    if (checkNormFactor) {
      double realFactor = (*pFactor)();
      TS_ASSERT_DELTA(realFactor, normFactor, 1.e-5);
    }

    return output;
  }

  MatrixWorkspace_const_sptr doTest(std::string wsNameIn, std::string wsNameOut,
                                    double expectedY, double expectedE) {
    MatrixWorkspace_sptr inWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsNameIn);

    // Now set the charge
    inWS->mutableRun().setProtonCharge(2.0);
    inWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("TOF");
    inWS->setYUnit("Counts");

    return doTest(inWS, wsNameOut, expectedY, expectedE);
  }

  void testExec() {
    AnalysisDataService::Instance().add(
        "normIn", WorkspaceCreationHelper::Create2DWorkspaceBinned(10, 3, 1));
    doTest("normIn", "normOut", 1.0, 0.5 * M_SQRT2);
    AnalysisDataService::Instance().remove("normIn");
    AnalysisDataService::Instance().remove("normOut");
  }

  void testMultiPeriodData() {
    const std::string protonChargeByPeriod = "2.0, 4.0, 8.0";

    // Note that CreateWorkspace123 creates uniform signal value of 2.0, and
    // uniform error value of 3.0.

    MatrixWorkspace_sptr a =
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
    a->setYUnit("Counts");
    addMultiPeriodLogsTo(a, 1, protonChargeByPeriod);

    MatrixWorkspace_sptr b =
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
    b->setYUnit("Counts");
    addMultiPeriodLogsTo(b, 2, protonChargeByPeriod);

    MatrixWorkspace_sptr c =
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
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
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
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
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
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
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
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
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
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
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
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

  void testExec_InPlace() {
    AnalysisDataService::Instance().add(
        "normIn", WorkspaceCreationHelper::Create2DWorkspaceBinned(10, 3, 1));
    doTest("normIn", "normIn", 1.0, 0.5 * M_SQRT2);
    AnalysisDataService::Instance().remove("normIn");
  }

  void testExecEvent() {
    AnalysisDataService::Instance().add(
        "normInEvent",
        WorkspaceCreationHelper::CreateEventWorkspace(10, 3, 100, 0.0, 1.0, 2));

    EventWorkspace_const_sptr outputEvent;
    outputEvent = boost::dynamic_pointer_cast<const EventWorkspace>(
        doTest("normInEvent", "normOutEvent", 1.0, 0.5 * M_SQRT2));
    // Output is an event workspace
    TS_ASSERT(outputEvent);

    AnalysisDataService::Instance().remove("normInEvent");
    AnalysisDataService::Instance().remove("normOutEvent");
  }

  void testExecEvent_InPlace() {
    AnalysisDataService::Instance().add(
        "normInEvent",
        WorkspaceCreationHelper::CreateEventWorkspace(10, 3, 100, 0.0, 1.0, 2));

    EventWorkspace_const_sptr outputEvent;
    outputEvent = boost::dynamic_pointer_cast<const EventWorkspace>(
        doTest("normInEvent", "normInEvent", 1.0, 0.5 * M_SQRT2));
    // Output is an event workspace
    TS_ASSERT(outputEvent);

    AnalysisDataService::Instance().remove("normInEvent");
  }

  void testExecZero() {
    AnalysisDataService::Instance().add(
        "normIn", WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1));

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

#endif /*NORMALISEBYCURRENTTEST_H_*/
