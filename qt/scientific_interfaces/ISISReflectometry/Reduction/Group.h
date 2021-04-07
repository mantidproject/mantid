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
#include <boost/optional.hpp>
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
  Group(std::string name, std::vector<boost::optional<Row>> rows);

  bool isGroup() const override;
  std::string const &name() const;
  void setName(std::string const &name);
  bool hasPostprocessing() const;
  bool requiresProcessing(bool reprocessFailed) const override;
  bool requiresPostprocessing(bool reprocessFailed) const;
  std::string postprocessedWorkspaceName() const;
  std::vector<std::string> workspaceNamesToPostprocess() const;
  void setOutputNames(std::vector<std::string> const &outputNames) override;
  void resetOutputs() override;

  void appendEmptyRow();
  void appendRow(boost::optional<Row> const &row);
  void insertRow(boost::optional<Row> const &row, int beforeRowAtIndex);
  int insertRowSortedByAngle(boost::optional<Row> const &row);
  void removeRow(int rowIndex);
  void updateRow(int rowIndex, boost::optional<Row> const &row);
  bool allRowsAreValid() const;

  int totalItems() const override;
  int completedItems() const override;

  void resetState(bool resetChildren = true) override;
  void resetSkipped();
  void renameOutputWorkspace(std::string const &oldName, std::string const &newName) override;

  boost::optional<int> indexOfRowWithTheta(double angle, double tolerance) const;

  boost::optional<Row> const &operator[](int rowIndex) const;
  std::vector<boost::optional<Row>> const &rows() const;
  std::vector<boost::optional<Row>> &mutableRows();

  boost::optional<Item &> getItemWithOutputWorkspaceOrNone(std::string const &wsName);

private:
  std::string m_name;
  std::string m_postprocessedWorkspaceName;
  std::vector<boost::optional<Row>> m_rows;

  friend class Encoder;
  friend class Decoder;
  friend class CoderCommonTester;
};

template <typename ModificationListener>
void mergeRowsInto(Group &intoHere, Group const &fromHere, int groupIndex, double thetaTolerance,
                   ModificationListener &listener) {
  for (auto const &maybeRow : fromHere.rows()) {
    if (maybeRow.is_initialized()) {
      auto const &fromRow = maybeRow.get();
      auto index = intoHere.indexOfRowWithTheta(fromRow.theta(), thetaTolerance);
      if (index.is_initialized()) {
        auto const updateAtIndex = index.get();
        auto const &intoRow = intoHere[updateAtIndex].get();
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