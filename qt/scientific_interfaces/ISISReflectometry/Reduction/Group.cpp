// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Group.h"
#include "Common/IndexOf.h"
#include "Common/Map.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include <cmath>

namespace MantidQt {
namespace CustomInterfaces {

Group::Group( // cppcheck-suppress passedByValue
    std::string name, std::vector<boost::optional<Row>> rows)
    : m_name(std::move(name)), m_postprocessedWorkspaceName(),
      m_rows(std::move(rows)) {}

Group::Group(
    // cppcheck-suppress passedByValue
    std::string name)
    : m_name(std::move(name)), m_rows() {}

bool Group::isGroup() const { return true; }

std::string const &Group::name() const { return m_name; }

bool Group::requiresPostprocessing() const { return m_rows.size() > 1; }

std::string Group::postprocessedWorkspaceName() const {
  return m_postprocessedWorkspaceName;
}

boost::optional<int> Group::indexOfRowWithTheta(double theta,
                                                double tolerance) const {
  return indexOf(m_rows,
                 [theta, tolerance](boost::optional<Row> const &row) -> bool {
                   return row.is_initialized() &&
                          std::abs(row.get().theta() - theta) < tolerance;
                 });
}

void Group::setName(std::string const &name) { m_name = name; }

void Group::resetState() {
  Item::resetState();
  for (auto &row : m_rows)
    row->resetState();
}

bool Group::allRowsAreValid() const {
  return std::all_of(m_rows.cbegin(), m_rows.cend(),
                     [](boost::optional<Row> const &row) -> bool {
                       return row.is_initialized();
                     });
}

std::vector<boost::optional<Row>> const &Group::rows() const { return m_rows; }

std::vector<boost::optional<Row>> &Group::mutableRows() { return m_rows; }

void Group::appendRow(boost::optional<Row> const &row) {
  m_rows.emplace_back(row);
}

void Group::setOutputNames(std::vector<std::string> const &outputNames) {
  if (outputNames.size() != 1)
    throw std::runtime_error("Invalid number of output workspaces for group");

  m_postprocessedWorkspaceName = outputNames[0];
}

void Group::appendEmptyRow() { m_rows.emplace_back(boost::none); }

void Group::insertRow(boost::optional<Row> const &row, int beforeRowAtIndex) {
  m_rows.insert(m_rows.begin() + beforeRowAtIndex, row);
}

void Group::removeRow(int rowIndex) { m_rows.erase(m_rows.begin() + rowIndex); }

void Group::updateRow(int rowIndex, boost::optional<Row> const &row) {
  m_rows[rowIndex] = row;
}

boost::optional<Row> const &Group::operator[](int rowIndex) const {
  return m_rows[rowIndex];
}

boost::optional<Item &>
Group::getItemWithOutputWorkspaceOrNone(std::string const &wsName) {
  // Check if any of the child rows have this workspace output
  for (auto &row : m_rows) {
    if (row && row->hasOutputWorkspace(wsName)) {
      Item &item = *row;
      return boost::optional<Item &>(item);
    }
  }
  return boost::none;
}

void Group::renameOutputWorkspace(std::string const &oldName,
                                  std::string const &newName) {
  UNUSED_ARG(oldName);
  m_postprocessedWorkspaceName = newName;
}
} // namespace CustomInterfaces
} // namespace MantidQt
