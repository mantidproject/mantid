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

  void test_update_parent_notifies_group() {
    auto mockGroup = MockGroup();
    auto row = makeEmptyRow();
    row.setParent(&mockGroup);
    EXPECT_CALL(mockGroup, notifyChildStateChanged()).Times(1);

    row.updateParent();
  }

  void test_update_parent_with_null_parent_does_not_error() {
    auto row = makeEmptyRow();
    row.updateParent();
  }
};