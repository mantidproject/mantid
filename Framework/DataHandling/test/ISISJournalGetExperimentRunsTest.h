// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IJournal.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/ISISJournalGetExperimentRuns.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/WarningSuppressions.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using Mantid::API::ITableWorkspace_sptr;
using testing::_;
using testing::AtLeast;
using testing::NiceMock;
using testing::Return;

static auto testRun1 = IJournal::RunData{{"name", "run 1"}, {"run_number", "12345"}, {"title", "run 1 description"}};
static auto testRun2 = IJournal::RunData{{"name", "run 2"}, {"run_number", "22345"}, {"title", "run 2 description"}};

/**
 * Mock out the IJournal calls used by the algorithm
 */
class MockJournal : public IJournal {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD0(getCycleNames, std::vector<std::string>());
  MOCK_METHOD2(getRuns, std::vector<RunData>(std::vector<std::string> const &, RunData const &));
  GNU_DIAG_ON_SUGGEST_OVERRIDE
};

/**
 * Derive our own version of the algorithm we want to test so that we can set
 * it up to use the mock journal.
 */
class MockISISJournalGetExperimentRuns : public ISISJournalGetExperimentRuns {
public:
  // The constructor takes ownership of the journal, but note that when
  // makeJournal is called ownership is lost, so this algorithm can only be
  // executed once
  MockISISJournalGetExperimentRuns() : m_journal(std::make_unique<NiceMock<MockJournal>>()) {}

  void setJournal(std::unique_ptr<IJournal> journal) { m_journal = std::move(journal); }

private:
  std::unique_ptr<IJournal> m_journal;

  // Note that this passes ownership of the journal to the caller so can only
  // be called once (i.e. create a new algorithm for each test).
  std::unique_ptr<IJournal> makeJournal(std::string const &, std::string const &) override {
    return std::move(m_journal);
  }
};

class ISISJournalGetExperimentRunsTest : public CxxTest::TestSuite {
public:
  void test_init() {
    MockISISJournalGetExperimentRuns alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_executes() {
    MockISISJournalGetExperimentRuns alg;
    setupAlg(alg);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }

  void test_returns_correct_table_names() {
    MockISISJournalGetExperimentRuns alg;
    setupAlg(alg);
    alg.execute();

    ITableWorkspace_sptr table = alg.getProperty("Outputworkspace");
    auto const names = table->getColumnNames();
    auto const expectedNames = std::vector<std::string>{"Name", "Run Number", "Title"};
    TS_ASSERT_EQUALS(table->columnCount(), expectedNames.size());
    TS_ASSERT_EQUALS(names, expectedNames);
  }

  void test_returns_correct_table_values() {
    MockISISJournalGetExperimentRuns alg;
    setupAlg(alg);
    auto journal = setupAlgJournal(alg);
    auto expectedRuns = std::vector<IJournal::RunData>{testRun1, testRun2};
    expectJournalReturns(journal, expectedRuns);
    alg.execute();

    ITableWorkspace_sptr table = alg.getProperty("Outputworkspace");
    TS_ASSERT_EQUALS(table->rowCount(), expectedRuns.size());
    for (size_t i = 0; i < expectedRuns.size(); ++i) {
      TableRow row = table->getRow(i);
      assertTableRowEquals(row, expectedRuns[i]);
    }
  }

private:
  // Set up some example properties for an algorithm and initialise it
  void setupAlg(MockISISJournalGetExperimentRuns &alg) {
    Mantid::API::FrameworkManager::Instance();
    alg.initialize();
    alg.setChild(true);
    alg.setPropertyValue("Instrument", "OFFSPEC");
    alg.setPropertyValue("Cycle", "12_3");
    alg.setPropertyValue("InvestigationId", "1234567");
    alg.setPropertyValue("OutputWorkspace", "output_runs");
  }

  // Set up and return a known mock journal on the algorithm so that we can
  // test expectations on it
  NiceMock<MockJournal> *setupAlgJournal(MockISISJournalGetExperimentRuns &alg) {
    auto journal_uptr = std::make_unique<NiceMock<MockJournal>>();
    auto journal = journal_uptr.get();
    alg.setJournal(std::move(journal_uptr));
    return journal;
  }

  void expectJournalReturns(NiceMock<MockJournal> *journal, std::vector<IJournal::RunData> const &expectedRuns) {
    EXPECT_CALL(*journal, getRuns(_, _)).Times(AtLeast(1)).WillRepeatedly(Return(expectedRuns));
  }

  void assertTableRowEquals(TableRow &tableRow, IJournal::RunData &runData) {
    TS_ASSERT_EQUALS(tableRow.String(0), runData["name"]);
    TS_ASSERT_EQUALS(tableRow.String(1), runData["run_number"]);
    TS_ASSERT_EQUALS(tableRow.String(2), runData["title"]);
  }
};
