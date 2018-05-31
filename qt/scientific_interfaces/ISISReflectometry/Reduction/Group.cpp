#include "Group.h"
namespace MantidQt {
namespace CustomInterfaces {

template <typename Row>
Group<Row>::Group(std::string name, std::vector<boost::optional<Row>> rows,
                  std::string postprocessedWorkspaceName)
    : m_name(std::move(name)), m_rows(std::move(rows)),
      m_postprocessedWorkspaceName(std::move(postprocessedWorkspaceName)) {}

template <typename Row> std::string const &Group<Row>::name() const {
  return m_name;
}

template <typename Row> void Group<Row>::setName(std::string const& name) {
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
void Group<Row>::insertRow(boost::optional<Row> const &row, int beforeRowAtIndex) {
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
}
}
