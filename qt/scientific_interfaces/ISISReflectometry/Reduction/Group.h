// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "IGroup.h"
#include "Item.h"
#include "Row.h"
#include <boost/optional.hpp>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class Row;

/** @class Group

    The Group model holds information about a group of rows in the runs table.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL Group final : public IGroup {
public:
  explicit Group(std::string name);
  Group(std::string name, std::vector<boost::optional<Row>> rows);

  ~Group() = default;

  Group(Group &&group) noexcept;
  Group(const Group &old_group);
  Group &operator=(Group &&) noexcept;
  Group &operator=(Group const &);

  // copy-and-swap idiom.
  friend void swap(Group &first, Group &second) {
    std::swap(first.m_name, second.m_name);
    std::swap(first.m_rows, second.m_rows);
    std::swap(first.m_postprocessedWorkspaceName, second.m_postprocessedWorkspaceName);
    std::swap(first.m_itemState, second.m_itemState);
    std::swap(first.m_skipped, second.m_skipped);
  }

  // Overrides from Item
  bool isGroup() const override;
  bool isPreview() const override;
  bool requiresProcessing(bool reprocessFailed) const override;
  void setOutputNames(std::vector<std::string> const &outputNames) override;
  void resetOutputs() override;
  int totalItems() const override;
  int completedItems() const override;
  void resetState(bool resetChildren = true) override;
  void renameOutputWorkspace(std::string const &oldName, std::string const &newName) override;
  void notifyChildStateChanged() override;

  // Overrides from IGroup
  std::string const &name() const override;
  void setName(std::string const &name) override;
  bool hasPostprocessing() const override;
  bool requiresPostprocessing(bool reprocessFailed) const override;
  std::string postprocessedWorkspaceName() const override;

  void appendEmptyRow() override;
  void appendRow(boost::optional<Row> const &row) override;
  void insertRow(boost::optional<Row> const &row, int beforeRowAtIndex) override;
  int insertRowSortedByAngle(boost::optional<Row> const &row) override;
  void removeRow(int rowIndex) override;
  void updateRow(int rowIndex, boost::optional<Row> const &row) override;

  void resetSkipped() override;

  std::optional<int> indexOfRowWithTheta(double angle, double tolerance) const override;

  boost::optional<Row> const &operator[](int rowIndex) const override;
  std::vector<boost::optional<Row>> const &rows() const override;
  std::vector<boost::optional<Row>> &mutableRows() override;

  boost::optional<Item &> getItemWithOutputWorkspaceOrNone(std::string const &wsName) override;

  void setAllRowParents() override;

  void setChildrenSuccess();

private:
  std::string m_name;
  std::string m_postprocessedWorkspaceName;
  std::vector<boost::optional<Row>> m_rows;
  friend class Encoder;

  friend class Decoder;
  friend class CoderCommonTester;

  bool allChildRowsSucceeded() const;
};

template <typename ModificationListener>
void mergeRowsInto(Group &intoHere, Group const &fromHere, int groupIndex, double thetaTolerance,
                   ModificationListener &listener) {
  for (auto const &maybeRow : fromHere.rows()) {
    if (maybeRow.is_initialized()) {
      auto const &fromRow = maybeRow.get();
      auto index = intoHere.indexOfRowWithTheta(fromRow.theta(), thetaTolerance);
      if (index.has_value()) {
        auto const updateAtIndex = index.value();
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
