// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Clipboard.h"
#include "MantidQtWidgets/Common/Batch/Cell.h"
#include "Reduction/RowLocation.h"
#include "Reduction/ValidateRow.h"

#include <algorithm>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

Clipboard::Clipboard() : m_subtrees(std::nullopt), m_subtreeRoots(std::nullopt) {}

Clipboard::Clipboard(std::vector<MantidQt::MantidWidgets::Batch::Subtree> subtrees,
                     std::vector<MantidQt::MantidWidgets::Batch::RowLocation> subtreeRoots) {
  m_subtrees = std::optional(subtrees);
  m_subtreeRoots = std::optional(subtreeRoots);
}

bool Clipboard::isInitialized() const { return m_subtrees.has_value() && m_subtreeRoots.has_value(); }

int Clipboard::numberOfRoots() const {
  if (!isInitialized())
    return 0;

  if (subtrees().size() != subtreeRoots().size())
    throw std::runtime_error("Invalid content on clipboard");

  return static_cast<int>(subtrees().size());
}

bool Clipboard::isGroupLocation(int rootIndex) const {
  if (!isInitialized() || rootIndex >= numberOfRoots())
    throw std::runtime_error("Attempted to access invalid value in clipboard");

  // Check if the root is a group
  if (!MantidQt::CustomInterfaces::ISISReflectometry::isGroupLocation(subtreeRoots()[rootIndex]))
    return false;

  // If so, check if the first selected item in this root is the root itself
  // (otherwise we only have child rows selected)
  if (!subtrees()[rootIndex][0].location().isRoot())
    return false;

  return true;
}

std::string Clipboard::groupName(int rootIndex) const {
  if (!isGroupLocation(rootIndex))
    throw std::runtime_error("Attempted to get group name for non-group clipboard item");

  // Check the first item in the selection for this root
  auto const rowIndex = 0;
  auto const &row = subtrees()[rootIndex][rowIndex];
  // Groups have a single cell containing the name.
  auto const cellIndex = 0;
  return row.cells()[cellIndex].contentText();
}

void Clipboard::setGroupName(int rootIndex, std::string const &groupName) {
  if (!isGroupLocation(rootIndex))
    throw std::runtime_error("Attempted to set group name for non-group clipboard item");

  // Get the first item in the selection for this root
  auto const rowIndex = 0;
  auto &row = mutableSubtrees()[rootIndex][rowIndex];
  // Groups have a single cell containing the name.
  auto const cellIndex = 0;
  return row.cells()[cellIndex].setContentText(groupName);
}

Group Clipboard::createGroupForRoot(int rootIndex) const {
  if (!isGroupLocation(rootIndex))
    throw std::runtime_error("Attempted to get group for non-group clipboard item");

  auto result = Group(groupName(rootIndex));
  auto rowsToAdd = createRowsForRootChildren(rootIndex);
  for (auto &row : rowsToAdd)
    result.appendRow(row);
  return result;
}

std::vector<std::optional<Row>> Clipboard::createRowsForAllRoots() const {
  if (containsGroups(*this))
    throw std::runtime_error("Attempted to get row for group clipboard item");

  auto result = std::vector<std::optional<Row>>();
  std::for_each(subtrees().cbegin(), subtrees().cend(), [this, &result](const auto &subtree) {
    const auto rowsToAdd = createRowsForSubtree(subtree);
    std::copy(rowsToAdd.cbegin(), rowsToAdd.cend(), std::back_inserter(result));
  });
  return result;
}

std::vector<std::optional<Row>> Clipboard::createRowsForRootChildren(int rootIndex) const {
  return createRowsForSubtree(subtrees()[rootIndex]);
}

std::vector<std::optional<Row>>
Clipboard::createRowsForSubtree(MantidQt::MantidWidgets::Batch::Subtree const &subtree) const {
  auto result = std::vector<std::optional<Row>>();

  for (auto const &row : subtree) {
    // Skip the root item if it is a group
    if (containsGroups(*this) && row.location().isRoot())
      continue;

    auto cells = std::vector<std::string>();
    std::transform(row.cells().cbegin(), row.cells().cend(), std::back_inserter(cells),
                   [](MantidQt::MantidWidgets::Batch::Cell const &cell) { return cell.contentText(); });
    auto validationResult = validateRow(cells);
    if (validationResult.isValid()) {
      result.emplace_back(validationResult.assertValid());
    } else {
      result.emplace_back(std::nullopt);
    }
  }

  return result;
}

std::vector<MantidQt::MantidWidgets::Batch::Subtree> &Clipboard::mutableSubtrees() { return m_subtrees.value(); }

std::vector<MantidQt::MantidWidgets::Batch::Subtree> const &Clipboard::subtrees() const { return m_subtrees.value(); }

std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &Clipboard::subtreeRoots() const {
  return m_subtreeRoots.value();
}

std::vector<MantidQt::MantidWidgets::Batch::RowLocation> &Clipboard::mutableSubtreeRoots() {
  return m_subtreeRoots.value();
}

bool containsGroups(Clipboard const &clipboard) {
  if (!clipboard.isInitialized())
    throw std::runtime_error("Attempted to access invalid value in clipboard");

  return containsGroups(clipboard.subtreeRoots());
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
