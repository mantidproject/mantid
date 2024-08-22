// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ColumnFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/Logger.h"

#include <queue>

namespace Mantid::DataObjects {
namespace {
/// static logger
Kernel::Logger g_log("TableWorkspace");

// struct to keep record on what rows to sort and according to which criteria
struct SortIterationRecord {
  SortIterationRecord(size_t ki, size_t is, size_t ie) : keyIndex(ki), iStart(is), iEnd(ie) {}
  size_t keyIndex; // index in criteria vector
  size_t iStart;   // start row to sort
  size_t iEnd;     // end row to sort (one past last)
};
} // namespace

DECLARE_WORKSPACE(TableWorkspace)

/// Constructor
TableWorkspace::TableWorkspace(size_t nrows) : ITableWorkspace(), m_rowCount(0), m_LogManager(new API::LogManager) {
  setRowCount(nrows);
}

TableWorkspace::TableWorkspace(const TableWorkspace &other)
    : ITableWorkspace(other), m_rowCount(0), m_LogManager(new API::LogManager) {
  setRowCount(other.m_rowCount);

  auto it = other.m_columns.cbegin();
  while (it != other.m_columns.cend()) {
    addColumn(std::shared_ptr<API::Column>((*it)->clone()));
    ++it;
  }
  // copy logs/properties.
  m_LogManager = std::make_shared<API::LogManager>(*(other.m_LogManager));
}

size_t TableWorkspace::getMemorySize() const {
  size_t data_size = std::accumulate(m_columns.cbegin(), m_columns.cend(), static_cast<size_t>(0),
                                     [](size_t sum, const auto &column) { return sum = column->sizeOfData(); });
  data_size += m_LogManager->getMemorySize();
  return data_size;
}

/** @param type :: Data type of the column.
    @param name :: Column name.
    @return A shared pointer to the created column (will be null on failure).
*/
API::Column_sptr TableWorkspace::addColumn(const std::string &type, const std::string &name) {
  API::Column_sptr c;
  if (type.empty()) {
    throw std::invalid_argument("Empty string passed as type argument of addColumn.");
  }
  if (name.empty()) {
    throw std::invalid_argument("Empty string passed as name argument of addColumn.");
  }
  // Check that there is no column with the same name.
  auto ci = std::find_if(m_columns.begin(), m_columns.end(), FindName(name));
  if (ci != m_columns.end()) {
    std::stringstream ss;
    ss << "Column with name " << name << " already exists.\n";
    throw std::invalid_argument(ss.str());
  }
  try {
    c = API::ColumnFactory::Instance().create(type);
    m_columns.emplace_back(c);
    c->setName(name);
    resizeColumn(c.get(), rowCount());
  } catch (Kernel::Exception::NotFoundError &e) {
    std::stringstream ss;
    ss << "Column of type " << type << " and name " << name << " has not been added.\n";
    ss << e.what() << '\n';
    throw std::invalid_argument(ss.str());
  }
  modified();
  return c;
}

/** If count is greater than the current number of rows extra rows are added to
   the bottom of the table.
    Otherwise rows at the end are erased to reach the new size.
    @param count :: New number of rows.
*/
void TableWorkspace::setRowCount(size_t count) {
  if (count == rowCount())
    return;
  for (auto &column : m_columns)
    resizeColumn(column.get(), count);
  m_rowCount = count;
}

/// Gets the shared pointer to a column.
API::Column_sptr TableWorkspace::getColumn(const std::string &name) {
  auto ci = std::find_if(m_columns.begin(), m_columns.end(), FindName(name));
  if (ci == m_columns.end()) {
    std::string str = "Column " + name + " does not exist.\n";
    throw std::runtime_error(str);
  }
  return *ci;
}
/// Gets the shared pointer to a column.
API::Column_const_sptr TableWorkspace::getColumn(const std::string &name) const {
  const auto found = std::find_if(m_columns.cbegin(), m_columns.cend(),
                                  [&name](const auto &column) { return column->name() == name; });
  if (found == m_columns.cend()) {
    throw std::runtime_error("Column " + name + " does not exist.");
  }
  return *found;
}

/// Gets the shared pointer to a column.
API::Column_sptr TableWorkspace::getColumn(size_t index) {
  if (index >= columnCount()) {
    std::stringstream ss;
    ss << "Column index is out of range: " << index << "(" << columnCount() << ")\n";
    throw std::range_error(ss.str());
  }
  return m_columns[index];
}

/// Gets the shared pointer to a column.
API::Column_const_sptr TableWorkspace::getColumn(size_t index) const {
  if (index >= columnCount()) {
    std::stringstream ss;
    ss << "Column index is out of range: " << index << "(" << columnCount() << ")\n";
    throw std::range_error(ss.str());
  }
  return m_columns[index];
}

void TableWorkspace::removeColumn(const std::string &name) {
  auto ci = std::find_if(m_columns.begin(), m_columns.end(), FindName(name));
  if (ci != m_columns.end()) {
    if (ci->use_count() > 1) {
      g_log.error() << "Deleting column in use (" << name << ").\n";
    }
    m_columns.erase(ci);
  }
  modified();
}

/** @param index :: Points where to insert the new row.
    @return Position of the inserted row.
*/
size_t TableWorkspace::insertRow(size_t index) {
  if (index >= rowCount())
    index = rowCount();
  for (auto &column : m_columns)
    insertInColumn(column.get(), index);
  ++m_rowCount;
  modified();
  return index;
}

/** @param index :: Row to delete.
 */
void TableWorkspace::removeRow(size_t index) {
  if (index >= rowCount()) {
    std::stringstream ss;
    ss << "Attempt to delete a non-existing row (" << index << ")\n";
    throw std::range_error(ss.str());
  }
  for (auto &column : m_columns)
    removeFromColumn(column.get(), index);
  --m_rowCount;
  modified();
}

std::vector<std::string> TableWorkspace::getColumnNames() const {
  std::vector<std::string> nameList;
  nameList.reserve(m_columns.size());
  std::transform(m_columns.cbegin(), m_columns.cend(), std::back_inserter(nameList),
                 [](const auto &column) { return column->name(); });
  return nameList;
}

void TableWorkspace::addColumn(const std::shared_ptr<API::Column> &column) {
  auto ci = std::find_if(m_columns.begin(), m_columns.end(), FindName(column->name()));
  if (ci != m_columns.end()) {
    std::stringstream ss;
    ss << "Column with name " << column->name() << " already exists.\n";
    throw std::invalid_argument(ss.str());
  } else {
    modified();
    m_columns.emplace_back(column);
  }
}

/**
 * Sort.
 * @param criteria : a vector with a list of pairs: column name, bool;
 *        where bool = true for ascending, false for descending sort.
 */
void TableWorkspace::sort(std::vector<std::pair<std::string, bool>> &criteria) {
  if (criteria.empty())
    return;

  if (criteria.size() > columnCount()) {
    throw std::runtime_error("Too many column names given.");
  }

  const size_t nRows = rowCount();
  if (nRows == 0)
    return;

  // first sort an array of indices according to criteria then put the rows
  // in order of the sorted indices

  std::vector<size_t> indexVec(nRows);
  // initialize the index vector with consecutive numbers
  for (auto i = indexVec.begin() + 1; i != indexVec.end(); ++i) {
    *i = *(i - 1) + 1;
  }

  // dynamically populate and use a queue of records for iteratively sort all
  // rows
  std::queue<SortIterationRecord> sortRecords;
  // start with first pair in criteria and sort all rows
  sortRecords.push(SortIterationRecord(0, 0, nRows));

  // maximum possible number of calls to Column::sortIndex (I think)
  const size_t maxNLoops = criteria.size() * nRows / 2;
  // loop over sortRecords and sort indexVec
  for (size_t counter = 0; counter < maxNLoops; ++counter) {
    // get the record from the front of the queue
    SortIterationRecord record = sortRecords.front();
    sortRecords.pop();
    // define the arguments for Column::sortIndex
    auto &crt = criteria[record.keyIndex];
    const std::string columnName = crt.first;
    const bool ascending = crt.second;
    auto &column = *getColumn(columnName);
    std::vector<std::pair<size_t, size_t>> equalRanges;

    // sort indexVec
    column.sortIndex(ascending, record.iStart, record.iEnd, indexVec, equalRanges);

    // if column had 1 or more ranges of equal values and there is next item in
    // criteria
    // add more records to the back of the queue
    if (record.keyIndex < criteria.size() - 1) {
      size_t keyIndex = record.keyIndex + 1;
      for (auto &equalRange : equalRanges) {
        sortRecords.push(SortIterationRecord(keyIndex, equalRange.first, equalRange.second));
      }
    }

    if (sortRecords.empty()) {
      // there is nothing to do
      break;
    }
  }

  // finally sort the rows
  const size_t nCols = columnCount();
  for (size_t i = 0; i < nCols; ++i) {
    getColumn(i)->sortValues(indexVec);
  }
  modified();
}

/// Clone the workspace keeping only selected columns.
/// @param colNames :: Names of columns to clone.
TableWorkspace *TableWorkspace::doCloneColumns(const std::vector<std::string> &colNames) const {
  if (colNames.empty()) {
    return new TableWorkspace(*this);
  }
  auto ws = new TableWorkspace();
  ws->setRowCount(rowCount());
  auto it = m_columns.cbegin();
  while (it != m_columns.cend()) {
    if (colNames.end() != std::find(colNames.begin(), colNames.end(), (**it).name())) {
      ws->addColumn(std::shared_ptr<API::Column>((*it)->clone()));
    }
    ++it;
  }
  // copy logs/properties.
  ws->m_LogManager = std::make_shared<API::LogManager>(*(m_LogManager));
  return ws;
}

//    template<>
//    boost::tuples::null_type TableWorkspace::make_TupleRef<
//    boost::tuples::null_type >(size_t j,const std::vector<std::string>&
//    names,size_t i)
//    {return boost::tuples::null_type();}

} // namespace Mantid::DataObjects

///\cond TEMPLATE
namespace Mantid::Kernel {
template <>
DLLExport DataObjects::TableWorkspace_sptr
IPropertyManager::getValue<DataObjects::TableWorkspace_sptr>(const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<DataObjects::TableWorkspace_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected shared_ptr<TableWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport DataObjects::TableWorkspace_const_sptr
IPropertyManager::getValue<DataObjects::TableWorkspace_const_sptr>(const std::string &name) const {
  auto *prop = dynamic_cast<PropertyWithValue<DataObjects::TableWorkspace_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected const shared_ptr<TableWorkspace>.";
    throw std::runtime_error(message);
  }
}

} // namespace Mantid::Kernel

///\endcond TEMPLATE
