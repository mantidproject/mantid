// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "Item.h"
#include "Row.h"

#include <optional>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

/** @class Group

    The Group model holds information about a group of rows in the runs table.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL Group : public Item {
public:
  explicit Group(std::string name);
  Group(std::string name, std::vector<std::optional<Row>> rows);

  bool isGroup() const override;
  bool isPreview() const override;
  std::string const &name() const;
  void setName(std::string const &name);
  bool hasPostprocessing() const;
  bool requiresProcessing(bool reprocessFailed) const override;
  bool requiresPostprocessing(bool reprocessFailed) const;
  std::string postprocessedWorkspaceName() const;
  void setOutputNames(std::vector<std::string> const &outputNames) override;
  void resetOutputs() override;

  void appendEmptyRow();
  void appendRow(std::optional<Row> const &row);
  void insertRow(std::optional<Row> const &row, int beforeRowAtIndex);
  int insertRowSortedByAngle(std::optional<Row> const &row);
  void removeRow(int rowIndex);
  void updateRow(int rowIndex, std::optional<Row> const &row);

  int totalItems() const override;
  int completedItems() const override;

  void resetState(bool resetChildren = true) override;
  void resetSkipped();
  void renameOutputWorkspace(std::string const &oldName, std::string const &newName) override;

  std::optional<int> indexOfRowWithTheta(double angle, double tolerance) const;

  std::optional<Row> const &operator[](int rowIndex) const;
  std::vector<std::optional<Row>> const &rows() const;
  std::vector<std::optional<Row>> &mutableRows();

  Item* getItemWithOutputWorkspaceOrNone(std::string const &wsName);

private:
  std::string m_name;
  std::string m_postprocessedWorkspaceName;
  std::vector<std::optional<Row>> m_rows;

  friend class Encoder;
  friend class Decoder;
  friend class CoderCommonTester;
};

template <typename ModificationListener>
void mergeRowsInto(Group &intoHere, Group const &fromHere, int groupIndex, double thetaTolerance,
                   ModificationListener &listener) {
  for (auto const &maybeRow : fromHere.rows()) {
    if (maybeRow.has_value()) {
      auto const &fromRow = maybeRow.value();
      auto index = intoHere.indexOfRowWithTheta(fromRow.theta(), thetaTolerance);
      if (index.has_value()) {
        auto const updateAtIndex = index.value();
        auto const &intoRow = intoHere[updateAtIndex].value();
        auto updatedRow = mergedRow(intoRow, fromRow);
        intoHere.updateRow(updateAtIndex, updatedRow);
        listener.rowModified(groupIndex, updateAtIndex, updatedRow);
      } else {
        auto insertedRowIndex = intoHere.insertRowSortedByAngle(fromRow);
        listener.rowInserted(groupIndex, insertedRowIndex, fromRow);
      }
    }
  }
}

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(Group const &lhs, Group const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(Group const &lhs, Group const &rhs);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
