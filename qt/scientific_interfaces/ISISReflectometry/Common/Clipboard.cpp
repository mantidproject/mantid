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

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

Clipboard::Clipboard() : m_subtrees(boost::none), m_subtreeRoots(boost::none) {}

Clipboard::Clipboard(boost::optional<std::vector<MantidQt::MantidWidgets::Batch::Subtree>> subtrees,
                     boost::optional<std::vector<MantidQt::MantidWidgets::Batch::RowLocation>> subtreeRoots)
    : m_subtrees(std::move(subtrees)), m_subtreeRoots(std::move(subtreeRoots)) {}

bool Clipboard::isInitialized() const { return m_subtrees.is_initialized() && m_subtreeRoots.is_initialized(); }

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

std::vector<boost::optional<Row>> Clipboard::createRowsForAllRoots() const {
  if (containsGroups(*this))
    throw std::runtime_error("Attempted to get row for group clipboard item");

  auto result = std::vector<boost::optional<Row>>();
  for (auto const &subtree : subtrees()) {
    auto rowsToAdd = createRowsForSubtree(subtree);
    for (auto &row : rowsToAdd)
      result.emplace_back(row);
  }
  return result;
}

std::vector<boost::optional<Row>> Clipboard::createRowsForRootChildren(int rootIndex) const {
  return createRowsForSubtree(subtrees()[rootIndex]);
}

std::vector<boost::optional<Row>>
Clipboard::createRowsForSubtree(MantidQt::MantidWidgets::Batch::Subtree const &subtree) const {
  auto result = std::vector<boost::optional<Row>>();

  for (auto const &row : subtree) {
    // Skip the root item if it is a group
    if (containsGroups(*this) && row.location().isRoot())
      continue;

    auto cells = std::vector<std::string>();
    std::transform(row.cells().cbegin(), row.cells().cend(), std::back_inserter(cells),
                   [](MantidQt::MantidWidgets::Batch::Cell const &cell) { return cell.contentText(); });
    auto validationResult = validateRow(cells);
    if (validationResult.isValid())
      result.emplace_back(validationResult.assertValid());
    else
      result.emplace_back(boost::none);
  }

  return result;
}

std::vector<MantidQt::MantidWidgets::Batch::Subtree> &Clipboard::mutableSubtrees() { return m_subtrees.get(); }

std::vector<MantidQt::MantidWidgets::Batch::Subtree> const &Clipboard::subtrees() const { return m_subtrees.get(); }

std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &Clipboard::subtreeRoots() const {
  return m_subtreeRoots.get();
}

std::vector<MantidQt::MantidWidgets::Batch::RowLocation> &Clipboard::mutableSubtreeRoots() {
  return m_subtreeRoots.get();
}

bool containsGroups(Clipboard const &clipboard) {
  if (!clipboard.isInitialized())
    throw std::runtime_error("Attempted to access invalid value in clipboard");

  return containsGroups(clipboard.subtreeRoots());
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
