#ifndef PLOTASYMMETRYBYLOGVALUTEST_H_
#define PLOTASYMMETRYBYLOGVALUTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/PlotAsymmetryByLogValue.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/LoadMuonNexus.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/SaveNexus.h"

#include <iostream>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class PlotAsymmetryByLogValueTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotAsymmetryByLogValueTest *createSuite() {
    return new PlotAsymmetryByLogValueTest();
  }
  static void destroySuite(PlotAsymmetryByLogValueTest *suite) { delete suite; }

  PlotAsymmetryByLogValueTest()
      : firstRun("MUSR00015189.nxs"), lastRun("MUSR00015190.nxs") {}

  void testExec() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", "PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue", "Field_Danfysik");
    alg.setPropertyValue("Red", "2");
    alg.setPropertyValue("Green", "1");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(
            "PlotAsymmetryByLogValueTest_WS"));

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->blocksize(), 2);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    const Mantid::MantidVec &Y = outWS->readY(0);
    TS_ASSERT_DELTA(Y[0], 0.0128845, 0.001);
    TS_ASSERT_DELTA(Y[1], 0.0224898, 0.00001);

    const TextAxis *axis = dynamic_cast<const TextAxis *>(outWS->getAxis(1));
    TS_ASSERT(axis);
    if (axis) {
      TS_ASSERT_EQUALS(axis->length(), 4);
      TS_ASSERT_EQUALS(axis->label(0), "Red-Green");
      TS_ASSERT_EQUALS(axis->label(1), "Red");
      TS_ASSERT_EQUALS(axis->label(2), "Green");
      TS_ASSERT_EQUALS(axis->label(3), "Red+Green");
    }
    AnalysisDataService::Instance().clear();
  }

  void testDifferential() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", "PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue", "Field_Danfysik");
    alg.setPropertyValue("Red", "2");
    alg.setPropertyValue("Green", "1");
    alg.setPropertyValue("Type", "Differential");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(
            "PlotAsymmetryByLogValueTest_WS"));

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->blocksize(), 2);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    const Mantid::MantidVec &Y = outWS->readY(0);
    TS_ASSERT_DELTA(Y[0], -0.01236, 0.001);
    TS_ASSERT_DELTA(Y[1], 0.019186, 0.00001);

    AnalysisDataService::Instance().clear();
  }

  void test_int_log() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", "PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue", "nspectra");
    alg.setPropertyValue("Red", "2");
    alg.setPropertyValue("Green", "1");
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(
            "PlotAsymmetryByLogValueTest_WS"));

    TS_ASSERT(outWS);
    AnalysisDataService::Instance().clear();
  }

  void test_string_log() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", "PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue", "run_number");
    alg.setPropertyValue("Red", "2");
    alg.setPropertyValue("Green", "1");
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(
            "PlotAsymmetryByLogValueTest_WS"));

    TS_ASSERT(outWS);
    AnalysisDataService::Instance().clear();
  }

  void test_text_log() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", "PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue", "run_title");
    alg.setPropertyValue("Red", "2");
    alg.setPropertyValue("Green", "1");
    alg.execute();

    TS_ASSERT(!alg.isExecuted());

    AnalysisDataService::Instance().clear();
  }

  void test_DeadTimeCorrection_FromSpecifiedFile() {
    const std::string ws = "Ws";
    const std::string deadTimeWs = "DeadTimeWs";
    const std::string deadTimeFile = "TestDeadTimeFile.nxs";

    ITableWorkspace_sptr deadTimeTable =
        Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
    deadTimeTable->addColumn("int", "spectrum");
    deadTimeTable->addColumn("double", "dead-time");

    for (int i = 0; i < 64; i++) {
      TableRow row = deadTimeTable->appendRow();
      row << (i + 1) << 0.015;
    }

    AnalysisDataService::Instance().addOrReplace(deadTimeWs, deadTimeTable);

    // Save dead time table to file
    SaveNexus saveNexusAlg;
    TS_ASSERT_THROWS_NOTHING(saveNexusAlg.initialize());
    saveNexusAlg.setPropertyValue("InputWorkspace", deadTimeWs);
    saveNexusAlg.setPropertyValue("Filename", deadTimeFile);
    TS_ASSERT_THROWS_NOTHING(saveNexusAlg.execute());
    TS_ASSERT(saveNexusAlg.isExecuted());

    PlotAsymmetryByLogValue alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", ws);
    alg.setPropertyValue("LogValue", "run_number");
    alg.setPropertyValue("DeadTimeCorrType", "FromSpecifiedFile");
    alg.setPropertyValue("DeadTimeCorrFile", deadTimeFile);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    if (!alg.isExecuted())
      return;

    MatrixWorkspace_sptr outWs = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(ws));

    TS_ASSERT(outWs);
    TS_ASSERT_EQUALS(outWs->blocksize(), 2);
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), 1);

    const Mantid::MantidVec &Y = outWs->readY(0);

    TS_ASSERT_DELTA(Y[0], 0.15214, 0.00001);
    TS_ASSERT_DELTA(Y[1], 0.14492, 0.00001);

    AnalysisDataService::Instance().clear();
    Poco::File(deadTimeFile).remove();
  }

  void test_DeadTimeCorrection_FromRunData() {
    const std::string ws = "Test_DeadTimeCorrection_FromRunData_Ws";

    PlotAsymmetryByLogValue alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", ws);
    alg.setPropertyValue("LogValue", "run_number");
    alg.setPropertyValue("DeadTimeCorrType", "FromRunData");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    if (!alg.isExecuted())
      return;

    MatrixWorkspace_sptr outWs = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(ws));

    TS_ASSERT(outWs);
    TS_ASSERT_EQUALS(outWs->blocksize(), 2);
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), 1);

    const Mantid::MantidVec &Y = outWs->readY(0);

    TS_ASSERT_DELTA(Y[0], 0.151202, 0.00001);
    TS_ASSERT_DELTA(Y[1], 0.144008, 0.00001);

    AnalysisDataService::Instance().clear();
  }

  void test_customGrouping() {
    PlotAsymmetryByLogValue alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", "PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue", "run_number");
    alg.setPropertyValue("Red", "2");
    alg.setPropertyValue("Green", "1");
    alg.setPropertyValue("ForwardSpectra", "1-16,33-48");
    alg.setPropertyValue("BackwardSpectra", "17-32,49-64");
    alg.setPropertyValue("DeadTimeCorrType", "None");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    if (!alg.isExecuted())
      return;

    MatrixWorkspace_sptr outWs = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(
            "PlotAsymmetryByLogValueTest_WS"));

    TS_ASSERT(outWs);
    TS_ASSERT_EQUALS(outWs->blocksize(), 2);
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), 4);

    const Mantid::MantidVec &YDiff = outWs->readY(0);
    const Mantid::MantidVec &EDiff = outWs->readE(0);
    const Mantid::MantidVec &YSum = outWs->readY(3);
    const Mantid::MantidVec &ESum = outWs->readE(3);

    TS_ASSERT_DELTA(YDiff[0], 0.001135, 0.000001);
    TS_ASSERT_DELTA(EDiff[0], 0.001805, 0.000001);
    TS_ASSERT_DELTA(YDiff[1], -0.000151, 0.000001);
    TS_ASSERT_DELTA(EDiff[1], 0.001806, 0.000001);

    TS_ASSERT_DELTA(YSum[0], 0.170842, 0.000001);
    TS_ASSERT_DELTA(ESum[0], 0.001805, 0.000001);
    TS_ASSERT_DELTA(YSum[1], 0.171467, 0.000001);
    TS_ASSERT_DELTA(ESum[1], 0.001806, 0.000001);

    AnalysisDataService::Instance().clear();
  }

  void test_customTimeLimits() {
    const std::string ws = "Test_customTimeLimits";

    PlotAsymmetryByLogValue alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", ws);
    alg.setPropertyValue("LogValue", "run_number");
    alg.setPropertyValue("TimeMin", "0.5");
    alg.setPropertyValue("TimeMax", "0.6");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWs = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(ws));

    TS_ASSERT(outWs);
    TS_ASSERT_EQUALS(outWs->blocksize(), 2);
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), 1);

    const Mantid::MantidVec &Y = outWs->readY(0);
    TS_ASSERT_DELTA(Y[0], 0.14700, 0.00001);
    TS_ASSERT_DELTA(Y[1], 0.13042, 0.00001);

    AnalysisDataService::Instance().clear();
  }

  void test_LogValueFunction() {
    const std::string ws = "Test_LogValueFunction";

    PlotAsymmetryByLogValue alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", ws);
    // We use 'beamlog_current' as log value because
    // we want to test the 'Mean' function below and this is
    // one of the few properties that contains different values over time
    alg.setPropertyValue("LogValue", "beamlog_current");
    alg.setPropertyValue("Function", "Mean");
    alg.setPropertyValue("DeadTimeCorrType", "None");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWs = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(ws));

    TS_ASSERT(outWs);
    TS_ASSERT_EQUALS(outWs->blocksize(), 2);
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), 1);

    // Now we want to test X values (log values) in the output workspace
    // rather than asymmetry (Y values)
    const Mantid::MantidVec &X = outWs->readX(0);

    TS_ASSERT_DELTA(X[0], 178.740476, 0.00001);
    TS_ASSERT_DELTA(X[1], 178.849998, 0.00001);

    AnalysisDataService::Instance().clear();
  }

  void test_invalidRunNumbers() {
    const std::string ws = "Test_LogValueFunction";

    PlotAsymmetryByLogValue alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("FirstRun", lastRun);
    alg.setPropertyValue("LastRun", firstRun);
    alg.setPropertyValue("OutputWorkspace", ws);

    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);
    TS_ASSERT(!alg.isExecuted());

    AnalysisDataService::Instance().clear();
  }

  void test_singlePeriodGreen() {
    // Load a single-period dataset and set the green period to a
    // number. The algorithm should ignore the supplied green and/or red periods
    // as the input nexus file is single-period
    const std::string ws = "Test_singlePeriodGreen";

    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("FirstRun", "emu00006473.nxs");
    alg.setPropertyValue("LastRun", "emu00006473.nxs");
    alg.setPropertyValue("OutputWorkspace", ws);
    alg.setPropertyValue("LogValue", "run_number");
    alg.setPropertyValue("Red", "3");
    alg.setPropertyValue("Green", "1");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve(ws));

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->blocksize(), 1);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 1);

    TS_ASSERT_EQUALS(outWS->readX(0)[0], 6473);
    TS_ASSERT_DELTA(outWS->readY(0)[0], 0.283444, 0.000001);
    TS_ASSERT_DELTA(outWS->readE(0)[0], 0.000145, 0.000001);

    AnalysisDataService::Instance().clear();
  }

private:
  std::string firstRun, lastRun;
};

#endif /*PLOTASYMMETRYBYLOGVALUTEST_H_*/
