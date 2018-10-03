// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_ITABLEWORKSPACE_H_
#define MANTID_API_ITABLEWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Column.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/LogManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/V3D.h"

#ifndef Q_MOC_RUN
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include <sstream>

namespace Mantid {

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------

namespace API {

/// Helper class used to create ColumnVector
class TableColumnHelper {
public:
  /// Constructor
  TableColumnHelper(ITableWorkspace *tw, const std::string &name)
      : m_workspace(tw), m_name(name) {}
  ITableWorkspace *m_workspace; ///< Pointer to the TableWorkspace
  std::string m_name;           ///< column namae
};

/// Helper class used to create ConstColumnVector
class TableConstColumnHelper {
public:
  /// Constructor
  TableConstColumnHelper(const ITableWorkspace *tw, const std::string &name)
      : m_workspace(tw), m_name(name) {}
  const ITableWorkspace *m_workspace; ///< Pointer to the TableWorkspace
  std::string m_name;                 ///< column namae
};

/// Helper class used to create TableRow
class TableRowHelper {
public:
  /// Constructor
  TableRowHelper(ITableWorkspace *tw, size_t row)
      : m_workspace(tw), m_row(row) {}
  ITableWorkspace *m_workspace; ///< Pointer to the TableWorkspace
  size_t m_row;                 ///< Row number
};

/** \class ITableWorkspace

     ITableWorkspace is an implementation of Workspace in which the data are
   organised in columns of same size.
     Elements of a column have the same data type. Columns can be added to the
   TableWorkspace with
     ctreateColumn(type,name). name is a name given to the column. type is a
   symbolic name for the data type of the column. Predefined types are:
     - "int"    for int
     - "float"  for float
     - "double" for double
     - "bool"   for bool
     - "str"    for std::string
     - "V3D"    for Mantid::Kernel::V3D

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
       - Using ColumnVector returned by getVector()
       - Creating a TableRow object and working with it.


    \author Roman Tolchenov
    \date 31/10/2008
*/

// =====================================================================================
class MANTID_API_DLL ITableWorkspace : public API::Workspace {
public:
  /// Constructor
  ITableWorkspace() {}

  /// Returns a clone of the workspace
  ITableWorkspace_uptr clone() const {
    return std::unique_ptr<ITableWorkspace>(doClone());
  }

  /// Returns a default-initialized clone of the workspace
  ITableWorkspace_uptr cloneEmpty() const {
    return ITableWorkspace_uptr(doCloneEmpty());
  }

  ITableWorkspace_uptr
  cloneColumns(const std::vector<std::string> &colNames) const;

  ITableWorkspace &operator=(const ITableWorkspace &) = delete;
  /// Return the workspace typeID
  const std::string id() const override { return "ITableWorkspace"; }
  const std::string toString() const override;
  /** Creates a new column
   * @param type :: The datatype of the column
   * @param name :: The name to assign to the column
   * @return True if the column was successfully added
   */
  virtual Column_sptr addColumn(const std::string &type,
                                const std::string &name) = 0;
  /// Creates n new columns of the same type.
  virtual bool addColumns(const std::string &type, const std::string &name,
                          size_t n);
  /**Get access to shared pointer containing workspace properties */
  virtual API::LogManager_sptr logs() = 0;
  /**Get constant access to shared pointer containing workspace properties */
  virtual API::LogManager_const_sptr getLogs() const = 0;

  /// Removes a column.
  virtual void removeColumn(const std::string &name) = 0;

  /// Number of columns in the workspace.
  virtual size_t columnCount() const = 0;

  /// Gets the shared pointer to a column by name.
  virtual Column_sptr getColumn(const std::string &name) = 0;

  /// Gets the shared pointer to a column by name.
  virtual Column_const_sptr getColumn(const std::string &name) const = 0;

  /// Gets the shared pointer to a column by index.
  virtual Column_sptr getColumn(size_t index) = 0;

  /// Gets the shared pointer to a column by index - return none-modifyable
  /// column.
  virtual Column_const_sptr getColumn(size_t index) const = 0;

  /// Returns a vector of all column names.
  virtual std::vector<std::string> getColumnNames() const = 0;

  /// Number of rows in the workspace.
  virtual size_t rowCount() const = 0;

  /// Resizes the workspace.
  virtual void setRowCount(size_t count) = 0;

  /// Inserts a row before row pointed to by index and fills it with default
  /// vales.
  virtual size_t insertRow(size_t index) = 0;

  /// Delets a row if it exists.
  virtual void removeRow(size_t index) = 0;

  /// Appends a row.
  TableRowHelper appendRow();

  /** Does this type of TableWorkspace need a custom sorting call (e.g.
   * PeaksWorkspace)
   * @return true if the workspace needs custom sorting calls */
  virtual bool customSort() const { return false; }

  /// Overridable method to custom-sort the workspace
  virtual void sort(std::vector<std::pair<std::string, bool>> &criteria);

  /// Access the column with name \c name trough a ColumnVector object
  TableColumnHelper getVector(const std::string &name);

  /// Access the column with name \c name trough a ColumnVector object
  TableConstColumnHelper getVector(const std::string &name) const;
  template <class T>
  /**  Get a reference to a data element
         @param name :: Column name.
         @param index :: Element's opsition in the column.
         @tparam T Type of the data in the column. If it doesn't match the
     actual type
           a runtime_error exception is thrown.
         @return the reference to the data element
   */
  T &getRef(const std::string &name, size_t index) {
    boost::shared_ptr<Column> c = getColumn(name);
    if (!c->isType<T>()) {
      std::string str = std::string("getRef: Type mismatch. ") +
                        typeid(T).name() + " != " + c->get_type_info().name() +
                        '\n';
      throw std::runtime_error(str);
    }
    return *(static_cast<T *>(c->void_pointer(index)));
  }

  /**  Get the reference to the element in row \c row and column \c col.
         @param row :: Row number
         @param col :: Column number
         @tparam T Type of the data in the column. If it doesn't match the
     actual type
           a runtime_error exception is thrown.
         @return the reference to the requested cell
   */
  template <class T> T &cell(size_t row, size_t col) {
    Column_sptr c = getColumn(col);
    if (!c->isType<T>()) {
      std::ostringstream ostr;
      ostr << "cell: Type mismatch:\n"
           << typeid(T).name() << " != \n"
           << c->get_type_info().name() << '\n';
      throw std::runtime_error(ostr.str());
    }
    if (row >= this->rowCount()) {
      throw std::range_error("Table does not have row " + std::to_string(row));
    }
    return *(static_cast<T *>(c->void_pointer(row)));
  }

  /**  Get the reference to the element in row \c row and column \c col if its
     type is \c int.
         If it doesn't match the actual type of the column a runtime_error
     exception is thrown.
         @param row :: Row number
         @param col :: Column number
         @return the reference of a requested cell if it's an integer
   */
  int &Int(size_t row, size_t col) { return cell<int>(row, col); }
  /**  Get the reference to the element in row \c row and column \c col if its
     type is \c double.
         If it doesn't match the actual type of the column a runtime_error
     exception is thrown.
         @param row :: Row number
         @param col :: Column number
         @return the reference of a requested cell if it's a double
   */
  double &Double(size_t row, size_t col) { return cell<double>(row, col); }
  /**  Get the reference to the element in row \c row and column \c col if its
     type is \c bool.
         If it doesn't match the actual type of the column a runtime_error
     exception is thrown.
         @param row :: Row number
         @param col :: Column number
         @return the reference of a requested cell if it's a boolean
   */
  Boolean &Bool(size_t row, size_t col) { return cell<Boolean>(row, col); }
  /**  Get the reference to the element in row \a row and column \a col if its
     type is \c std::string.
         If it doesn't match the actual type of the column a runtime_error
     exception is thrown.
         @param row :: Row number
         @param col :: Column number
         @return the reference of a requested cell if it's a string
   */
  std::string &String(size_t row, size_t col) {
    return cell<std::string>(row, col);
  }

  /**  Creates a TableRow object for row \a row.
         @param row :: Row number
         @return the requested row
   */
  TableRowHelper getRow(size_t row) { return TableRowHelper(this, row); }
  /**  Creates a TableRow object for the first row (\c row == 0).
   */
  TableRowHelper getFirstRow() { return TableRowHelper(this, 0); }

  /// find method to get the index of integer cell value in a table workspace
  virtual void find(size_t value, size_t &row, const size_t &col) = 0;
  /// find method to get the index of  double cell value in a table workspace
  virtual void find(double value, size_t &row, const size_t &col) = 0;
  /// find method to get the index of  float cell value in a table workspace
  virtual void find(float value, size_t &row, const size_t &col) = 0;
  /// find method to get the index of  API::Boolean value cell in a table
  /// workspace
  virtual void find(API::Boolean value, size_t &row, const size_t &col) = 0;
  /// find method to get the index of cellstd::string  value in a table
  /// workspace
  virtual void find(std::string value, size_t &row, const size_t &col) = 0;
  /// find method to get the index of  Mantid::Kernel::V3D cell value in a table
  /// workspace
  virtual void find(Mantid::Kernel::V3D value, size_t &row,
                    const size_t &col) = 0;

  void modified();

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  ITableWorkspace(const ITableWorkspace &) = default;

  /**  Resize a column.
         @param c :: Pointer to the column
         @param size :: New column size
   */
  void resizeColumn(Column *c, size_t size) { c->resize(size); }

  /**  Insert a new element into a column.
         @param c :: Pointer to the column
         @param index :: Index in the column before which a new element wil be
     inserted.
   */
  void insertInColumn(Column *c, size_t index) { c->insert(index); }

  /**  Remove an element from a column.
         @param c :: Pointer to the column
         @param index :: Index of the element to be removed.
   */
  void removeFromColumn(Column *c, size_t index) { c->remove(index); }

private:
  ITableWorkspace *doClone() const override {
    return doCloneColumns(std::vector<std::string>());
  }
  ITableWorkspace *doCloneEmpty() const override = 0;
  virtual ITableWorkspace *
  doCloneColumns(const std::vector<std::string> &colNames) const = 0;
};

// =====================================================================================
/** @class ColumnVector
     ColumnVector gives access to the column elements without alowing its
   resizing.
     Created by TableWorkspace::getVector(...)
 */
template <class T> class ColumnVector {
public:
  /// Constructor
  ColumnVector(const TableColumnHelper &th)
      : m_column(th.m_workspace->getColumn(th.m_name)) {
    if (!m_column->isType<T>()) {
      std::stringstream mess;
      mess << "Type mismatch when creating a ColumnVector<" << typeid(T).name()
           << ">.";
      throw std::runtime_error(mess.str());
    }
  }
  /// Construct directly from column
  ColumnVector(Column_sptr column) : m_column(column) {
    if (!m_column->isType<T>()) {
      std::stringstream mess;
      mess << "Type mismatch when creating a ColumnVector<" << typeid(T).name()
           << ">.";
      throw std::runtime_error(mess.str());
    }
  }
  /** Get the element
        @param i :: Element's position
        @return the column at the requested index
   */
  T &operator[](size_t i) { return m_column->cell<T>(i); }
  /// Size of the vector
  size_t size() { return (m_column->size()); }

private:
  Column_sptr m_column; ///< Pointer to the underlying column
};

/** @class ConstColumnVector
     ConstColumnVector gives const access to the column elements without alowing
   its resizing.
     Created by TableWorkspace::getVector(...)
 */
template <class T> class ConstColumnVector {
public:
  /// Constructor
  ConstColumnVector(const TableConstColumnHelper &th)
      : m_column(th.m_workspace->getColumn(th.m_name)) {
    if (!m_column->isType<T>()) {
      std::stringstream mess;
      mess << "Type mismatch when creating a ColumnVector<" << typeid(T).name()
           << ">.";
      throw std::runtime_error(mess.str());
    }
  }
  /// Construct directly from column
  ConstColumnVector(Column_const_sptr column) : m_column(column) {
    if (!m_column->isType<T>()) {
      std::stringstream mess;
      mess << "Type mismatch when creating a ColumnVector<" << typeid(T).name()
           << ">.";
      throw std::runtime_error(mess.str());
    }
  }
  /** Get the const reference to element
        @param i :: Element's position
        @return the column at the requested index
   */
  const T &operator[](size_t i) { return m_column->cell<T>(i); }
  /// Size of the vector
  size_t size() { return m_column->size(); }

private:
  Column_const_sptr m_column; ///< Pointer to the underlying column
};

} // namespace API
} // Namespace Mantid
#endif /*MANTID_API_ITABLEWORKSPACE_H_*/
