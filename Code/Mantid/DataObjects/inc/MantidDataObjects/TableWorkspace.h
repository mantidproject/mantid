#ifndef MANTID_DATAOBJECTS_TABLEWORKSPACE_H_
#define MANTID_DATAOBJECTS_TABLEWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/System.h"
#include "MantidDataObjects/TableColumn.h"
#include "MantidDataObjects/TablePointerColumn.h"

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

namespace Mantid
{

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}

namespace DataObjects
{

class TableColumnHelper
{
public:
    TableColumnHelper(TableWorkspace *tw,const std::string& name):m_workspace(tw),m_name(name){}
    TableWorkspace *m_workspace;
    std::string m_name;
};

class TableRowHelper
{
public:
    TableRowHelper(TableWorkspace* tw,int row):m_workspace(tw),m_row(row){}
    TableWorkspace* m_workspace;
    int m_row;
};

/** \class TableWorkspace

     



    \author Roman Tolchenov
    \date 31/10/2008

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
#ifdef _WIN32
#ifdef IN_MANTID_DATA_OBJECTS
  #define TableWorkspace_DllExport __declspec( dllexport )
#else
  #define TableWorkspace_DllExport __declspec( dllimport )
#endif
#else
  #define TableWorkspace_DllExport
  #define TableWorkspace_DllImport
#endif

class TableWorkspace_DllExport TableWorkspace
{
public:
    /// Constructor.
    TableWorkspace(int nrows=0);
    /// Virtual destructor.
    virtual ~TableWorkspace();
    /// Creates a new column.
    bool createColumn(const std::string& type, const std::string& name);
    /// Removes a column.
    void removeColumn( const std::string& name);
    /// Number of columns in the workspace.
    int columnCount(){return m_columns.size();}
    /// Gets the shared pointer to a column.
    boost::shared_ptr<Column> getColumn(const std::string& name);
    /// Returns a vector of all column names.
    std::vector<std::string> getColumnNames();
    /// Number of rows in the workspace.
    int rowCount(){return m_rowCount;}
    /// Resizes the workspace.
    void setRowCount(int count);
    /// Inserts a row before row pointed to by index and fills it with default vales.
    int insertRow(int index);
    /// Delets a row if it exists.
    void removeRow(int index);
    /// Appends a row.
    TableRowHelper appendRow(){insertRow(rowCount());return getRow(rowCount()-1);}
    /// Gets the reference to the data vector.
    template <class T>
    std::vector<T>& getStdVector(const std::string& name)
    {
        TableColumn_ptr<T> c = getColumn(name);
        return c->data();
    }
    TableColumnHelper getVector(const std::string& name){return TableColumnHelper(this,name);}
    template <class T>
    T& getRef(const std::string& name, int index)
    {
        boost::shared_ptr<Column> c = getColumn(name);
        if (c->get_type_info() != typeid(T))
        {
            std::string str = "getRef: Type mismatch. ";
            g_log.error(str);
            throw std::runtime_error(str);
        }
        return *(static_cast<T*>(c->void_pointer(index)));
    }
    template <class P>// P is of the type of a pointer to data
    P getPointer(const std::string& name, int index)
    {
        boost::shared_ptr<Column> c = getColumn(name);
        if (c->get_pointer_type_info() != typeid(P))
        {
            std::string str = "getPointer: Type mismatch. ";
            g_log.error(str);
            throw std::runtime_error(str);
        }
        return static_cast<P>(c->void_pointer(index));
    }
    template<class T>
    T& cell(int row,int col)
    {
        TableColumn_ptr<T> c = m_columns[col];
        return c->data()[row];
        //return boost::static_pointer_cast<TableColumn<T> >(m_columns[col])->data()[row];
    }
    int& Int(int row,int col){return cell<int>(row,col);}
    double& Double(int row,int col){return cell<double>(row,col);}
    Boolean& Bool(int row,int col){return cell<Boolean>(row,col);}
    std::string& String(int row,int col){return cell<std::string>(row,col);}
    TableRowHelper getRow(int row){return TableRowHelper(this,row);}
    TableRowHelper getFirstRow(){return TableRowHelper(this,0);}


    /*/---------------- Tuples ---------------------------//
    template<typename Tuple>
    void set_Tuple(int j,Tuple& t,const std::vector<std::string>& names,int i=0)
    {
        boost::tuples::get<0>(t) = getRef<typename Tuple::head_type>(names[i],j);
        if (i == int(names.size()-1)) return;
        if (typeid(t.get_tail()) != typeid(boost::tuples::null_type))
        set_Tuple(j,t.get_tail(),names,i+1);
    }

    void set_Tuple(int j,boost::tuples::null_type t,const std::vector<std::string>& names,int i)
    {}

    template<typename Tuple>
    Tuple make_TupleRef(int j,const std::vector<std::string>& names,int i=0)
    {
        Tuple t;
        boost::tuples::get<0>(t) = getPointer<typename Tuple::head_type>(names[i],j);
        if (i < int(names.size()-1)&& typeid(t.get_tail()) != typeid(boost::tuples::null_type))
            t.get_tail() = make_TupleRef<typename Tuple::tail_type>(j,names,i+1);
        return t;
    }

    boost::tuples::null_type make_TupleRef(int j,const std::vector<std::string>& names,int i)
    {return boost::tuples::null_type();}

    template<typename Tuple>
    void set_TupleRef(int j,Tuple& t,const std::vector<std::string>& names,int i=0)
    {
      boost::tuples::get<0>(t) = getPointer<typename Tuple::head_type>(names[i],j);
        if (i < int(names.size()-1)&& typeid(t.get_tail()) != typeid(boost::tuples::null_type))
            set_TupleRef(j,t.get_tail(),names,i+1);
    }

    void set_TupleRef(int j,boost::tuples::null_type t,const std::vector<std::string>& names,int i)
    {}//*/

protected:

private:
    /// Used in std::find_if algorithm to find a Column with name name.
    class FindName
    {
        std::string m_name;
    public:
        FindName(const std::string& name):m_name(name){}
        bool operator()(boost::shared_ptr<Column>& cp)
        {
            return cp->name() == m_name;
        }
    };
    friend class TableRow;

    typedef std::vector< boost::shared_ptr<Column> >::iterator column_it;
    /// Logger
    static Kernel::Logger& g_log;
    /// Shared pointers to the columns.
    std::vector< boost::shared_ptr<Column> > m_columns;
    /// row count
    int m_rowCount;

};


template< class T>
class ColumnVector
{
public:
    ColumnVector(const TableColumnHelper& th):m_column(th.m_workspace->getColumn(th.m_name)){}
    T& operator[](size_t i){return m_column->data()[i];}
    int size(){return int(m_column->size());}
private:
    TableColumn_ptr<T> m_column;
};

template< class T>
class ColumnPointerVector
{
public:
    ColumnPointerVector(const TableColumnHelper& th):m_column(th.m_workspace->getColumn(th.m_name)){}
    T& operator[](size_t i){return m_column->data(i);}
    int size(){return int(m_column->size());}
private:
    TablePointerColumn_ptr<T> m_column;
};



} // namespace DataObjects
} // Namespace Mantid
#endif /*MANTID_DATAOBJECTS_TABLEWORKSPACE_H_*/
