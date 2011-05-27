#ifndef MANTID_API_ITABLEWORKSPACE_H_
#define MANTID_API_ITABLEWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

//#include "MantidKernel/System.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Column.h"
#include "MantidGeometry/V3D.h"

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <sstream>

namespace Mantid
{

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}

namespace API
{

class ITableWorkspace;

/// Helper class used to create ColumnVector
class TableColumnHelper
{
public:
    /// Constructor
    TableColumnHelper(ITableWorkspace *tw,const std::string& name):m_workspace(tw),m_name(name){}
    ITableWorkspace *m_workspace;///< Pointer to the TableWorkspace
    std::string m_name;///< column namae
};

/// Helper class used to create TableRow
class TableRowHelper
{
public:
    /// Constructor
    TableRowHelper(ITableWorkspace* tw,int row):m_workspace(tw),m_row(row){}
    ITableWorkspace* m_workspace;///< Pointer to the TableWorkspace
    int m_row;///< Row number
};



/** \class ITableWorkspace

     ITableWorkspace is an implementation of Workspace in which the data are organised in columns of same size.
     Elements of a column have the same data type. Columns can be added to the TableWorkspace with
     ctreateColumn(type,name). name is a name given to the column. type is a symbolic name for the data type of the column. Predefined types are:
     - "int"    for int
     - "float"  for float
     - "double" for double
     - "bool"   for bool
     - "str"    for std::string
     - "V3D"    for Mantid::Geometry::V3D

     User defined types can be used after declaring them with DECLARE_TABLECOLUMN macro: 
     DECLARE_TABLECOLUMN(typeName, UserDefinedType)

     Ways to access the data:
       - Using templated cell method. E.g. SomeType var = table.cell<SomeType>(i,j); where j is the column number, i is 
         the position of the element in the column or the row number. The type of var must match the column's type,
         otherwise a runtime_error exception will be thrown. The columns are kept in the order of their creation.
       - Using specialized access methods for the predefined types. E.g. int var = table.Int(i,j);. If j-th column is
         of the wrong type a runtime_error exception will be thrown. 
       - Getting the pointer to a column and working with it.
       - Using ColumnVector returned by getVector()
       - Creating a TableRow object and working with it.


    \author Roman Tolchenov
    \date 31/10/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#ifdef _WIN32
#ifdef IN_MANTID_API
  #define ITableWorkspace_DllExport __declspec( dllexport )
#else
  #define ITableWorkspace_DllExport __declspec( dllimport )
#endif
#else
#define ITableWorkspace_DllExport
#define ITableWorkspace_DllImport
#endif



// =====================================================================================
class ITableWorkspace_DllExport ITableWorkspace: public API::Workspace
{
public:
  /// Virtual destructor.
  virtual ~ITableWorkspace(){}
  /// Return the workspace typeID
  virtual const std::string id() const{return "ITableWorkspace";}

  /** Creates a new column
   * @param type :: The datatype of the column
   * @param name :: The name to assign to the column
   * @return True if the column was successfully added
   */
  virtual bool addColumn(const std::string& type, const std::string& name) = 0;

  /// Creates n new columns of the same type.
  virtual bool addColumns(const std::string& type, const std::string& name, int n)
  {
    bool ok = true;
    for(int i=0;i<n;i++)
    {
      std::ostringstream ostr;
      ostr<<name<<'_'<<i;
      ok = ok && addColumn(type,ostr.str());
    }
    return ok;
  }

  /// Removes a column.
  virtual void removeColumn( const std::string& name) = 0;

  /// Number of columns in the workspace.
  virtual int columnCount() const = 0;

  /// Gets the shared pointer to a column by name.
  virtual boost::shared_ptr<Mantid::API::Column> getColumn(const std::string& name) = 0;

  /// Gets the shared pointer to a column by index.
  virtual boost::shared_ptr<Mantid::API::Column> getColumn(int index) = 0;

  /// Returns a vector of all column names.
  virtual std::vector<std::string> getColumnNames() = 0;

  /// Number of rows in the workspace.
  virtual int rowCount() const = 0;

  /// Resizes the workspace.
  virtual void setRowCount(int count) = 0;

  /// Inserts a row before row pointed to by index and fills it with default vales.
  virtual int insertRow(int index) = 0;

  /// Delets a row if it exists.
  virtual void removeRow(int index) = 0;

  /// Appends a row.
  TableRowHelper appendRow()
  {
    insertRow(rowCount());
    return getRow(rowCount()-1);
  }

  /// Access the column with name \c name trough a ColumnVector object
  TableColumnHelper getVector(const std::string& name)
  {
    return TableColumnHelper(this,name);
  }

  template <class T>
  /**  Get a reference to a data element
         @param name :: Column name.
         @param index :: Element's opsition in the column.
         @tparam T Type of the data in the column. If it doesn't match the actual type 
           a runtime_error exception is thrown.
         @return the reference to the data element
   */
  T& getRef(const std::string& name, int index)
  {
    boost::shared_ptr<Column> c = getColumn(name);
    if (!c->isType<T>())
    {
      std::string str = std::string("getRef: Type mismatch. ") +
          typeid(T).name() + " != " + c->get_type_info().name() + '\n';
      g_log.error(str);
      throw std::runtime_error(str);
    }
    return *(static_cast<T*>(c->void_pointer(index)));
  }

  /**  Get the reference to the element in row \c row and column \c col.
         @param row :: Row number
         @param col :: Column number
         @tparam T Type of the data in the column. If it doesn't match the actual type 
           a runtime_error exception is thrown.
         @return the reference to the requested cell
   */
  template<class T>
  T& cell(int row,int col)
  {
    Column_sptr c = getColumn(col);
    if (!c->isType<T>())
    {
      std::ostringstream ostr;
      ostr << "cell: Type mismatch:\n"<<typeid(T).name()<<" != \n"<<c->get_type_info().name()<<'\n';
      g_log.error(ostr.str());
      throw std::runtime_error(ostr.str());
    }
    if (row >= this->rowCount())
    {
      throw std::range_error("Table does not have row " + boost::lexical_cast<std::string>(row));
    }
    return *(static_cast<T*>(c->void_pointer(row)));
  }

  /**  Get the reference to the element in row \c row and column \c col if its type is \c int.
         If it doesn't match the actual type of the column a runtime_error exception is thrown.
         @param row :: Row number
         @param col :: Column number
         @return the reference of a requested cell if it's an integer
   */
  int& Int(int row,int col){return cell<int>(row,col);}
  /**  Get the reference to the element in row \c row and column \c col if its type is \c double.
         If it doesn't match the actual type of the column a runtime_error exception is thrown.
         @param row :: Row number
         @param col :: Column number
         @return the reference of a requested cell if it's a double
   */
  double& Double(int row,int col){return cell<double>(row,col);}
  /**  Get the reference to the element in row \c row and column \c col if its type is \c bool.
         If it doesn't match the actual type of the column a runtime_error exception is thrown.
         @param row :: Row number
         @param col :: Column number
         @return the reference of a requested cell if it's a boolean
   */
  Boolean& Bool(int row,int col){return cell<Boolean>(row,col);}
  /**  Get the reference to the element in row \a row and column \a col if its type is \c std::string.
         If it doesn't match the actual type of the column a runtime_error exception is thrown.
         @param row :: Row number
         @param col :: Column number
         @return the reference of a requested cell if it's a string
   */
  std::string& String(int row,int col){return cell<std::string>(row,col);}

  /**  Creates a TableRow object for row \a row.
         @param row :: Row number
         @return the requested row
   */
  TableRowHelper getRow(int row){return TableRowHelper(this,row);}
  /**  Creates a TableRow object for the first row (\c row == 0).
   */
  TableRowHelper getFirstRow(){return TableRowHelper(this,0);}

  /// find method to get the index of integer cell value in a table workspace
  virtual void find(int value,int& row,const int & col)=0;
  /// find method to get the index of  double cell value in a table workspace
  virtual void find (double  value,int& row,const int & col)=0;
  /// find method to get the index of  float cell value in a table workspace
  virtual void find(float value,int& row,const int & col)=0;
  /// find method to get the index of  API::Boolean value cell in a table workspace
  virtual void find(API::Boolean value,int& row,const int & col)=0;
  /// find method to get the index of cellstd::string  value in a table workspace
  virtual void find(std::string value,int& row,const int & col)=0;
  /// find method to get the index of  Mantid::Geometry::V3D cell value in a table workspace
  virtual void find(Mantid::Geometry::V3D value,int& row,const int & col)=0;

  void modified();

protected:

  /**  Resize a column.
         @param c :: Pointer to the column
         @param size :: New column size
   */
  void resizeColumn(Column* c,int size)
  {
    c->resize(size);
  }

  /**  Insert a new element into a column.
         @param c :: Pointer to the column
         @param index :: Index in the column before which a new element wil be inserted.
   */
  void insertInColumn(Column* c,int index)
  {
    c->insert(index);
  }

  /**  Remove an element from a column.
         @param c :: Pointer to the column
         @param index :: Index of the element to be removed.
   */
  void removeFromColumn(Column* c,int index)
  {
    c->remove(index);
  }

private:
  /// Logger
  static Kernel::Logger& g_log;
};



// =====================================================================================
/** @class ColumnVector
     ColumnVector gives access to the column elements without alowing its resizing.
     Created by TableWorkspace::getVector(...)
 */
template< class T>
class ColumnVector
{
public:
  /// Constructor
  ColumnVector(const TableColumnHelper& th):m_column(th.m_workspace->getColumn(th.m_name))
  {
    if (!m_column->isType<T>())
    {
      std::string str = "Type mismatch. ";
      m_column->g_log.error(str);
      throw std::runtime_error(str);
    }
  }
  /** Get the element
        @param i :: Element's position
        @return the column at the requested index
   */
  T& operator[](size_t i){return m_column->cell<T>(static_cast<int>(i));}
  /// Size of the vector
  int size(){return static_cast<int>(m_column->size());}
private:
  Column_sptr m_column;///< Pointer to the underlying column
};

/// Typedef for a shared pointer to \c TableWorkspace
typedef boost::shared_ptr<ITableWorkspace> ITableWorkspace_sptr;
/// Typedef for a shared pointer to \c const \c TableWorkspace
typedef boost::shared_ptr<const ITableWorkspace> ITableWorkspace_const_sptr;


} // namespace DataObjects
} // Namespace Mantid
#endif /*MANTID_API_ITABLEWORKSPACE_H_*/
