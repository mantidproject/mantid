// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/Reduction/Batch.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <unordered_set>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class BatchLookupIndexTest : public CxxTest::TestSuite {
public:
  static BatchLookupIndexTest *createSuite() { return new BatchLookupIndexTest(); }
  static void destroySuite(BatchLookupIndexTest *suite) { delete suite; }

  BatchLookupIndexTest()
      : m_instruments{"INTER", "OFFSPEC", "POLREF", "SURF", "CRISP"}, m_thetaTolerance(0.01),
        m_experiment(ModelCreationHelper::makeExperiment()), m_instrument(ModelCreationHelper::makeInstrument()),
        m_runsTable(m_instruments, m_thetaTolerance, ModelCreationHelper::twoGroupsWithTwoRowsAndOneEmptyGroupModel()),
        m_slicing() {}

  void test_update_lookup_index_single_row_match() {
    constexpr auto angle = 0.5;
    auto row = ModelCreationHelper::makeRow(angle);
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    TS_ASSERT_EQUALS(row.lookupIndex(), boost::none);
    model.updateLookupIndex(row);
    TS_ASSERT(row.lookupIndex().is_initialized());
    TS_ASSERT_EQUALS(row.lookupIndex().get(), size_t(1));
  }

  void test_update_lookup_index_single_row_wildcard() {
    constexpr auto angle = 0.1;
    auto row = ModelCreationHelper::makeRow(angle);
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    TS_ASSERT_EQUALS(row.lookupIndex(), boost::none);
    model.updateLookupIndex(row);
    TS_ASSERT(row.lookupIndex().is_initialized());
    TS_ASSERT_EQUALS(row.lookupIndex().get(), size_t(0));
  }

  void test_update_lookup_index_single_row_no_match() {
    constexpr auto angle = 0.1;
    auto row = ModelCreationHelper::makeRow(angle);
    auto model = Batch(ModelCreationHelper::makeEmptyExperiment(), m_instrument, m_runsTable, m_slicing);
    TS_ASSERT_EQUALS(row.lookupIndex(), boost::none);
    model.updateLookupIndex(row);
    TS_ASSERT(!row.lookupIndex().is_initialized());
    TS_ASSERT_EQUALS(row.lookupIndex(), boost::none);
  }

  void test_update_lookup_index_group_updates_all_rows() {
    auto rowA = ModelCreationHelper::makeRow(0.5);
    auto rowB = ModelCreationHelper::makeRow(2.3);
    auto rowW = ModelCreationHelper::makeRow(1.8);
    auto group = Group("groupName");
    group.appendRow(std::move(rowA));
    group.appendRow(std::move(rowB));
    group.appendRow(std::move(rowW));
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    TS_ASSERT_EQUALS(group.mutableRows()[0].get().lookupIndex(), boost::none);
    TS_ASSERT_EQUALS(group.mutableRows()[1].get().lookupIndex(), boost::none);
    TS_ASSERT_EQUALS(group.mutableRows()[2].get().lookupIndex(), boost::none);

    model.updateLookupIndexesOfGroup(group);

    TS_ASSERT(group.mutableRows()[0].get().lookupIndex().is_initialized());
    TS_ASSERT_EQUALS(group.mutableRows()[0].get().lookupIndex().get(), size_t(1));
    TS_ASSERT(group.mutableRows()[1].get().lookupIndex().is_initialized());
    TS_ASSERT_EQUALS(group.mutableRows()[1].get().lookupIndex().get(), size_t(2));
    TS_ASSERT(group.mutableRows()[2].get().lookupIndex().is_initialized());
    TS_ASSERT_EQUALS(group.mutableRows()[2].get().lookupIndex().get(), size_t(0));
  }

  void test_update_lookup_index_table_updates_all_groups() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);

    for (auto const &group : m_runsTable.reductionJobs().groups()) {
      for (auto const &row : group.rows()) {
        TS_ASSERT_EQUALS(row.get().lookupIndex(), boost::none);
      }
    }

    model.updateLookupIndexesOfTable();

    for (auto const &group : m_runsTable.reductionJobs().groups()) {
      for (auto const &row : group.rows()) {
        TS_ASSERT(row.is_initialized());
        if (row.get().theta() == 0.5) {
          TS_ASSERT_EQUALS(row.get().lookupIndex(), size_t(1));
        } else {
          TS_ASSERT_EQUALS(row.get().lookupIndex(), size_t(0));
        }
      }
    }
  }

private:
  std::vector<std::string> m_instruments;
  double m_thetaTolerance;
  Experiment m_experiment;
  Instrument m_instrument;
  RunsTable m_runsTable;
  Slicing m_slicing;
};
