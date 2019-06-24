// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Clipboard.h"
#include "Reduction/RowLocation.h"

namespace MantidQt {
namespace CustomInterfaces {

Clipboard::Clipboard() : m_subtrees(boost::none), m_subtreeRoots(boost::none) {}

Clipboard::Clipboard(
    boost::optional<std::vector<MantidQt::MantidWidgets::Batch::Subtree>>
        subtrees,
    boost::optional<std::vector<MantidQt::MantidWidgets::Batch::RowLocation>>
        subtreeRoots)
    : m_subtrees(std::move(subtrees)), m_subtreeRoots(std::move(subtreeRoots)) {
}

bool Clipboard::isInitialized() const {
  return m_subtrees.is_initialized() && m_subtreeRoots.is_initialized();
}

int Clipboard::size() const {
  if (!isInitialized())
    return 0;

  if (subtrees().size() != subtreeRoots().size())
    throw std::runtime_error("Invalid content on clipboard");

  return static_cast<int>(subtrees().size());
}

bool Clipboard::isGroupLocation(int rootIndex) const {
  if (!isInitialized() || rootIndex >= size())
    throw std::runtime_error("Attempted to access invalid value in clipboard");

  // Check if the root is a group
  if (!MantidQt::CustomInterfaces::isGroupLocation(subtreeRoots()[rootIndex]))
    return false;

  // If so, check if the first selected item in this root is the root itself
  // (otherwise we only have child rows selected)
  if (!subtrees()[rootIndex][0].location().isRoot())
    return false;

  return true;
}

std::string Clipboard::groupName(int rootIndex) const {
  if (!isGroupLocation(rootIndex))
    throw std::runtime_error(
        "Attempted to get group name for non-group clipboard item");

  // Check the first item in the selection for this root
  auto const rowIndex = 0;
  auto const &row = subtrees()[rootIndex][rowIndex];
  // Groups have a single cell containing the name.
  auto const cellIndex = 0;
  return row.cells()[cellIndex].contentText();
}

void Clipboard::setGroupName(int rootIndex, std::string const &groupName) {
  if (!isGroupLocation(rootIndex))
    throw std::runtime_error(
        "Attempted to set group name for non-group clipboard item");

  // Get the first item in the selection for this root
  auto const rowIndex = 0;
  auto &row = mutableSubtrees()[rootIndex][rowIndex];
  // Groups have a single cell containing the name.
  auto const cellIndex = 0;
  return row.cells()[cellIndex].setContentText(groupName);
}

std::vector<MantidQt::MantidWidgets::Batch::Subtree> &
Clipboard::mutableSubtrees() {
  return m_subtrees.get();
}

std::vector<MantidQt::MantidWidgets::Batch::Subtree> const &
Clipboard::subtrees() const {
  return m_subtrees.get();
}

std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
Clipboard::subtreeRoots() const {
  return m_subtreeRoots.get();
}

std::vector<MantidQt::MantidWidgets::Batch::RowLocation> &
Clipboard::mutableSubtreeRoots() {
  return m_subtreeRoots.get();
}

bool containsGroups(Clipboard const &clipboard) {
  return containsGroups(clipboard.subtreeRoots());
}
} // namespace CustomInterfaces
} // namespace MantidQt
