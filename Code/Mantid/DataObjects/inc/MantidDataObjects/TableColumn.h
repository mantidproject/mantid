#ifndef MANTID_DATAOBJECTS_TABLECOLUMN_H_
#define MANTID_DATAOBJECTS_TABLECOLUMN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidDataObjects/Column.h"
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
template <class Type>
class DLLExport TableColumn: public Column
{
public:

    //TableColumn();
    /// Virtual destructor.
    virtual ~TableColumn(){}
    /// Number of individual elements in the column.
    int size()const{return int(m_data.size());}
    /// Reference to the data.
    std::vector<Type>& data(){return m_data;}
    const std::type_info& get_type_info()const{return typeid(Type);}
    const std::type_info& get_pointer_type_info()const{return typeid(Type*);}
    void print(std::ostream& s, int index)const{s << m_data[index];}
protected:
    /// Resize.
    void resize(int count){m_data.resize(count);}
    /// Inserts default value at position index. 
    void insert(int index)
    {
        if (index < int(m_data.size()))
            m_data.insert(m_data.begin()+index,Type()); 
        else
            m_data.push_back(Type());
    }
    /// Removes an item.
    void remove(int index){m_data.erase(m_data.begin()+index);}
    void* void_pointer(int index){return &m_data[index];}
private:
    std::vector<Type> m_data;
    friend class TableWorkspace;
};


template< class T>
class TableColumn_ptr: public boost::shared_ptr<TableColumn<T> >
{
public:
    TableColumn_ptr(boost::shared_ptr<Column> c):boost::shared_ptr<TableColumn<T> >(boost::dynamic_pointer_cast<TableColumn<T> >(c))
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

struct Boolean
{
    Boolean():value(false){}
    Boolean(bool b):value(b){}
    operator bool(){return value;}
    bool value;
};

DLLExport std::ostream& operator<<(std::ostream& s,const Boolean& b);

} // namespace DataObjects
} // Namespace Mantid

#define DECLARE_TABLECOLUMN(DataType,TypeName) \
    namespace{ \
    Mantid::Kernel::RegistrationHelper register_column_##TypeName(  \
    (Mantid::DataObjects::ColumnFactory::Instance().subscribe< Mantid::DataObjects::TableColumn< DataType > >(#TypeName),0)); \
    } 

#endif /*MANTID_DATAOBJECTS_TABLECOLUMN_H_*/
