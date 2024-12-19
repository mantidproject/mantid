// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/SaveNexus.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidMuon/PlotAsymmetryByLogValue.h"
#include <cxxtest/TestSuite.h>

#include <Poco/File.h>
#include <Poco/NObserver.h>
#include <Poco/TemporaryFile.h>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

/// RAII class to temporarily rename a file for the duration of a test
/// Original name is restored on destruction.
class TemporaryRenamer {
public:
  /// Constructor: rename the file and store its original name
  explicit TemporaryRenamer(const std::string &fileName) : m_originalName(fileName) {
    try {
      Poco::File file(m_originalName);
      TS_ASSERT(file.exists() && file.canWrite() && file.isFile());
      m_tempName = Poco::TemporaryFile::tempName();
      file.copyTo(m_tempName);
      file.remove();
    } catch (const Poco::FileException &ex) {
      failCopyWithError(m_originalName, m_tempName, ex);
    }
  }
  /// Destructor: restore the file's original name
  ~TemporaryRenamer() {
    try {
      Poco::File file(m_tempName);
      file.copyTo(m_originalName);
      file.remove();
    } catch (const Poco::FileException &ex) { // Do not throw in the destructor!
      failCopyWithError(m_tempName, m_originalName, ex);
    }
  }
  /// Fail with an error
  void failCopyWithError(const std::string &from, const std::string &to, const Poco::FileException &error) const {
    std::ostringstream message;
    message << "Failed to copy " << from << " to " << to << ": " << error.displayText();
    TS_FAIL(message.str());
  }

private:
  const std::string m_originalName;
  std::string m_tempName;
};

/// Class to count number of progress reports given out by an algorithm
class ProgressWatcher {
public:
  /// Constructor
  ProgressWatcher() : m_loadedCount(0), m_foundCount(0), m_observer(*this, &ProgressWatcher::handleProgress) {}
  /// Add a notification to the count
  void handleProgress(const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification> &notification) {
    const auto &message = notification->message;
    if (0 == message.compare(0, 5, "Found")) {
      ++m_foundCount;
    } else if (0 == message.compare(0, 6, "Loaded")) {
      ++m_loadedCount;
    }
  }
  /// Return the number of "found" progress reports seen so far
  size_t getFoundCount() { return m_foundCount; }
  /// Return the number of "loaded" progress reports seen so far
  size_t getLoadedCount() { return m_loadedCount; }
  /// Getter for the observer
  Poco::NObserver<ProgressWatcher, Mantid::API::Algorithm::ProgressNotification> getObserver() { return m_observer; }

private:
  /// Count of "file loaded" progress reports seen so far
  size_t m_loadedCount;
  /// Count of "file found" progress reports seen so far
  size_t m_foundCount;
  /// Observer
  Poco::NObserver<ProgressWatcher, Mantid::API::Algorithm::ProgressNotification> m_observer;
};

class PlotAsymmetryByLogValueTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotAsymmetryByLogValueTest *createSuite() { return new PlotAsymmetryByLogValueTest(); }
  static void destroySuite(PlotAsymmetryByLogValueTest *suite) { delete suite; }
  PlotAsymmetryByLogValueTest() : firstRun("MUSR00015189.nxs"), lastRun("MUSR00015190.nxs") {}

  /// Clear the ADS at the end of every test
  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_exec_with_first_and_last() {
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

    MatrixWorkspace_sptr outWS = std::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("PlotAsymmetryByLogValueTest_WS"));

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->blocksize(), 2);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    const auto &Y = outWS->y(0);
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
  }

  void test_exec_with_workspacenames() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    const std::vector<std::string> names{firstRun, lastRun};

    alg.setProperty("WorkspaceNames", names);
    alg.setPropertyValue("OutputWorkspace", "PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue", "Field_Danfysik");
    alg.setPropertyValue("Red", "2");
    alg.setPropertyValue("Green", "1");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS = std::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("PlotAsymmetryByLogValueTest_WS"));

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->blocksize(), 2);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    const auto &Y = outWS->y(0);
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

    MatrixWorkspace_sptr outWS = std::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("PlotAsymmetryByLogValueTest_WS"));

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->blocksize(), 2);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 4);
    const auto &Y = outWS->y(0);
    TS_ASSERT_DELTA(Y[0], -0.01236, 0.001);
    TS_ASSERT_DELTA(Y[1], 0.019186, 0.00001);
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

    MatrixWorkspace_sptr outWS = std::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("PlotAsymmetryByLogValueTest_WS"));

    TS_ASSERT(outWS);
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

    MatrixWorkspace_sptr outWS = std::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("PlotAsymmetryByLogValueTest_WS"));

    TS_ASSERT(outWS);
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
  }

  void test_DeadTimeCorrection_FromSpecifiedFile() {
    const std::string ws = "Ws";
    const std::string deadTimeWs = "DeadTimeWs";
    const std::string deadTimeFile = "TestDeadTimeFile.nxs";

    ITableWorkspace_sptr deadTimeTable = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
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

    MatrixWorkspace_sptr outWs =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(ws));

    TS_ASSERT(outWs);
    TS_ASSERT_EQUALS(outWs->blocksize(), 2);
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), 1);

    const auto &Y = outWs->y(0);

    TS_ASSERT_DELTA(Y[0], 0.15214, 0.00001);
    TS_ASSERT_DELTA(Y[1], 0.14492, 0.00001);
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

    MatrixWorkspace_sptr outWs =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(ws));

    TS_ASSERT(outWs);
    TS_ASSERT_EQUALS(outWs->blocksize(), 2);
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), 1);

    const auto &Y = outWs->y(0);

    TS_ASSERT_DELTA(Y[0], 0.151202, 0.00001);
    TS_ASSERT_DELTA(Y[1], 0.144008, 0.00001);
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

    MatrixWorkspace_sptr outWs = std::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("PlotAsymmetryByLogValueTest_WS"));

    TS_ASSERT(outWs);
    TS_ASSERT_EQUALS(outWs->blocksize(), 2);
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), 4);

    const auto &YDiff = outWs->y(0);
    const auto &EDiff = outWs->e(0);
    const auto &YSum = outWs->y(3);
    const auto &ESum = outWs->e(3);

    TS_ASSERT_DELTA(YDiff[0], 0.001135, 0.000001);
    TS_ASSERT_DELTA(EDiff[0], 0.001805, 0.000001);
    TS_ASSERT_DELTA(YDiff[1], -0.000151, 0.000001);
    TS_ASSERT_DELTA(EDiff[1], 0.001806, 0.000001);

    TS_ASSERT_DELTA(YSum[0], 0.170842, 0.000001);
    TS_ASSERT_DELTA(ESum[0], 0.001805, 0.000001);
    TS_ASSERT_DELTA(YSum[1], 0.171467, 0.000001);
    TS_ASSERT_DELTA(ESum[1], 0.001806, 0.000001);
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

    MatrixWorkspace_sptr outWs =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(ws));

    TS_ASSERT(outWs);
    TS_ASSERT_EQUALS(outWs->blocksize(), 2);
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), 1);

    const auto &Y = outWs->y(0);
    TS_ASSERT_DELTA(Y[0], 0.14700, 0.00001);
    TS_ASSERT_DELTA(Y[1], 0.13042, 0.00001);
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

    MatrixWorkspace_sptr outWs =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(ws));

    TS_ASSERT(outWs);
    TS_ASSERT_EQUALS(outWs->blocksize(), 2);
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), 1);

    // Now we want to test X values (log values) in the output workspace
    // rather than asymmetry (Y values)
    const auto &X = outWs->x(0);
    // The correct 'Mean' calculated by hand. The actual duration of the values
    // during the runs are 101 seconds and 96 seconds, respectively.
    TS_ASSERT_DELTA(X[0], 178.66634, 1e-5);
    TS_ASSERT_DELTA(X[1], 179.24375, 1e-5);
  }

  void test_invalidRunNumbers() {
    const std::string ws = "Test_LogValueFunction";

    PlotAsymmetryByLogValue alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("FirstRun", lastRun);
    alg.setPropertyValue("LastRun", firstRun);
    alg.setPropertyValue("OutputWorkspace", ws);

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!alg.isExecuted());
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

    MatrixWorkspace_sptr outWS =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(ws));

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->blocksize(), 1);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 1);

    TS_ASSERT_EQUALS(outWS->x(0)[0], 6473);
    TS_ASSERT_DELTA(outWS->y(0)[0], 0.283444, 0.000001);
    TS_ASSERT_DELTA(outWS->e(0)[0], 0.000145, 0.000001);
  }

  void test_run_start_log() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", "PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue", "run_start");
    alg.setPropertyValue("Red", "2");
    alg.setPropertyValue("Green", "1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS = std::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("PlotAsymmetryByLogValueTest_WS"));

    TS_ASSERT(outWS);

    const auto outputX = outWS->points(0);
    TS_ASSERT_EQUALS(outputX.size(), 2);
    // Zero = start time of first run (17:10:35)
    TS_ASSERT_DELTA(outputX[0], 0.0, 1.e-7);
    // 17:10:35 to 17:12:30 is 115 seconds
    TS_ASSERT_DELTA(outputX[1], 115.0, 1.e-7);
  }

  void test_run_end_log() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", "PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue", "run_end");
    alg.setPropertyValue("Red", "2");
    alg.setPropertyValue("Green", "1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS = std::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("PlotAsymmetryByLogValueTest_WS"));

    TS_ASSERT(outWS);

    const auto outputX = outWS->points(0);
    TS_ASSERT_EQUALS(outputX.size(), 2);
    // Zero = start time of first run (17:10:35)
    // 17:10:35 to 17:12:16 is 101 seconds
    TS_ASSERT_DELTA(outputX[0], 101.0, 1.e-7);
    // 17:10:35 to 17:14:10 is 215 seconds
    TS_ASSERT_DELTA(outputX[1], 215.0, 1.e-7);
  }

  void test_skip_missing_file() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();

    alg.setPropertyValue("FirstRun", "MUSR00015193.nxs");
    alg.setPropertyValue("LastRun", "MUSR00015195.nxs");
    alg.setPropertyValue("OutputWorkspace", "PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue", "run_number");
    alg.setPropertyValue("Red", "2");
    alg.setPropertyValue("Green", "1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS = std::dynamic_pointer_cast<MatrixWorkspace>(
        AnalysisDataService::Instance().retrieve("PlotAsymmetryByLogValueTest_WS"));
    TS_ASSERT(outWS);
    const auto &outputX = outWS->points(0);
    TS_ASSERT_EQUALS(outputX.size(), 2);
    TS_ASSERT_DELTA(outputX[0], 15193.0, 1.e-7);
    TS_ASSERT_DELTA(outputX[1], 15195.0, 1.e-7);
  }

  void test_extend_run_sequence() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();

    // Watch for the algorithm's progress reports as it loads each file
    ProgressWatcher watcher;
    alg.addObserver(watcher.getObserver());

    // Load the first two runs
    alg.setPropertyValue("FirstRun", "MUSR00015189.nxs");
    alg.setPropertyValue("LastRun", "MUSR00015190.nxs");
    alg.setPropertyValue("OutputWorkspace", "PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue", "run_number");
    alg.setPropertyValue("Red", "2");
    alg.setPropertyValue("Green", "1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    TS_ASSERT_EQUALS(watcher.getLoadedCount(), 2);
    TS_ASSERT_EQUALS(watcher.getFoundCount(), 0);

    // Now extend the run sequence with an extra run
    alg.setPropertyValue("LastRun", "MUSR00015191.nxs");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    TS_ASSERT_EQUALS(watcher.getLoadedCount(), 3); // i.e. not 5 loads
    TS_ASSERT_EQUALS(watcher.getFoundCount(), 2);  // reused 2
  }

  void test_validate_inputs_fails_if_neither_first_and_last_or_workspacenames_is_defined() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    auto result = alg.validateInputs();
    const auto expected = "Must either supply WorkspaceNames or FirstRun and "
                          "LastRun";
    TS_ASSERT_EQUALS(result["FirstRun"], expected);
    TS_ASSERT_EQUALS(result["LastRun"], expected);
    TS_ASSERT_EQUALS(result["WorkspaceNames"], expected);
  }

  void test_input_passes_with_first_and_last() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setProperty("FirstRun", firstRun);
    alg.setProperty("LastRun", lastRun);
    auto result = alg.validateInputs();
    TS_ASSERT(result.empty());
  }

  void test_input_passes_with_workspacenames() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    std::vector<std::string> input{firstRun, lastRun};
    alg.setProperty("WorkspaceNames", input);
    auto result = alg.validateInputs();
    std::vector<std::string> propertyValue = alg.getProperty("WorkspaceNames");
    TS_ASSERT(result.empty());
    TS_ASSERT_EQUALS(input, propertyValue);
  }

  void test_input_passes_with_both_file_input_methods_used() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    std::vector<std::string> input{firstRun, lastRun};
    alg.setProperty("WorkspaceNames", input);
    alg.setProperty("FirstRun", firstRun);
    alg.setProperty("LastRun", lastRun);
    auto result = alg.validateInputs();
    TS_ASSERT(result.empty());
  }

  void test_input_fails_with_only_first_supplied() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setProperty("FirstRun", firstRun);
    auto result = alg.validateInputs();
    const auto expected = "Must supply both FirstRun and LastRun";
    TS_ASSERT_EQUALS(result["FirstRun"], expected);
    TS_ASSERT_EQUALS(result["LastRun"], expected);
  }

  void test_input_fails_with_only_last_supplied() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setProperty("LastRun", lastRun);
    auto result = alg.validateInputs();
    const auto expected = "Must supply both FirstRun and LastRun";
    TS_ASSERT_EQUALS(result["FirstRun"], expected);
    TS_ASSERT_EQUALS(result["LastRun"], expected);
  }

  void test_extract_run_number_from_run_name() {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    const int firstRunNumber = alg.extractRunNumberFromRunName(firstRun);
    const int lastRunNumber = alg.extractRunNumberFromRunName(lastRun);
    TS_ASSERT_EQUALS(firstRunNumber, 15189);
    TS_ASSERT_EQUALS(lastRunNumber, 15190);
  }

private:
  std::string firstRun, lastRun;
};

class PlotAsymmetryByLogValueTestPerformance : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotAsymmetryByLogValueTestPerformance *createSuite() { return new PlotAsymmetryByLogValueTestPerformance(); }

  static void destroySuite(PlotAsymmetryByLogValueTestPerformance *suite) { delete suite; }

  PlotAsymmetryByLogValueTestPerformance() : firstRun("MUSR00015189.nxs"), lastRun("MUSR00015190.nxs") {}

  void setUp() override {
    alg.initialize();
    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", "outputWS");
    alg.setPropertyValue("LogValue", "Field_Danfysik");
    alg.setPropertyValue("Red", "2");
    alg.setPropertyValue("Green", "1");
  }

  void tearDown() override { Mantid::API::AnalysisDataService::Instance().remove("outputWS"); }

  void testPerformanceWS() { alg.execute(); }

private:
  PlotAsymmetryByLogValue alg;
  std::string firstRun, lastRun;
};
