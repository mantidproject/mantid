// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/DataProcessorUI/WhiteList.h"
#include <QString>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** Adds an element to the whitelist
 * @param colName : the name of the column to be added
 * @param algProperty : the name of the property linked to this column
 * @param description : a description of this column
 * @param isShown : true if we want to use what's in this column to
 * generate the output ws name.
 * @param isKey : true if we want to use this column as a key value i.e.
 * something that uniquely identifies the row within the group
 * @param prefix : the prefix to be added to the value of this column
 */
void WhiteList::addElement(const QString &colName, const QString &algProperty,
                           const QString &description, bool isShown,
                           const QString &prefix, bool isKey) {
  m_names.emplace_back(colName);
  m_algorithmProperties.emplace_back(algProperty);
  m_isShown.push_back(isShown);
  /* std::vector<bool> does not have emplace_back until c++14 (currently not
   * fully supported on RHEL7).
   * See: http://en.cppreference.com/w/cpp/container/vector/emplace_back */
  m_prefixes.emplace_back(prefix);
  m_descriptions.emplace_back(description);
  m_isKey.push_back(isKey);
}

/** Returns the column index for a column specified via its name
    @param columnName : The column name
*/
int WhiteList::indexFromName(const QString &columnName) const {
  auto nameIt = std::find(m_names.cbegin(), m_names.cend(), columnName);
  return static_cast<int>(std::distance(m_names.cbegin(), nameIt));
}

/** Returns the column name for a column specified via its index
    @param index : The column index
*/
QString WhiteList::name(int index) const { return m_names.at(index); }

/** Returns the algorithm property linked to a column specified via its index
    @param index : The column index
*/
QString WhiteList::algorithmProperty(int index) const {
  return m_algorithmProperties.at(index);
}

/** Returns the column description for a column specified via its index
    @param index : The column index
*/
QString WhiteList::description(int index) const {
  return m_descriptions.at(index);
}

/** Returns the size of this whitelist, i.e. the number of columns
 */
size_t WhiteList::size() const { return m_names.size(); }

/** Returns true if the contents of this column should be used to generate the
 * name of the output ws
 * @param index : The column index
 */
bool WhiteList::isShown(int index) const { return m_isShown.at(index); }

/** Check whether any of the columns are marked as a key column
 */
bool WhiteList::hasKeyColumns() const {
  return std::any_of(m_isKey.cbegin(), m_isKey.cend(),
                     [](const auto isKey) { return isKey; });
}

/** Returns true if the contents of this column should be used to identify the
 * row uniquely within the group
 * @param index : The column index
 */
bool WhiteList::isKey(int index) const { return m_isKey.at(index); }

/** Returns the column prefix used to generate the name of the output ws (will
 * only be used if showValue is true for this column
 * @param index : The column index
 */
QString WhiteList::prefix(int index) const { return m_prefixes.at(index); }

/** Returns the list of entry names.
 *  @returns The list of entry names ordered by column index. */
std::vector<QString> const &WhiteList::names() const { return m_names; }

auto WhiteList::end() const -> const_iterator { return cend(); }

auto WhiteList::begin() const -> const_iterator { return cbegin(); }

/// Returns a ForwardIterator pointing to the first entry in the whitelist.
auto WhiteList::cbegin() const -> const_iterator {
  return const_iterator(m_names.cbegin(), m_descriptions.cbegin(),
                        m_algorithmProperties.cbegin(), m_isShown.cbegin(),
                        m_prefixes.cbegin(), m_isKey.cbegin());
}

/// Returns a ForwardIterator pointing to one past the last entry in the
/// whitelist.
auto WhiteList::cend() const -> const_iterator {
  return const_iterator(m_names.cend(), m_descriptions.cend(),
                        m_algorithmProperties.cend(), m_isShown.cend(),
                        m_prefixes.cend(), m_isKey.cend());
}

ConstColumnIterator operator+(const ConstColumnIterator &lhs,
                              typename ConstColumnIterator::difference_type n) {
  auto result = lhs;
  result += n;
  return result;
}

ConstColumnIterator operator+(typename ConstColumnIterator::difference_type n,
                              const ConstColumnIterator &rhs) {
  auto result = rhs;
  result += n;
  return result;
}

ConstColumnIterator operator-(const ConstColumnIterator &lhs,
                              typename ConstColumnIterator::difference_type n) {
  auto result = lhs;
  result -= n;
  return result;
}

ConstColumnIterator operator-(typename ConstColumnIterator::difference_type n,
                              const ConstColumnIterator &rhs) {
  auto result = rhs;
  result -= n;
  return result;
}
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
