#include "Group.h"
#include "../Map.h"
#include "../IndexOf.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"

namespace MantidQt {
namespace CustomInterfaces {

template <typename Row>
Group<Row>::Group(std::string name, std::vector<boost::optional<Row>> rows)
    : m_name(std::move(name)), m_rows(std::move(rows)) {}

template <typename Row>
Group<Row>::Group(std::string name)
    : m_name(std::move(name)), m_rows() {}

template <typename Row> std::string const &Group<Row>::name() const {
  return m_name;
}

template <typename Row>
boost::optional<int> Group<Row>::indexOfRowWithTheta(double theta,
                                                     double tolerance) const {
  return indexOf(m_rows,
                 [theta, tolerance](boost::optional<Row> const &row) -> bool {
                   return row.is_initialized() &&
                          std::abs(row.get().theta() - theta) < tolerance;
                 });
}

template <typename Row> void Group<Row>::setName(std::string const &name) {
  m_name = name;
}

template <typename Row> bool Group<Row>::allRowsAreValid() const {
  return std::all_of(m_rows.cbegin(), m_rows.cend(),
                     [](boost::optional<Row> const &row)
                         -> bool { return row.is_initialized(); });
}

template <typename Row>
std::string Group<Row>::postprocessedWorkspaceName(
    WorkspaceNamesFactory const &workspaceNamesFactory) const {
  assertOrThrow(allRowsAreValid(), "Attempted to get postprocessed workspace "
                                   "name from group with invalid rows.");
  auto runNumbers = map(m_rows, [](boost::optional<Row> const &row)
                                    -> std::vector<std::string> const *{
                                      return &row.get().runNumbers();
                                    });
  return workspaceNamesFactory
      .makePostprocessedName<typename Row::WorkspaceNames>(runNumbers);
}

template <typename Row>
std::vector<boost::optional<Row>> const &Group<Row>::rows() const {
  return m_rows;
}

template <typename Row>
void Group<Row>::appendRow(boost::optional<Row> const &row) {
  m_rows.emplace_back(row);
}

template <typename Row> void Group<Row>::appendEmptyRow() {
  m_rows.emplace_back(boost::none);
}

template <typename Row>
void Group<Row>::insertRow(boost::optional<Row> const &row,
                           int beforeRowAtIndex) {
  m_rows.insert(m_rows.begin() + beforeRowAtIndex, row);
}

template <typename Row> void Group<Row>::removeRow(int rowIndex) {
  m_rows.erase(m_rows.begin() + rowIndex);
}

template <typename Row>
void Group<Row>::updateRow(int rowIndex, boost::optional<Row> const &row) {
  m_rows[rowIndex] = row;
}

template <typename Row>
boost::optional<Row> const &Group<Row>::operator[](int rowIndex) const {
  return m_rows[rowIndex];
}

UnslicedGroup unslice(SlicedGroup const &slicedGroup,
                      WorkspaceNamesFactory const &workspaceNamesFactory) {
  auto const &slicedRows = slicedGroup.rows();
  auto unslicedRows =
      map(slicedRows, [&](boost::optional<SlicedRow> const &sliced)
                          -> boost::optional<UnslicedRow> {
                            return unslice(sliced, workspaceNamesFactory);
                          });
  return UnslicedGroup(slicedGroup.name(), std::move(unslicedRows));
}

SlicedGroup slice(UnslicedGroup const &unslicedGroup,
                  WorkspaceNamesFactory const &workspaceNamesFactory) {
  auto const &unslicedRows = unslicedGroup.rows();
  auto slicedRows =
      map(unslicedRows, [&](boost::optional<UnslicedRow> const &unsliced)
                            -> boost::optional<SlicedRow> {
                              return slice(unsliced, workspaceNamesFactory);
                            });
  return SlicedGroup(unslicedGroup.name(), std::move(slicedRows));
}

template class Group<SlicedRow>;
template class Group<UnslicedRow>;
}
}
