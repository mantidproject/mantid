// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidQtWidgets/Common/MuonPeriodInfo.h"

using namespace MantidQt::MantidWidgets;

class MuonPeriodInfoTest : public CxxTest::TestSuite {
public:
  static MuonPeriodInfoTest *createSuite() { return new MuonPeriodInfoTest(); }
  static void destroySuite(MuonPeriodInfoTest *suite) { delete suite; }
  void setUp() override { m_periodInfo = std::make_unique<MuonPeriodInfo>(); }
  void tearDown() override { m_periodInfo.reset(); }

  void assertRowValues(const int row, const std::string &name, const std::string &type, const std::string &DAQNumber,
                       const std::string &frames, const std::string &totalFrames, const std::string &counts,
                       const std::string &tag) {
    const auto table = m_periodInfo->getTable();
    TS_ASSERT_EQUALS(std::to_string(row + 1), table->item(row, 0)->text().toStdString()); // Period Count
    TS_ASSERT_EQUALS(name, table->item(row, 1)->text().toStdString());
    TS_ASSERT_EQUALS(type, table->item(row, 2)->text().toStdString());
    TS_ASSERT_EQUALS(DAQNumber, table->item(row, 3)->text().toStdString());
    TS_ASSERT_EQUALS(frames, table->item(row, 4)->text().toStdString());
    TS_ASSERT_EQUALS(totalFrames, table->item(row, 5)->text().toStdString());
    TS_ASSERT_EQUALS(counts, table->item(row, 6)->text().toStdString());
    TS_ASSERT_EQUALS(tag, table->item(row, 7)->text().toStdString());
  }

  void test_that_the_table_is_empty_on_initialization() { TS_ASSERT_EQUALS(true, m_periodInfo->isEmpty()); }

  void test_clear() {
    auto table = m_periodInfo->getTable();
    table->insertRow(0);
    table->insertRow(1);
    TS_ASSERT_EQUALS(false, m_periodInfo->isEmpty());
    m_periodInfo->clear();
    TS_ASSERT_EQUALS(true, m_periodInfo->isEmpty());
    TS_ASSERT_EQUALS(0, m_periodInfo->getDAQCount());
    TS_ASSERT_EQUALS(-1, m_periodInfo->getNumberOfSequences());
    TS_ASSERT_EQUALS("Period Information for Run(s) ", m_periodInfo->getWidgetTitleRuns());
  }

  void test_addPeriodToTable() {
    m_periodInfo->addPeriodToTable("state 1 dwell", "2", "10", "200", "25", "1");
    m_periodInfo->addPeriodToTable("state 1", "1", "50", "1000", "25", "2");
    assertRowValues(0, "state 1 dwell", "DWELL", "-", "10", "200", "-", "0001");
    assertRowValues(1, "state 1", "DAQ", "1", "50", "1000", "25", "0010");
    TS_ASSERT_EQUALS(2, m_periodInfo->getTable()->rowCount());
    TS_ASSERT_EQUALS(false, m_periodInfo->isEmpty());
  }

  void test_addPeriodToTable_with_bad_tag() {
    m_periodInfo->addPeriodToTable("state 1 dwell", "2", "10", "200", "25", "tag");
    assertRowValues(0, "state 1 dwell", "DWELL", "-", "10", "200", "-", "Not found");
  }

  void test_DAQ_count_increases_as_expected() {
    // Periods can be DAQ (1) or DWELL (2) indicated by the second argument of the addPeriodToTable method
    // Add one DAQ period and expect the count to increase
    TS_ASSERT_EQUALS(0, m_periodInfo->getDAQCount());
    m_periodInfo->addPeriodToTable("state 1", "1", "10", "200", "25", "1");
    TS_ASSERT_EQUALS(1, m_periodInfo->getDAQCount());

    // Now add a DWELL period and expect the count to be the same
    m_periodInfo->addPeriodToTable("state 1 dwell", "2", "10", "200", "25", "1");
    TS_ASSERT_EQUALS(1, m_periodInfo->getDAQCount());

    // Add some final DAQ periods to check count is as expected at the end
    m_periodInfo->addPeriodToTable("state 1", "1", "10", "200", "25", "1");
    m_periodInfo->addPeriodToTable("state 1", "1", "10", "200", "25", "1");
    TS_ASSERT_EQUALS(3, m_periodInfo->getDAQCount());
  }
  void test_setNumberOfSequences_to_negative() {
    m_periodInfo->setNumberOfSequences(-1);
    TS_ASSERT_EQUALS("Number of period cycles not found", m_periodInfo->getNumberOfSequencesString());
  }

  void test_setNumberOfSequences() {
    m_periodInfo->setNumberOfSequences(2);
    TS_ASSERT_EQUALS("Run contains 2 cycles of periods", m_periodInfo->getNumberOfSequencesString());
  }

  void test_setWidgetTitleRuns_to_empty() {
    m_periodInfo->setWidgetTitleRuns("");
    TS_ASSERT_EQUALS("Period Information for Run(s) ", m_periodInfo->getWidgetTitleRuns());
  }

  void test_setWidgetTitleRuns() {
    m_periodInfo->setWidgetTitleRuns("HIFI110542");
    TS_ASSERT_EQUALS("Period Information for Run(s) HIFI110542", m_periodInfo->getWidgetTitleRuns());
  }

  void test_parseSampleLog() {
    std::string log{"name1;name2;name3;name4"};
    std::vector<std::string> expected{"name1", "name2", "name3", "name4"};
    TS_ASSERT_EQUALS(expected, m_periodInfo->parseSampleLog(log, ";"));
  }

  void test_makeCorrections() {
    std::vector<std::vector<std::string>> logs{{"name1"}, {"10", "20"}, {}};
    std::vector<std::vector<std::string>> expected{{"name1", "Not found"}, {"10", "20"}, {"Not found", "Not found"}};
    TS_ASSERT_EQUALS(expected, m_periodInfo->makeCorrections(logs));
  }

private:
  std::unique_ptr<MuonPeriodInfo> m_periodInfo;
};
