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
    int row()const{return m_row;}
    void row(int i);
    bool next();
    bool prev();
    void sep(const std::string& s){m_sep = s;}

    template<class T>
    TableRow& operator<<(const T& t)
    {
        TableColumn_ptr<T> c = m_columns[m_col];
        c->data()[m_row] = t;
        ++m_col;
        return *this;
    }
    TableRow& operator<<(const char* t){return operator<<(std::string(t));}
    TableRow& operator<<(bool t){return operator<<(Boolean(t));}

    template<class T>
    TableRow& operator>>(T& t)
    {
        TableColumn_ptr<T> c = m_columns[m_col];
        t = c->data()[m_row];
        ++m_col;
        return *this;
    }
    TableRow& operator>>(bool& t)
    {
        Boolean b;
        operator>>(b);
        t = b;
        return *this;
    }

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

    int& Int(int col){return cell<int>(col);}
    double& Double(int col){return cell<double>(col);}
    Boolean& Bool(int col){return cell<Boolean>(col);}
    std::string& String(int col){return cell<std::string>(col);}

private:
    friend TableRow_DllExport std::ostream& operator<<(std::ostream& s,const TableRow& row);
    std::vector< boost::shared_ptr<Column> >& m_columns;
    int m_row;
    int m_col;
    int m_nrows;
    std::string m_sep;
};

TableRow_DllExport std::ostream& operator<<(std::ostream& s,const TableRow& row);

} // namespace DataObjects
} // namespace Mantid

#endif  /*  MANTID_DATAOBJECTS_TABLEROW_H_ */
