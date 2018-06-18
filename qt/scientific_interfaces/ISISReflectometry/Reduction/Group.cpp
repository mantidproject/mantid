#include "Group.h"
#include "../Map.h"
#include "../IndexOf.h"
namespace MantidQt {
namespace CustomInterfaces {

template <typename Row>
Group<Row>::Group(std::string name, std::vector<boost::optional<Row>> rows,
                  std::string postprocessedWorkspaceName)
    : m_name(std::move(name)), m_rows(std::move(rows)),
      m_postprocessedWorkspaceName(std::move(postprocessedWorkspaceName)) {}

template <typename Row>
Group<Row>::Group(std::string name)
    : m_name(std::move(name)), m_rows(), m_postprocessedWorkspaceName() {}

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

template <typename Row>
std::string const &Group<Row>::postprocessedWorkspaceName() const {
  return m_postprocessedWorkspaceName;
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

UnslicedGroup unslice(SlicedGroup const &slicedGroup) {
  using UnsliceFunctionPtr =
      boost::optional<UnslicedRow>(*)(boost::optional<SlicedRow> const &);
  auto const &slicedRows = slicedGroup.rows();
  auto unslicedRows =
      map(slicedRows, static_cast<UnsliceFunctionPtr>(&unslice));
  return UnslicedGroup(slicedGroup.name(), std::move(unslicedRows),
                       slicedGroup.postprocessedWorkspaceName());
}

SlicedGroup slice(UnslicedGroup const &unslicedGroup) {
  using SliceFunctionPtr =
      boost::optional<SlicedRow>(*)(boost::optional<UnslicedRow> const &);
  auto const &unslicedRows = unslicedGroup.rows();
  auto slicedRows = map(unslicedRows, static_cast<SliceFunctionPtr>(&slice));
  return SlicedGroup(unslicedGroup.name(), std::move(slicedRows),
                     unslicedGroup.postprocessedWorkspaceName());
}

template <typename Row>
std::ostream &operator<<(std::ostream &os, Group<Row> const &group) {
  os << "  Group (name: " << group.name() << ")\n";
  for (auto &&row : group.rows()) {
    if (row.is_initialized())
      os << "    " << row.get() << '\n';
    else
      os << "    Row (invalid)\n";
  }
  return os;
}

template class Group<SlicedRow>;
template std::ostream &operator<<(std::ostream &, SlicedGroup const &);

template class Group<UnslicedRow>;
template std::ostream &operator<<(std::ostream &, UnslicedGroup const &);
}
}
