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

  void test_that_the_table_is_empty_on_initialization() { TS_ASSERT_EQUALS(true, m_periodInfo->isEmpty()); }
  void test_clear() { return; }
  void test_add_info_to_table() { return; }
  void test_DAQ_count_increases_as_expected() { return; }
  void test_number_of_sequences_when_empty() { return; }
  void test_set_number_of_sequences() { return; }
  void test_set_title() { return; }

private:
  std::unique_ptr<MuonPeriodInfo> m_periodInfo;
};