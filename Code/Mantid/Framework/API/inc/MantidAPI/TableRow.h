#ifndef MANTID_API_TABLEROW_H_
#define MANTID_API_TABLEROW_H_

#include "MantidAPI/Column.h"

#ifndef Q_MOC_RUN
# include <boost/lexical_cast.hpp>
#endif

#include <ostream>
#include <vector>
#include <stdexcept>

namespace Mantid
{

namespace API
{
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class TableRowHelper;

/** \class TableRow

     TableRow represents a row in a TableWorkspace. The elements in the row can be accessed through
       a) tmplated cell method
       b) streaming operators (<< & >>)
       c) specialized access function: Int(c), Double(c), Bool(c), String(c)


    \author Roman Tolchenov
    \date 13/11/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class MANTID_API_DLL TableRow
{
public:
    TableRow(const TableRowHelper& trh);
    /// Returns the row number of the TableRow 
    size_t row()const{return m_row;}
    /// Returns the number of rows in the TableWorkspace
    size_t size()const{return m_nrows;}
    void row(size_t i);
    bool next();
    bool prev();
    /// Sets a new separator character(s) between elements in a text output
    void sep(const std::string& s){m_sep = s;}

    /**  Input streaming operator. Makes TableRow look like a standard input stream.
         Every << operatr moves an internal pointer to the next element in the row.
         If the types of the input value and the current element don't match an exception is thrown.
         @param t :: Input value
         @return Self reference
     */
    template<class T>
    TableRow& operator<<(const T& t)
    {
        if (m_col >= m_columns.size())
        {
          std::stringstream errss;
          errss << "Column index " << m_col << " is out of range " << m_columns.size()
                << " of operator << ";
          throw std::range_error(errss.str());
        }
        Column_sptr c = m_columns[m_col];
        if (!c->isType<T>())
        {
            std::string str = "Type mismatch. ";
            throw std::runtime_error(str);
        }
        c->cell<T>(m_row) = t;
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
         @param t :: Variable for output
         @return Self reference
     */
    template<class T>
    const TableRow& operator>>(T& t)const
    {
        if (m_col >= m_columns.size())
        {
          std::stringstream errss;
          errss << "Column index " << m_col << " is out of range " << m_columns.size()
                << " of operator >> ";
          throw std::range_error(errss.str());
        }
        Column_sptr c = m_columns[m_col];
        if (!c->isType<T>())
        {
            throw std::runtime_error("TableRow type mismatch.");
        }
        t = c->cell<T>(m_row);
        ++m_col;
        return *this;
    }

    /// Special case of bool
    const TableRow& operator>>(bool& t)const;

    /**  Templated method to access the element col in the row. The internal pointer moves to point to the element after col.
         @param col :: Element's position in the row
         @return cell of the template type
     */
    template<class T>
    T& cell(size_t col)
    {
        if (col >= m_columns.size())
        {
          std::stringstream errss;
          errss << "Column index " << m_col << " is out of range " << m_columns.size()
                << " of method cell(). ";
          throw std::range_error(errss.str());
        }
        m_col = col;
        Column_sptr c = m_columns[m_col];
        ++m_col; // Is it right?
        return c->cell<T>(m_row);
    }

    /**  Returns a reference to the element in position col if its type is int
         @param col :: Position of the element
         @return Reference to the element
     */
    int& Int(size_t col){return cell<int>(col);}

    /**  Returns a reference to the element in position col if its type is double
         @param col :: Position of the element
         @return Reference to the element
     */
    double& Double(size_t col){return cell<double>(col);}

    /**  Returns a reference to the element in position col if its type is bool
         @param col :: Position of the element
         @return Reference to the element
     */
    Boolean& Bool(size_t col){return cell<Boolean>(col);}

    /**  Returns a reference to the element in position col if its type is std::string
         @param col :: Position of the element
         @return Reference to the element
     */
    std::string& String(size_t col){return cell<std::string>(col);}

private:
    friend MANTID_API_DLL std::ostream& operator<<(std::ostream& s,const TableRow& row);
    std::vector< Column_sptr > m_columns;  ///< Pointers to the columns in the ITableWorkspace
    size_t m_row;          ///< Row number in the TableWorkspace
    mutable size_t m_col;          ///< Current column number (for streaming operations)
    size_t m_nrows;        ///< Number of rows in the TableWorkspace
    std::string m_sep;  ///< Separator character(s) between elements in a text output
};

MANTID_API_DLL std::ostream& operator<<(std::ostream& s,const TableRow& row);

} // namespace API
} // namespace Mantid

#endif  /*  MANTID_API_TABLEROW_H_ */
