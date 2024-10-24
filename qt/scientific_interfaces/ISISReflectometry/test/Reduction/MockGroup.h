// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/Reduction/Group.h"
#include "MantidFrameworkTestHelpers/FallbackBoostOptionalIO.h"

#include <gmock/gmock.h>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
class MockGroup : public IGroup {
public:
  // Mock methods overridden from Item
  MOCK_METHOD(bool, isGroup, (), (const, override));
  MOCK_METHOD(bool, isPreview, (), (const, override));
  MOCK_METHOD(void, renameOutputWorkspace, (std::string const &, std::string const &), (override));
  MOCK_METHOD(void, setOutputNames, (std::vector<std::string> const &), (override));
  MOCK_METHOD(int, totalItems, (), (const, override));
  MOCK_METHOD(int, completedItems, (), (const, override));
  MOCK_METHOD(void, notifyChildStateChanged, (), (override));

  // Mock methods overridden from IGroup
  MOCK_METHOD(std::string const &, name, (), (const, override));
  MOCK_METHOD(void, setName, (std::string const &), (override));
  MOCK_METHOD(bool, hasPostprocessing, (), (const, override));
  MOCK_METHOD(bool, requiresPostprocessing, (bool), (const, override));
  MOCK_METHOD(std::string, postprocessedWorkspaceName, (), (const, override));

  MOCK_METHOD(void, appendEmptyRow, (), (override));
  MOCK_METHOD(void, appendRow, (boost::optional<Row> const &), (override));
  MOCK_METHOD(void, insertRow, (boost::optional<Row> const &, int), (override));
  MOCK_METHOD(int, insertRowSortedByAngle, (boost::optional<Row> const &), (override));
  MOCK_METHOD(void, removeRow, (int), (override));
  MOCK_METHOD(void, updateRow, (int, boost::optional<Row> const &), (override));

  MOCK_METHOD(void, resetSkipped, (), (override));

  MOCK_METHOD(boost::optional<int>, indexOfRowWithTheta, (double, double), (const, override));

  MOCK_METHOD(boost::optional<Row> const &, bracketOp, (int), (const));
  MOCK_METHOD(std::vector<boost::optional<Row>> const &, rows, (), (const, override));
  MOCK_METHOD(std::vector<boost::optional<Row>> &, mutableRows, (), (override));

  MOCK_METHOD(boost::optional<Item &>, getItemWithOutputWorkspaceOrNone, (std::string const &), (override));

  MOCK_METHOD(void, setAllRowParents, (), (override));

  boost::optional<Row> const &operator[](int rowIndex) const override { return bracketOp(rowIndex); }
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
