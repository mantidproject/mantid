#ifndef MANTID_DATAOBJECTS_TABLEROW_H_
#define MANTID_DATAOBJECTS_TABLEROW_H_

#include "MantidDataObjects/TableColumn.h"

#include <ostream>

namespace Mantid
{
namespace DataObjects
{

class TableRowHelper;

#ifdef _WIN32
#ifdef IN_MANTID_DATA_OBJECTS
  #define TableRow_DllExport __declspec( dllexport )
#else
  #define TableRow_DllExport __declspec( dllimport )
#endif
#else
  #define TableRow_DllExport
  #define TableRow_DllImport
#endif

/** \class TableRow

     TableRow represents a row in a TableWorkspace. The elements in the row can be accessed through
       a) tmplated cell method
       b) streaming operators (<< & >>)
       c) specialized access function: Int(c), Double(c), Bool(c), String(c)


    \author Roman Tolchenov
    \date 13/11/2008

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
class TableRow_DllExport TableRow
{
public:
    TableRow(const TableRowHelper& trh);
    /// Returns the row number of the TableRow 
    int row()const{return m_row;}
    void row(int i);
    bool next();
    bool prev();
    /// Sets a new separator character(s) between elements in a text output
    void sep(const std::string& s){m_sep = s;}

    /**  Input streaming operator. Makes TableRow look like a standard input stream.
         Every << operatr moves an internal pointer to the next element in the row.
         If the types of the input value and the current element don't match an exception is thrown.
         @param t Input value
         @return Self reference
     */
    template<class T>
    TableRow& operator<<(const T& t)
    {
        TableColumn_ptr<T> c = m_columns[m_col];
        c->data()[m_row] = t;
        ++m_col;
        return *this;
    }
    /// Special case of char*
    TableRow& operator<<(const char* t){return operator<<(std::string(t));}
    /// Special case of bool
    TableRow& operator<<(bool t){return operator<<(Boolean(t));}

    /**  Output streaming operator. Makes TableRow look like a standard output stream.
         Every >> operatr moves an internal pointer to the next element in the row.
         If the types of the receiving variable and the current element don't match an exception is thrown.
         @param t Variable for output
         @return Self reference
     */
    template<class T>
    TableRow& operator>>(T& t)
    {
        TableColumn_ptr<T> c = m_columns[m_col];
        t = c->data()[m_row];
        ++m_col;
        return *this;
    }
    /// Special case of bool
    TableRow& operator>>(bool& t)
    {
        Boolean b;
        operator>>(b);
        t = b;
        return *this;
    }

    /**  Templated method to access the element col in the row. The internal pointer moves to point to the element after col.
         @param col Element's position in the row
     */
    template<class T>
    T& cell(int col)
    {
        if (col < 0 || col >= int(m_columns.size()))
        {
            throw std::runtime_error("TableRow: column index out of range.");
        }
        m_col = col;
        TableColumn_ptr<T> c = m_columns[m_col];
        ++m_col; // Is it right?
        return c->data()[m_row];
    }

    /**  Returns a reference to the element in position col if its type is int
         @param col Position of the element
         @return Reference to the element
     */
    int& Int(int col){return cell<int>(col);}

    /**  Returns a reference to the element in position col if its type is double
         @param col Position of the element
         @return Reference to the element
     */
    double& Double(int col){return cell<double>(col);}

    /**  Returns a reference to the element in position col if its type is bool
         @param col Position of the element
         @return Reference to the element
     */
    Boolean& Bool(int col){return cell<Boolean>(col);}

    /**  Returns a reference to the element in position col if its type is std::string
         @param col Position of the element
         @return Reference to the element
     */
    std::string& String(int col){return cell<std::string>(col);}

private:
    friend TableRow_DllExport std::ostream& operator<<(std::ostream& s,const TableRow& row);
    std::vector< boost::shared_ptr<Column> >& m_columns;  ///< Pointers to the columns in the TableWorkspace
    int m_row;          ///< Row number in the TableWorkspace
    int m_col;          ///< Current column number (for streaming operations)
    int m_nrows;        ///< Number of rows in the TableWorkspace
    std::string m_sep;  ///< Separator character(s) between elements in a text output
};

TableRow_DllExport std::ostream& operator<<(std::ostream& s,const TableRow& row);

} // namespace DataObjects
} // namespace Mantid

#endif  /*  MANTID_DATAOBJECTS_TABLEROW_H_ */
