// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/DllConfig.h"
#include "MantidDataObjects/TableColumn.h"
#include "MantidDataObjects/TableWorkspace_fwd.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/V3D.h"
#include <boost/tuple/tuple.hpp>

#include <memory>
#include <utility>

namespace Mantid {

namespace DataObjects {
/** \class TableWorkspace

     TableWorkspace is an implementation of Workspace in which the data are
   organised in columns of same size.
     Elements of a column have the same data type. Columns can be added to the
   TableWorkspace with
     ctreateColumn(type,name). name is a name given to the column. type is a
   symbolic name for the data type of the column. Predefined types are:
     - "int"     for int
     - "int32_t" for int32_t
     - "size_t"  for size_t
     - "float"   for float
     - "double"  for double
     - "bool"    for bool
     - "str"     for std::string
     - "V3D"     for Mantid::Kernel::V3D

     User defined types can be used after declaring them with
   DECLARE_TABLECOLUMN macro:
     DECLARE_TABLECOLUMN(typeName, UserDefinedType)

     Ways to access the data:
       - Using templated cell method. E.g. SomeType var =
   table.cell<SomeType>(i,j); where j is the column number, i is
         the position of the element in the column or the row number. The type
   of var must match the column's type,
         otherwise a runtime_error exception will be thrown. The columns are
   kept in the order of their creation.
       - Using specialized access methods for the predefined types. E.g. int var
   = table.Int(i,j);. If j-th column is
         of the wrong type a runtime_error exception will be thrown.
       - Getting the pointer to a column and working with it.
       - Creating a TableRow object and working with it.


    \author Roman Tolchenov
    \date 31/10/2008
*/

class MANTID_DATAOBJECTS_DLL TableWorkspace : public API::ITableWorkspace {
public:
  /// Constructor.
  TableWorkspace(size_t nrows = 0);

  TableWorkspace &operator=(const TableWorkspace &other) = delete;

  /// Returns a clone of the workspace
  std::unique_ptr<TableWorkspace> clone() const { return std::unique_ptr<TableWorkspace>(doClone()); }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<TableWorkspace> cloneEmpty() const { return std::unique_ptr<TableWorkspace>(doCloneEmpty()); }

  /// Return the workspace typeID
  const std::string id() const override { return "TableWorkspace"; }
  /// Get the footprint in memory in KB.
  size_t getMemorySize() const override;
  /// Creates a new column.
  API::Column_sptr addColumn(const std::string &type, const std::string &name) override;
  /// Removes a column.
  void removeColumn(const std::string &name) override;
  /// Number of columns in the workspace.
  size_t columnCount() const override { return static_cast<int>(m_columns.size()); }
  /// Gets the shared pointer to a column.
  API::Column_sptr getColumn(const std::string &name) override;
  API::Column_const_sptr getColumn(const std::string &name) const override;
  /// Gets the shared pointer to a column.
  API::Column_sptr getColumn(size_t index) override;
  /// Gets the shared pointer to a column by index - return none-modifyable
  /// column.
  API::Column_const_sptr getColumn(size_t index) const override;
  /// Returns a vector of all column names.
  std::vector<std::string> getColumnNames() const override;
  /// Number of rows in the workspace.
  size_t rowCount() const override { return m_rowCount; }
  /**Get access to shared pointer containing workspace porperties */
  API::LogManager_sptr logs() override { return m_LogManager; }
  /**Get constant access to shared pointer containing workspace porperties */
  API::LogManager_const_sptr getLogs() const override { return m_LogManager; }

  /** get access to column vector for index i.
   *
   *  The operation is unsafe with regards to the operations resizing obtained
   *vector.
   *   This will destroy all table ws internal coherency. DO NOT ABUSE!
   *  e.g.: resise/reserve are unsafe
   *   Writing/reading data to vector through [] or at() is safe. */
  template <class T> std::vector<T> &getColVector(size_t index) {
    auto pTableCol = dynamic_cast<TableColumn<T> *>(m_columns[index].get());
    if (pTableCol)
      return pTableCol->data();
    else {
      throw std::runtime_error("TableWorkspace::getColVector(): Can not cast "
                               "to proper TableCol type");
    }
  }
  /** get constant access to column vector for index i. */
  template <class T> const std::vector<T> &getColVector(size_t index) const {
    auto pTableCol = dynamic_cast<TableColumn<T> *>(m_columns[index].get());
    if (pTableCol)
      return pTableCol->data();
    else {
      throw std::runtime_error("TableWorkspace::getColVector()const: Can not "
                               "cast to proper TableCol type");
    }
  }
  /** get access to the column vector for column with given name .
   *
   *  The operation is unsafe with regards to the operations resizing obtained
   *vector.
   *  This will destroy all table ws internal coherency. DO NOT ABUSE!
   *  e.g.: resise/reserve are unsafe
   *   Writing/reading data to vector through [] or at() is safe. */
  template <class T> std::vector<T> &getColVector(const std::string &name) {
    auto ci = std::find_if(m_columns.begin(), m_columns.end(), FindName(name));
    if (ci == m_columns.end())
      throw(std::runtime_error("column with name: " + name + " does not exist"));
    auto pTableCol = dynamic_cast<TableColumn<T> *>(ci->get());
    if (pTableCol)
      return pTableCol->data();
    else {
      throw std::runtime_error("TableWorkspace::getColVector(): Can not cast "
                               "to proper TableCol type");
    }
  }
  /** get access to column vector for column with given name  */
  template <class T> const std::vector<T> &getColVector(const std::string &name) const {
    auto ci = std::find_if(m_columns.begin(), m_columns.end(), FindName(name));
    if (ci == m_columns.end())
      throw(std::runtime_error("column with name: " + name + " does not exist"));
    auto pTableCol = dynamic_cast<TableColumn<T> *>(ci->get());
    if (pTableCol)
      return pTableCol->data();
    else {
      throw std::runtime_error("TableWorkspace::getColVector()const: Can not "
                               "cast to proper TableCol type");
    }
  }
  /**Non-throwing access to the pointer to the column data array for the column
   * with given name. Returns null on error or if the coulmn has not been found
   * No checks if one tries to use pointer to work out of the array limits are
   * performed; The pointer has to be received right before usage as
   * underlying vectoor changes within the table workspace immidiately make
   * this pointer invalid. Nasty method. Use only if no choice.      */
  template <class T> T *getColDataArray(const std::string &name) {
    auto ci = std::find_if(m_columns.begin(), m_columns.end(), FindName(name));
    if (ci == m_columns.end())
      return nullptr;
    auto pTableCol = dynamic_cast<TableColumn<T> *>(ci->get());
    if (pTableCol)
      return pTableCol->dataArray();
    else
      return nullptr;
  }
  /**Non-throwing const access to the pointer to the column data array for the
   * column with given name. Returns null on error or if the coulmn has not been
   * found
   * No checks if one tries to use pointer to work out of the array limits are
   * performed; The pointer has to be received right before usage as
   * underlying vectoor changes within the table workspace immidiately make
   * this pointer invalid. Nasty method. Use only if no choice.      */
  template <class T> T *getColDataArray(const std::string &name) const {
    auto ci = std::find_if(m_columns.begin(), m_columns.end(), FindName(name));
    if (ci == m_columns.end())
      return nullptr;
    auto pTableCol = dynamic_cast<TableColumn<T> *>(ci->get());
    if (pTableCol)
      return pTableCol->dataArray();
    else
      return nullptr;
  }

  /// Resizes the workspace.
  void setRowCount(size_t count) final; // Marked as final because used in constructors
  /// Inserts a row before row pointed to by index and fills it with default
  /// vales.
  size_t insertRow(size_t index) override;
  /// Delets a row if it exists.
  void removeRow(size_t index) override;

  /** This method finds the row and column index of an integer cell value in a
   * table workspace
   * @param value :: -value to search
   * @param  row  row number of the value  searched
   * @param  col  column number of the value searched
   */
  void find(size_t value, size_t &row, size_t col) override { findValue(value, row, col); }
  /** This method finds the row and column index of an string cell value in a
   * table workspace
   * @param value :: -value to search
   * @param  row  row number of the value  searched
   * @param  col  column number of the value searched
   */
  void find(const std::string &value, size_t &row, size_t col) override { findValue(value, row, col); }
  /** This method finds the row and column index of an float value in a table
   * workspace
   * @param value :: -value to search
   * @param  row  row number of the value  searched
   * @param  col  column number of the value searched
   */
  void find(float value, size_t &row, size_t col) override { findValue(value, row, col); }
  /** This method finds the row and column index of an API::Bollean value in a
   * table workspace
   * @param value :: -value to search
   * @param  row  row number of the value  searched
   * @param  col  column number of the value searched
   */
  void find(API::Boolean value, size_t &row, size_t col) override { findValue(value, row, col); }
  /** This method finds the row and column index of an double cell value in a
   * table workspace
   * @param value :: -value to search
   * @param  row  row number of the value  searched
   * @param  col  column number of the value searched
   */
  void find(double value, size_t &row, size_t col) override { findValue(value, row, col); }
  /** This method finds the row and column index of an Mantid::Kernel::V3D cell
   * value in a table workspace
   * @param value :: -value to search
   * @param  row  row number of the value  searched
   * @param  col  column number of the value searched
   */
  void find(const Mantid::Kernel::V3D &value, size_t &row, size_t col) override { findValue(value, row, col); }
  /** Casts cells through converting their values to/from double without type
   * checking;
   * Can produce stuped results in case if the type is in any way not related to
   * double */
  template <class U> U cell_cast(size_t nRow, size_t nCol) const {
    API::Column_sptr spCol = this->m_columns[nCol];
    return static_cast<U>(spCol->operator[](nRow));
  }
  template <class U> U cell_cast(size_t nRow, const std::string &col_name) const {
    API::Column_const_sptr spCol = this->getColumn(col_name);
    return static_cast<U>(spCol->operator[](nRow));
  }

  /// Sort this table. @see ITableWorkspace::sort
  void sort(std::vector<std::pair<std::string, bool>> &criteria) override;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  TableWorkspace(const TableWorkspace &other);

private:
  TableWorkspace *doClone() const override { return doCloneColumns(std::vector<std::string>()); }

  TableWorkspace *doCloneEmpty() const override { return new TableWorkspace(); }

  TableWorkspace *doCloneColumns(const std::vector<std::string> &colNames) const override;

  /// template method to find a given value in a table.
  template <typename Type> void findValue(const Type value, size_t &row, size_t colIndex) {

    try {
      TableColumn_ptr<Type> tc_sptr = getColumn(colIndex);
      std::vector<Type> dataVec = tc_sptr->data();
      typename std::vector<Type>::iterator itr;
      itr = std::find(dataVec.begin(), dataVec.end(), value);
      if (itr != dataVec.end()) {
        std::vector<int>::difference_type pos;
        pos = std::distance(dataVec.begin(), itr);
        // size_t pos=static_cast<int>itr-dataVec.begin();
        row = static_cast<int>(pos);

      } else {
        throw std::out_of_range("Search object not found in table workspace");
      }
    } catch (std::range_error &) {
      throw;
    } catch (std::runtime_error &) {
      throw;
    }
  }

  void addColumn(const std::shared_ptr<API::Column> &column);

private:
  /// Used in std::find_if algorithm to find a Column with name \a name.
  class FindName {
    std::string m_name; ///< Name to find
  public:
    /// Constructor
    FindName(const std::string &name) : m_name(name) {}
    /// Comparison operator
    bool operator()(std::shared_ptr<API::Column> &cp) const { return cp->name() == m_name; }
    bool operator()(const std::shared_ptr<const API::Column> &cp) const { return cp->name() == m_name; }
  };

  using column_it = std::vector<std::shared_ptr<API::Column>>::iterator; ///< Column
                                                                         ///< iterator

  ///< Column const iterator
  using column_const_it = std::vector<std::shared_ptr<API::Column>>::const_iterator;

  /// Shared pointers to the columns.
  std::vector<std::shared_ptr<API::Column>> m_columns;
  /// row count
  size_t m_rowCount;

  /// shared pointer to the logManager, responsible for the workspace
  /// properties.
  API::LogManager_sptr m_LogManager;
};
} // namespace DataObjects
} // namespace Mantid
