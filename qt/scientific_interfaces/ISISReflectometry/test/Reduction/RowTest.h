// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/Reduction/Group.h"
#include "MockGroup.h"
#include "TestHelpers/ModelCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;

class RowTest : public CxxTest::TestSuite {
public:
  void test_new_row_has_no_parent() {
    auto row = makeEmptyRow();
    TS_ASSERT_EQUALS(nullptr, row.getParent());
  }

  void test_set_get_parent() {
    auto mockGroup = MockGroup();
    auto row = makeEmptyRow();

    row.setParent(&mockGroup);

    TS_ASSERT_EQUALS(&mockGroup, row.getParent());
  }

  void test_setting_success_state_updates_parent() {
    auto mockGroup = MockGroup();
    auto row = makeEmptyRow();
    row.setParent(&mockGroup);
    EXPECT_CALL(mockGroup, notifyChildStateChanged()).Times(1);

    row.setSuccess();
  }

  void test_setting_error_state_updates_parent() {
    auto mockGroup = MockGroup();
    auto row = makeEmptyRow();
    row.setParent(&mockGroup);
    EXPECT_CALL(mockGroup, notifyChildStateChanged()).Times(1);

    row.setError("failed");
  }

  void test_setting_running_state_updates_parent() {
    auto mockGroup = MockGroup();
    auto row = makeEmptyRow();
    row.setParent(&mockGroup);
    EXPECT_CALL(mockGroup, notifyChildStateChanged()).Times(1);

    row.setRunning();
  }

  void test_setting_starting_state_updates_parent() {
    auto mockGroup = MockGroup();
    auto row = makeEmptyRow();
    row.setParent(&mockGroup);
    EXPECT_CALL(mockGroup, notifyChildStateChanged()).Times(1);

    row.setStarting();
  }

  void test_resetting_state_updates_parent() {
    auto mockGroup = MockGroup();
    auto row = makeEmptyRow();
    row.setParent(&mockGroup);
    EXPECT_CALL(mockGroup, notifyChildStateChanged()).Times(1);

    row.resetState();
  }

  void test_set_get_lookup_row_index() {
    auto row = makeEmptyRow();
    auto index = std::optional<size_t>{1};
    row.setLookupIndex(index);
    TS_ASSERT(row.lookupIndex().has_value());
    TS_ASSERT_EQUALS(row.lookupIndex().value(), index.value());
  }

  void test_set_get_no_lookup_row_index() {
    auto row = makeEmptyRow();
    std::optional<size_t> index = std::nullopt;
    row.setLookupIndex(index);
    TS_ASSERT(!row.lookupIndex().has_value());
    TS_ASSERT_EQUALS(row.lookupIndex(), std::nullopt);
  }
};
