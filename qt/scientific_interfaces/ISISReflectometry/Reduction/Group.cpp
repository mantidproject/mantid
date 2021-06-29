// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Group.h"
#include "Common/IndexOf.h"
#include "Common/Map.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include <cmath>
#include <numeric>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

Group::Group( // cppcheck-suppress passedByValue
    std::string name, std::vector<boost::optional<Row>> rows)
    : m_name(std::move(name)), m_postprocessedWorkspaceName(), m_rows(std::move(rows)) {}

Group::Group(
    // cppcheck-suppress passedByValue
    std::string name)
    : m_name(std::move(name)), m_rows() {}

bool Group::isGroup() const { return true; }

std::string const &Group::name() const { return m_name; }

/** Returns true if postprocessing is applicable for this group, i.e. if it
 * has multiple valid rows whose outputs will be stitched
 */
bool Group::hasPostprocessing() const {
  auto numberOfValidRows = std::count_if(m_rows.cbegin(), m_rows.cend(),
                                         [](boost::optional<Row> const &row) { return row.is_initialized(); });
  return numberOfValidRows > 1;
}

/** Returns true if the group requires processing; that is if any of its rows
 * require processing (note the 'processing' here means reduction, not
 * postprocessing)
 */
bool Group::requiresProcessing(bool reprocessFailed) const {
  return std::any_of(m_rows.cbegin(), m_rows.cend(), [&reprocessFailed](boost::optional<Row> const &row) {
    return row && row->requiresProcessing(reprocessFailed);
  });
}

/** Returns true if the group is ready to be postprocessed, i.e. if its rows
 * have all been reduced successfully but the group itself has not already been
 * postprocessed yet. Returns false if postprocessing is not applicable.
 */
bool Group::requiresPostprocessing(bool reprocessFailed) const {
  // Check if postprocessing is applicable
  if (!hasPostprocessing())
    return false;

  // Check if it's already been done
  if (!Item::requiresProcessing(reprocessFailed))
    return false;

  // If all rows are valid and complete then we're ready to postprocess
  return std::all_of(m_rows.cbegin(), m_rows.cend(),
                     [](boost::optional<Row> const &row) { return row && row->success(); });
}

std::string Group::postprocessedWorkspaceName() const { return m_postprocessedWorkspaceName; }

boost::optional<int> Group::indexOfRowWithTheta(double theta, double tolerance) const {
  return indexOf(m_rows, [theta, tolerance](boost::optional<Row> const &row) -> bool {
    return row.is_initialized() && std::abs(row.get().theta() - theta) < tolerance;
  });
}

void Group::setName(std::string const &name) { m_name = name; }

void Group::resetState(bool resetChildren) {
  Item::resetState();
  if (!resetChildren)
    return;

  for (auto &row : m_rows)
    if (row)
      row->resetState();
}

void Group::resetSkipped() {
  Item::setSkipped(false);
  for (auto &row : m_rows)
    if (row)
      row->setSkipped(false);
}

bool Group::allRowsAreValid() const {
  return std::all_of(m_rows.cbegin(), m_rows.cend(),
                     [](boost::optional<Row> const &row) -> bool { return row.is_initialized(); });
}

std::vector<boost::optional<Row>> const &Group::rows() const { return m_rows; }

std::vector<boost::optional<Row>> &Group::mutableRows() { return m_rows; }

void Group::appendRow(boost::optional<Row> const &row) {
  Item::resetState();
  m_rows.emplace_back(row);
}

void Group::setOutputNames(std::vector<std::string> const &outputNames) {
  if (outputNames.size() != 1)
    throw std::runtime_error("Invalid number of output workspaces for group");

  m_postprocessedWorkspaceName = outputNames[0];
}

void Group::resetOutputs() { m_postprocessedWorkspaceName = ""; }

void Group::appendEmptyRow() {
  Item::resetState();
  m_rows.emplace_back(boost::none);
}

void Group::insertRow(boost::optional<Row> const &row, int beforeRowAtIndex) {
  Item::resetState();
  m_rows.insert(m_rows.begin() + beforeRowAtIndex, row);
}

/** Insert a row into a group and sort rows by the angle column. If the
 * row/angle is not initialized, add it at the end of the group.
 *
 * @param row : the row to insert
 * @returns : the index of the inserted row in the group's list of rows
 */
int Group::insertRowSortedByAngle(boost::optional<Row> const &row) {
  Item::resetState();

  // If the row is not sortable, or there is nothing in the list yet, append it
  if (!row.is_initialized() || m_rows.size() == 0) {
    appendRow(row);
    return static_cast<int>(m_rows.size() - 1);
  }

  // Find the first row with theta greater than the row we're inserting (or
  // end, if not found)
  auto valueLessThanRowTheta = [](double lhs, boost::optional<Row> const &rhs) {
    if (!rhs.is_initialized())
      return false;
    return lhs < rhs->theta();
  };
  auto insertIter = std::upper_bound(m_rows.cbegin(), m_rows.cend(), row->theta(), valueLessThanRowTheta);
  auto const insertedRowIndex = std::distance(m_rows.cbegin(), insertIter);
  m_rows.insert(insertIter, row); // invalidates iterator
  return static_cast<int>(insertedRowIndex);
}

void Group::removeRow(int rowIndex) {
  Item::resetState();
  m_rows.erase(m_rows.begin() + rowIndex);
}

void Group::updateRow(int rowIndex, boost::optional<Row> const &row) {
  if (row == m_rows[rowIndex])
    return;

  Item::resetState();
  m_rows[rowIndex] = row;
}

boost::optional<Row> const &Group::operator[](int rowIndex) const { return m_rows[rowIndex]; }

boost::optional<Item &> Group::getItemWithOutputWorkspaceOrNone(std::string const &wsName) {
  // Check if any of the child rows have this workspace output
  for (auto &row : m_rows) {
    if (row && row->hasOutputWorkspace(wsName)) {
      Item &item = *row;
      return boost::optional<Item &>(item);
    }
  }
  return boost::none;
}

void Group::renameOutputWorkspace(std::string const &oldName, std::string const &newName) {
  UNUSED_ARG(oldName);
  m_postprocessedWorkspaceName = newName;
}

int Group::totalItems() const {
  // Include the group if postprocessing is applicable
  auto initCount = hasPostprocessing() ? 1 : 0;
  // Include all valid rows
  return std::accumulate(rows().cbegin(), rows().cend(), initCount, [](int &count, boost::optional<Row> const &row) {
    if (row.is_initialized())
      return count + 1;
    else
      return count;
  });
}

int Group::completedItems() const {
  // Include the group if it has been postprocessing
  auto initCount = complete() ? 1 : 0;
  // Include all valid rows that have been processed
  return std::accumulate(rows().cbegin(), rows().cend(), initCount, [](int &count, boost::optional<Row> const &row) {
    if (row.is_initialized() && row->complete())
      return count + 1;
    else
      return count;
  });
}

bool operator!=(Group const &lhs, Group const &rhs) { return !(lhs == rhs); }

bool operator==(Group const &lhs, Group const &rhs) {
  return lhs.name() == rhs.name() && lhs.postprocessedWorkspaceName() == rhs.postprocessedWorkspaceName() &&
         lhs.rows() == rhs.rows();
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
