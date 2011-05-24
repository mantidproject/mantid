#ifndef MANTID_DATAOBJECTS_TABLECOLUMN_H_
#define MANTID_DATAOBJECTS_TABLECOLUMN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidAPI/Column.h"
#include "MantidKernel/Logger.h"

#include <vector>
#include <typeinfo>
#include <stdexcept>
#include <boost/shared_ptr.hpp>

namespace Mantid
{

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------

namespace DataObjects
{

template< class T>
class TableVector;

/** \class TableColumn

    Class TableColumn implements abstract class Column for any copyable data type.
    A TableColumn is created using TableWorkspace::addColumn(type,name).
    type is the simbolic name of the data type which must be first declared with
    DECLARE_TABLECOLUMN macro. Predeclared types are:

    "int"    for int
    "float"  for float
    "double" for double
    "bool"   for Boolean
    "str"    for std::string
    "V3D"    for Mantid::Geometry::V3D

    Boolean is used instead of bool because of bool's non-standard treatmemt in std::vector.

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
template <class Type>
class DLLExport TableColumn: public API::Column
{
public:

    //TableColumn();
    /// Virtual destructor.
    virtual ~TableColumn(){}
    /// Number of individual elements in the column.
    size_t size()const{return m_data.size();}
    /// Type id of the data in the column
    const std::type_info& get_type_info()const{return typeid(Type);}
    /// Type id of the pointer to data in the column
    const std::type_info& get_pointer_type_info()const{return typeid(Type*);}
    /// Output to an ostream.
    void print(std::ostream& s, size_t index)const{s << m_data[index];}
    /// Type check
    bool isBool()const{return typeid(Type) == typeid(API::Boolean);}
    /// Memory used by the column
    long int sizeOfData()const{return static_cast<long int>(m_data.size()*sizeof(Type));}

    /// Reference to the data.
    std::vector<Type>& data(){return m_data;}
protected:
    /// Resize.
    void resize(size_t count){m_data.resize(count);}
    /// Inserts default value at position index. 
    void insert(size_t index)
    {
        if (index < m_data.size())
            m_data.insert(m_data.begin()+index,Type()); 
        else
            m_data.push_back(Type());
    }
    /// Removes an item at index.
    void remove(size_t index){m_data.erase(m_data.begin()+index);}
    /// Returns a pointer to the data element.
    void* void_pointer(size_t index){return &m_data[index];}
private:
    /// Column data
    std::vector<Type> m_data;
    friend class TableWorkspace;
};

/// Shared pointer to a column with aoutomatic type cast and data type check.
/// Can be created with TableWorkspace::getColumn(...)
template< class T>
class TableColumn_ptr: public boost::shared_ptr<TableColumn<T> >
{
public:
    /** Constructor
        @param c :: Shared pointer to a column
      */
    TableColumn_ptr(boost::shared_ptr<API::Column> c):boost::shared_ptr<TableColumn<T> >(boost::dynamic_pointer_cast<TableColumn<T> >(c))
    {
        if (this->get() == NULL)
        {
            Kernel::Logger& log = Kernel::Logger::get("TableWorkspace");
            std::string str = "Data type of column "+c->name()+" does not match "+typeid(T).name();
            log.error(str);
            throw std::runtime_error(str);
        }
    }
};

/// Special case of bool
template<>
class TableColumn_ptr<bool>: public TableColumn_ptr<API::Boolean>
{
public:
    /** Constructor
        @param c :: Shared pointer to a column
      */
    TableColumn_ptr(boost::shared_ptr<API::Column> c):TableColumn_ptr<API::Boolean>(c)
    {
        if (this->get() == NULL)
        {
            Kernel::Logger& log = Kernel::Logger::get("TableWorkspace");
            std::string str = "Data type of column "+c->name()+" does not match "+typeid(API::Boolean).name();
            log.error(str);
            throw std::runtime_error(str);
        }
    }
};

} // namespace DataObjects
} // Namespace Mantid

/*
    Macro to declare a type to be used with TableColumn.
    DataType is the actual C++ type. TypeName is a symbolic name, used in TableWorkspace::createColumn(...)
    TypeName can contain only letters, numbers and _s.
*/
#define DECLARE_TABLECOLUMN(DataType,TypeName) \
    namespace{ \
    Mantid::Kernel::RegistrationHelper register_column_##TypeName(  \
    (Mantid::API::ColumnFactory::Instance().subscribe< Mantid::DataObjects::TableColumn< DataType > >(#TypeName),0)); \
    } 

#endif /*MANTID_DATAOBJECTS_TABLECOLUMN_H_*/
