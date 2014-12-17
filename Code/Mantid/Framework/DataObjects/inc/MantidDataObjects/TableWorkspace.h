#ifndef MANTID_DATAOBJECTS_TABLEWORKSPACE_H_
#define MANTID_DATAOBJECTS_TABLEWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidDataObjects/DllConfig.h"
#include "MantidDataObjects/TableColumn.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

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

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class MANTID_DATAOBJECTS_DLL TableWorkspace
    : virtual public API::ITableWorkspace {
public:
  /// Constructor.
  TableWorkspace(size_t nrows = 0);
  /// Virtual destructor.
  virtual ~TableWorkspace();
  /// Return the workspace typeID
  virtual const std::string id() const { return "TableWorkspace"; }
  /// Get the footprint in memory in KB.
  virtual size_t getMemorySize() const;
  /// Creates a new column.
  API::Column_sptr addColumn(const std::string &type, const std::string &name);
  /// Removes a column.
  void removeColumn(const std::string &name);
  /// Number of columns in the workspace.
  size_t columnCount() const { return static_cast<int>(m_columns.size()); }
  /// Gets the shared pointer to a column.
  API::Column_sptr getColumn(const std::string &name);
  API::Column_const_sptr getColumn(const std::string &name) const;
  /// Gets the shared pointer to a column.
  API::Column_sptr getColumn(size_t index);
  /// Gets the shared pointer to a column by index - return none-modifyable
  /// column.
  API::Column_const_sptr getColumn(size_t index) const;
  /// Returns a vector of all column names.
  std::vector<std::string> getColumnNames() const;
  /// Number of rows in the workspace.
  size_t rowCount() const { return m_rowCount; }
  /**Get access to shared pointer containing workspace porperties */
  API::LogManager_sptr logs() { return m_LogManager; }
  /**Get constant access to shared pointer containing workspace porperties */
  API::LogManager_const_sptr getLogs() const { return m_LogManager; }

  /** get access to column vecotor for index i.
   *
   *  The operation is unsafe with regards to the operaitons resizing obtained
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
  /** get constant access to column vecotor for index i. */
  template <class T> const std::vector<T> &getColVector(size_t index) const {
    auto pTableCol = dynamic_cast<TableColumn<T> *>(m_columns[index].get());
    if (pTableCol)
      return pTableCol->data();
    else {
      throw std::runtime_error("TableWorkspace::getColVector()const: Can not "
                               "cast to proper TableCol type");
    }
  }
  /** get access to the column vecotor for column with given name .
   *
   *  The operation is unsafe with regards to the operaitons resizing obtained
   *vector.
   *  This will destroy all table ws internal coherency. DO NOT ABUSE!
   *  e.g.: resise/reserve are unsafe
   *   Writing/reading data to vector through [] or at() is safe. */
  template <class T> std::vector<T> &getColVector(const std::string &name) {
    column_it ci =
        std::find_if(m_columns.begin(), m_columns.end(), FindName(name));
    if (ci == m_columns.end())
      throw(
          std::runtime_error("column with name: " + name + " does not exist"));
    auto pTableCol = dynamic_cast<TableColumn<T> *>(ci->get());
    if (pTableCol)
      return pTableCol->data();
    else {
      throw std::runtime_error("TableWorkspace::getColVector(): Can not cast "
                               "to proper TableCol type");
    }
  }
  /** get access to column vecotor for column with given name  */
  template <class T>
  const std::vector<T> &getColVector(const std::string &name) const {
    auto ci = std::find_if(m_columns.begin(), m_columns.end(), FindName(name));
    if (ci == m_columns.end())
      throw(
          std::runtime_error("column with name: " + name + " does not exist"));
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
      return NULL;
    auto pTableCol = dynamic_cast<TableColumn<T> *>(ci->get());
    if (pTableCol)
      return pTableCol->dataArray();
    else
      return NULL;
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
      return NULL;
    auto pTableCol = dynamic_cast<TableColumn<T> *>(ci->get());
    if (pTableCol)
      return pTableCol->dataArray();
    else
      return NULL;
  }

  /// Resizes the workspace.
  void setRowCount(size_t count);
  /// Inserts a row before row pointed to by index and fills it with default
  /// vales.
  size_t insertRow(size_t index);
  /// Delets a row if it exists.
  void removeRow(size_t index);
  /// Clone table workspace instance.
  TableWorkspace *clone() const;

  /** This method finds the row and column index of an integer cell value in a
   * table workspace
   * @param value :: -value to search
   * @param  row  row number of the value  searched
   * @param  col  column number of the value searched
   */
  virtual void find(size_t value, size_t &row, const size_t &col) {
    findValue(value, row, col);
  }
  /** This method finds the row and column index of an string cell value in a
   * table workspace
   * @param value :: -value to search
   * @param  row  row number of the value  searched
   * @param  col  column number of the value searched
   */
  virtual void find(std::string value, size_t &row, const size_t &col) {
    findValue(value, row, col);
  }
  /** This method finds the row and column index of an float value in a table
   * workspace
   * @param value :: -value to search
   * @param  row  row number of the value  searched
   * @param  col  column number of the value searched
   */
  virtual void find(float value, size_t &row, const size_t &col) {
    findValue(value, row, col);
  }
  /** This method finds the row and column index of an API::Bollean value in a
   * table workspace
   * @param value :: -value to search
   * @param  row  row number of the value  searched
   * @param  col  column number of the value searched
   */
  virtual void find(API::Boolean value, size_t &row, const size_t &col) {
    findValue(value, row, col);
  }
  /** This method finds the row and column index of an double cell value in a
   * table workspace
   * @param value :: -value to search
   * @param  row  row number of the value  searched
   * @param  col  column number of the value searched
   */
  virtual void find(double value, size_t &row, const size_t &col) {
    findValue(value, row, col);
  }
  /** This method finds the row and column index of an Mantid::Kernel::V3D cell
   * value in a table workspace
   * @param value :: -value to search
   * @param  row  row number of the value  searched
   * @param  col  column number of the value searched
   */
  void find(Mantid::Kernel::V3D value, size_t &row, const size_t &col) {
    findValue(value, row, col);
  }
  /** Casts cells through converting their values to/from double without type
   * checking;
   * Can produce stuped results in case if the type is in any way not related to
   * double */
  template <class U> U cell_cast(size_t nRow, size_t nCol) const {
    API::Column_sptr spCol = this->m_columns[nCol];
    return static_cast<U>(spCol->operator[](nRow));
  }
  template <class U>
  U cell_cast(size_t nRow, const std::string &col_name) const {
    API::Column_const_sptr spCol = this->getColumn(col_name);
    return static_cast<U>(spCol->operator[](nRow));
  }

  /// Sort this table. @see ITableWorkspace::sort
  void sort(std::vector<std::pair<std::string, bool>> &criteria);

private:
  /// template method to find a given value in a table.
  template <typename Type>
  void findValue(const Type value, size_t &row, const size_t &colIndex) {

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

  bool addColumn(boost::shared_ptr<API::Column> column);

  /** This method finds the row and column index of an integer cell value in a
  * table workspace
  * @param value :: -value to search
  * @param  row  row number of the value  searched
  * @param  col  column number of the value searched
  */
  virtual void find(size_t value, size_t &row, size_t &col) {
    findValue(value, row, col);
  }
  /** This method finds the row and column index of an string cell value in a
  * table workspace
  * @param value :: -value to search
  * @param  row  row number of the value  searched
  * @param  col  column number of the value searched
  */
  virtual void find(std::string value, size_t &row, size_t &col) {
    findValue(value, row, col);
  }
  /** This method finds the row and column index of an float value in a table
  * workspace
  * @param value :: -value to search
  * @param  row  row number of the value  searched
  * @param  col  column number of the value searched
  */
  virtual void find(float value, size_t &row, size_t &col) {
    findValue(value, row, col);
  }
  /** This method finds the row and column index of an API::Bollean value in a
  * table workspace
  * @param value :: -value to search
  * @param  row  row number of the value  searched
  * @param  col  column number of the value searched
  */
  virtual void find(API::Boolean value, size_t &row, size_t &col) {
    findValue(value, row, col);
  }
  /** This method finds the row and column index of an double cell value in a
  * table workspace
  * @param value :: -value to search
  * @param  row  row number of the value  searched
  * @param  col  column number of the value searched
  */
  virtual void find(double value, size_t &row, size_t &col) {
    findValue(value, row, col);
  }
  /** This method finds the row and column index of an Mantid::Kernel::V3D cell
  * value in a table workspace
  * @param value :: -value to search
  * @param  row  row number of the value  searched
  * @param  col  column number of the value searched
  */
  void find(Mantid::Kernel::V3D value, size_t &row, size_t &col) {
    findValue(value, row, col);
  }

private:
  /// Used in std::find_if algorithm to find a Column with name \a name.
  class FindName {
    std::string m_name; ///< Name to find
  public:
    /// Constructor
    FindName(const std::string &name) : m_name(name) {}
    /// Comparison operator
    bool operator()(boost::shared_ptr<API::Column> &cp) const {
      return cp->name() == m_name;
    }
    bool operator()(const boost::shared_ptr<const API::Column> &cp) const {
      return cp->name() == m_name;
    }
  };

  typedef std::vector<boost::shared_ptr<API::Column>>::iterator
      column_it; ///< Column iterator
  typedef std::vector<boost::shared_ptr<API::Column>>::const_iterator
      column_const_it; ///< Column const iterator
  /// Shared pointers to the columns.
  std::vector<boost::shared_ptr<API::Column>> m_columns;
  /// row count
  size_t m_rowCount;

  /// shared pointer to the logManager, responsible for the workspace
  /// properties.
  API::LogManager_sptr m_LogManager;

  // not asignable, not copy constructable, clonable
  /// Copy constructor
  TableWorkspace(const TableWorkspace &other);
  TableWorkspace &operator=(const TableWorkspace &rhs);
};

/// Typedef for a shared pointer to \c TableWorkspace
typedef boost::shared_ptr<TableWorkspace> TableWorkspace_sptr;
/// Typedef for a shared pointer to \c const \c TableWorkspace
typedef boost::shared_ptr<const TableWorkspace> TableWorkspace_const_sptr;

} // namespace DataObjects
} // Namespace Mantid
#endif /*MANTID_DATAOBJECTS_TABLEWORKSPACE_H_*/
