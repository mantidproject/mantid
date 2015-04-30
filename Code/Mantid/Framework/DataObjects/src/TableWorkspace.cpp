#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/ColumnFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <iostream>
#include <queue>

namespace Mantid {
namespace DataObjects {
namespace {
/// static logger
Kernel::Logger g_log("TableWorkspace");

// struct to keep record on what rows to sort and according to which criteria
struct SortIterationRecord {
  SortIterationRecord(size_t ki, size_t is, size_t ie)
      : keyIndex(ki), iStart(is), iEnd(ie) {}
  size_t keyIndex; // index in criteria vector
  size_t iStart;   // start row to sort
  size_t iEnd;     // end row to sort (one past last)
};
}

DECLARE_WORKSPACE(TableWorkspace)

/// Constructor
TableWorkspace::TableWorkspace(size_t nrows)
    : ITableWorkspace(), m_rowCount(0), m_LogManager(new API::LogManager) {
  setRowCount(nrows);
}

/// Destructor
TableWorkspace::~TableWorkspace() {}

size_t TableWorkspace::getMemorySize() const {
  size_t data_size = 0;
  for (column_const_it c = m_columns.begin(); c != m_columns.end(); c++) {
    data_size += (*c)->sizeOfData();
  }
  data_size += m_LogManager->getMemorySize();
  return data_size;
}

/** @param type :: Data type of the column.
    @param name :: Column name.
    @return A shared pointer to the created column (will be null on failure).
*/
API::Column_sptr TableWorkspace::addColumn(const std::string &type,
                                           const std::string &name) {
  API::Column_sptr c;
  if (type.empty()) {
    g_log.error("Empty string passed as type argument of createColumn.");
    return c;
  }
  if (name.empty()) {
    g_log.error("Empty string passed as name argument of createColumn.");
    return c;
  }
  // Check that there is no column with the same name.
  column_it ci =
      std::find_if(m_columns.begin(), m_columns.end(), FindName(name));
  if (ci != m_columns.end()) {
    g_log.error() << "Column with name " << name << " already exists.\n";
    return c;
  }
  try {
    c = API::ColumnFactory::Instance().create(type);
    m_columns.push_back(c);
    c->setName(name);
    resizeColumn(c.get(), rowCount());
  } catch (Kernel::Exception::NotFoundError &e) {
    g_log.error() << "Column of type " << type << " and name " << name
                  << " has not been created.\n";
    g_log.error() << e.what() << '\n';
    return c;
  }
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
  for (column_it ci = m_columns.begin(); ci != m_columns.end(); ci++)
    resizeColumn(ci->get(), count);
  m_rowCount = count;
}

/// Gets the shared pointer to a column.
API::Column_sptr TableWorkspace::getColumn(const std::string &name) {
  column_it ci =
      std::find_if(m_columns.begin(), m_columns.end(), FindName(name));
  if (ci == m_columns.end()) {
    std::string str = "Column " + name + " does not exist.\n";
    g_log.error(str);
    throw std::runtime_error(str);
  }
  return *ci;
}
/// Gets the shared pointer to a column.
API::Column_const_sptr
TableWorkspace::getColumn(const std::string &name) const {
  column_const_it c_it = m_columns.begin();
  column_const_it c_end = m_columns.end();
  for (; c_it != c_end; c_it++) {
    if (c_it->get()->name() == name) {
      return *c_it;
    }
  }
  std::string str = "Column " + name + " does not exist.\n";
  g_log.error(str);
  throw std::runtime_error(str);
}

/// Gets the shared pointer to a column.
API::Column_sptr TableWorkspace::getColumn(size_t index) {
  if (index >= columnCount()) {
    std::string str = "Column index is out of range";
    g_log.error() << str << ": " << index << "(" << columnCount() << ")\n";
    throw std::range_error(str);
  }
  return m_columns[index];
}

/// Gets the shared pointer to a column.
API::Column_const_sptr TableWorkspace::getColumn(size_t index) const {
  if (index >= columnCount()) {
    std::string str = "Column index is out of range";
    g_log.error() << str << ": " << index << "(" << columnCount() << ")\n";
    throw std::range_error(str);
  }
  return m_columns[index];
}

void TableWorkspace::removeColumn(const std::string &name) {
  column_it ci =
      std::find_if(m_columns.begin(), m_columns.end(), FindName(name));
  if (ci != m_columns.end()) {
    if (!ci->unique()) {
      g_log.error() << "Deleting column in use (" << name << ").\n";
    }
    m_columns.erase(ci);
  }
}

/** @param index :: Points where to insert the new row.
    @return Position of the inserted row.
*/
size_t TableWorkspace::insertRow(size_t index) {
  if (index >= rowCount())
    index = rowCount();
  for (column_it ci = m_columns.begin(); ci != m_columns.end(); ci++)
    insertInColumn(ci->get(), index);
  ++m_rowCount;
  return index;
}

/** @param index :: Row to delete.
*/
void TableWorkspace::removeRow(size_t index) {
  if (index >= rowCount()) {
    g_log.error() << "Attempt to delete a non-existing row (" << index << ")\n";
    return;
  }
  for (column_it ci = m_columns.begin(); ci != m_columns.end(); ci++)
    removeFromColumn(ci->get(), index);
  --m_rowCount;
}

std::vector<std::string> TableWorkspace::getColumnNames() const {
  std::vector<std::string> nameList;
  nameList.reserve(m_columns.size());
  for (auto ci = m_columns.begin(); ci != m_columns.end(); ci++)
    nameList.push_back((*ci)->name());
  return nameList;
}

bool TableWorkspace::addColumn(boost::shared_ptr<API::Column> column) {
  column_it ci = std::find_if(m_columns.begin(), m_columns.end(),
                              FindName(column->name()));
  if (ci != m_columns.end()) {
    g_log.error() << "Column with name " << column->name()
                  << " already exists.\n";
    return false;
  } else {
    m_columns.push_back(column);
  }
  return true;
}

TableWorkspace *TableWorkspace::clone() const {
  TableWorkspace *copy = new TableWorkspace(this->m_rowCount);
  column_const_it it = m_columns.begin();
  while (it != m_columns.end()) {
    copy->addColumn(boost::shared_ptr<API::Column>((*it)->clone()));
    it++;
  }
  // copy logs/properties.
  copy->m_LogManager = boost::make_shared<API::LogManager>(*this->m_LogManager);
  return copy;
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
    column.sortIndex(ascending, record.iStart, record.iEnd, indexVec,
                     equalRanges);

    // if column had 1 or more ranges of equal values and there is next item in
    // criteria
    // add more records to the back of the queue
    if (record.keyIndex < criteria.size() - 1) {
      size_t keyIndex = record.keyIndex + 1;
      for (auto range = equalRanges.begin(); range != equalRanges.end();
           ++range) {
        sortRecords.push(
            SortIterationRecord(keyIndex, range->first, range->second));
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
}

//    template<>
//    boost::tuples::null_type TableWorkspace::make_TupleRef<
//    boost::tuples::null_type >(size_t j,const std::vector<std::string>&
//    names,size_t i)
//    {return boost::tuples::null_type();}

} // namespace DataObjects
} // namespace Mantid

///\cond TEMPLATE
namespace Mantid {
namespace Kernel {
template <>
DLLExport DataObjects::TableWorkspace_sptr
IPropertyManager::getValue<DataObjects::TableWorkspace_sptr>(
    const std::string &name) const {
  PropertyWithValue<DataObjects::TableWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<DataObjects::TableWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message = "Attempt to assign property " + name +
                          " to incorrect type. Expected TableWorkspace.";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
